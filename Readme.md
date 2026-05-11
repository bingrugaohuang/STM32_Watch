# #串口任务 
## 5.1 中断中 FromISR 函数的 pxHigherPriorityTaskWoken 设计
FreeRTOS 要求：**所有可能唤醒高优先级任务的中断 API，都必须输出一个 `pxHigherPriorityTaskWoken` 标志，并且在 ISR 结束前统一调用 `portYIELD_FROM_ISR` 进行切换。**

原因有三：

1. **切换时机唯一**  
    任务切换必须在 ISR 的**最后**执行，不能中途随意切走。如果 `serial_send_async_from_isr` 内部私自调用了 `portYIELD_FROM_ISR`，但 ISR 后面还有代码（比如清理硬件标志），就会导致灾难性错误。因此切换权必须交给 ISR 的编写者。
    
2. **一次中断可能触发多次唤醒**  
    一个 ISR 里可能调用多个 `FromISR` 函数（比如先释放信号量，再发送队列）。这些函数都使用**同一个** `xHigherPriorityTaskWoken` 变量，所有唤醒信息会“累积”起来。中断结束时只需检查一次。如果每个函数内部自己执行切换，会造成多次切换和逻辑混乱。
    
3. **实时性保证**  
    如果隐藏了这个机制，假设你调用 `serial_send_async_from_isr` 时确实唤醒了一个高优先级任务，但没有执行切换，这个任务就要一直等到下一个系统节拍（或别的中断），这违反了实时系统的低延迟要求。
因此，**任何在中断中调用的、可能唤醒更高优先级任务的 FreeRTOS API（包括 `FromISR` 版本的队列、信号量、任务通知、事件组等），都必须传入一个 `pxHigherPriorityTaskWoken` 参数，并在 ISR 末尾统一调用 `portYIELD_FROM_ISR( xHigherPriorityTaskWoken )`**


## 5.8 串口任务逻辑
1. 接口使用：其他任务要想使用异步串口发送，需要需要使用专门的接口函数serial_send_async，传入需要发送的内容地址，与内容大小。
2. 串口任务逻辑：
		1. 接口函数：首先从内存池链栈中分配一块内存块（256字节），随后用memcopy将形参地址中的内容复制到该内存块中，随后将内存块地址与size一起打包成结构体SerialTxMsg_t，通过队列打包至串口队列，发送至串口任务处理。
		注：若成功发送，返回0，参数错误，返回-1，内存块用完，返回-2，队列满，返回-3。
		2. 串口任务：收到包后，退出阻塞态，根据s_dma_busy判断dma是否没有处理完上一条信息，若处理完，先将标志位置1，再将包中的数据投入Start_DMA函数；若忙，阻塞在任务通知中，等待DMA回调函数发送任务通知并将需要free的地址发来，随后再free掉内存块，==***之后再将s_dma_busy置0，这时新任务才有可能退出while(s_dma_busy),进一步执行之后的操作。***==
		3. DMA开启函数：用静态变量存储空闲块地址，避免丢失，随后调用hal的API，开启DMA发送
		4. DMA发送完成回调：pxHigherPriorityTaskWoken的类型BaseType_t，定义为pdFALSE。随后发任务通知将发送完的空闲块地址也带过去free掉，不用开启临界区。最后portYIELD唤醒参数，判断在结束handler，进入线程模式时是否需要进入pendSV以进行一次任务调度。
3. 补充:
		1. 内存池：建立一个足够大的二维数组，第一维为8行，再创建一个链表，用头插法插入链表，再写一个计数信号量，初始值为8，alloc时从链表头部取走一块内存块，并将计术信号量减1，free时间内存块插回头部，并将计数信号量加1.
		2. 接口函数有两套API，因为其内部队列有两套API；alloc和free也有两套API，因为技术信号量的API有两套。
		3. 有额外的为应对突发情况，不能信赖中断而使用的API，如error_handler，该API中直接写入数据到发送数据寄存器，待该寄存器空后（TXE=1），再写入下一字节，最后移位寄存器将数据通过串口发出，待移位寄存器空后（TC=1），可退出。

## 5.8 小知识点
1. 某些架构（如 ARM Cortex-M）要求指针必须按字对齐（4 字节），否则访问未对齐地址可能导致硬件异常或性能下降。对齐后的缓冲区也能更高效地被 DMA 使用。
2. 队列会通过接口传来的地址值将改地址中的数据复制到内部缓冲区，因此队列是能够传递数据的，不过串口任务是由于要传的信息往往长度不定，因此采用内存池的方式先将数据存到内存块中，传递内存块的地址值

# #日志
## 5.8 日志逻辑
1. 接口使用：几个define的宏，使用tag，fmt两个参数以及一个...可变参数，使用示例：
	LOG_I("WIFI", "Hello");  (应该没错?)或者LOG_I("WIFI", "%lu-%s",(unsigned long)val1, (char*)var2);
	对应log(LOG_LEVEL_INFO, tag, fmt, ...);
2. 日志逻辑：
		1. 接口函数逻辑：先判断s_log_level是否为OFF或者level大于当前的s_log_level，是则返回，否则继续；下一步是用va_list变量将fmt以及...出的可变变量（有的话）用va_start将可变变量的值复制到args中（va_list变量，vsnprintf的可用参数）；随后调用s_backend直接将level，tag，fmt，args传递到后端。
		2. 后端：创建一个buf，存储将要发送到串口任务的信息；先将level用switch-case转化为字符串，再用snprintf将Tick，level，tag一起存入buf，并返回使用的长度len；再用vsnprintf格式化fmt和args，之后再判断总长度是否超过了sizeof（buf）-3,若超过，则将len设置为sizeof（buf）－3后写入\r\n\0，最后将buf地址和len通过队列传递给串口任务。
		注：
			1. 后端默认是串口发送，可通过注册接口函数注册为其他函数
			2. 日志提供更改s_log_level的接口函数
	
# #按键服务程序
1. 接口函数：假想中也许可以这么写，但是作用不够，还需要确定具体是哪个按键
不不不！！！，完全可以这样写，只要上层能判断就够了wok
	```c
	int button_service_get_event(KeyEvent_t *pEvent, uint32_t timeout_ms) {
    if (pEvent == NULL) return -1;
    if (osal_queue_receive(s_key_queue, pEvent,
                           timeout_ms == 0 ? OSAL_NO_WAIT : pdMS_TO_TICKS(timeout_ms)) == OSAL_OK) {
        return 0;
    }
    return -1;
}
	```
2.  APP层用例：
	```c
	void ui_task(void *pv) {
    button_service_init();   // 系统启动时调用一次

    while (1) {
        KeyEvent_t e;
        if (button_service_get_event(&e, OSAL_WAIT_FOREVER) == 0) {
            switch (e.button_id) {
                case 0:   // KEY1
                    if (e.event == BTN_EVENT_SHORT_PRESS) { ... }
                    else if (e.event == BTN_EVENT_LONG_PRESS) { ... }
                    break;
                case 1:   // KEY2
                    ...
            }
        }
    }
}
	```

# #现在
1. 现在要做的：可以开始将裸机部分的内容直接移植过来，单独为它创建一个任务，还需要将部分内容拆分开来，以及添加一些东西：
	1. 也许可以尝试用双向循环链表来管理滑动菜单？用栈来管理多级菜单？
	2. 闹钟需要一个单独的模块，睡眠管理也需要一个单独的模块，最好在sleep和stop模式基础上使用RTOS的TICKLESS实现低功耗，精益求精嘛，并且优化原有的代码逻辑，原有的状态机简直是屎山代码
	3. 要重写RTC文件，以适配闹钟模块，主要是要有写入RTC后不再复位不再重新写入，还需要将设定好的初试时间写入
	4. 单独一个服务模块（或者应用？不清楚放哪）用来放显示帧率的模块
	5. 为栈高水位监测专门写一个任务？（需要吗？毕竟是写产品级代码，还是说重要写一个功能模块就行？）
	6. 重写OLED驱动？重写I2C驱动?(毕竟没有实战过，对I2C并不算熟练)（需要放到通信协议层？但是没有规划）
	7. 设置部分需要单独写一个任务吗？内容包括设置时间，设置显示亮度，设置栈高水位的输出开关
	8. 如何将日志嵌入到这些内容中呢？
	9. ~~按键模块需要一个单独的任务（需要分两层吗？分为BSP和Drivers？）内容有通过exti进行任务通知，从而同步按键的读取，还有通过软件定时器进行长短按检测，这次我想设计一个状态机了hh（如何设计呢？）~~                                                                            
	10. 硬件电路的开关机别忘了，最好有个长按确定键，就弹出是否关机的弹窗，选择关机后就置某位电平为0（可能记错了？反正是用晶体管搭的简单开关机电路，通过该电路隔离电源的输入）
	11. 还有个电量检测ADC也没写，可以写在服务层？
2. 现在已经做了的：
	5.9：
	1. 移植了FreeRTOS，配置好了Hal库文件，并完成了架构的初步规划，在User文件夹中分了BSP、Drivers、MiddleWares、OSAL、Service、APP这些层（通信协议层怎么办）
	2. 完成了串口驱动，使用串口任务进行异步发送处理，接口仅需要写入要发送的信息的地址和长度即可
	3. 完成了日志文件的编写，提供了5个接口宏，包括ERROR,WARNING,INFO,DEBUG,TRACE，也提供了修改日志输出等级限制的接口函数，修改为OFF关日志；日志尽量简化，牺牲了一些实时性，提高了可移植性和代码简洁度以及RAM占用（用任务管理日志以减少格式化的阻塞影响会导致占用RAM高达4kb，20%）
	4. 完成了OSAL层对FreeRTOS的封装，便于以后移植其他RTOS（在下也有写一套RTOS的打算，以加深对RTOS与C语言的理解）
	5.11：
	5. 完成按键部分的代码编写，包含三个部分，分别放在BSP层，MiddleWares层，Service层：
		MiddleWares层主要提供一些可移植的抽象接口，包括按键结构体类型，按键事件类型，回调函数绑定，按键实例初始化，以及用于完成定时扫描的Ticks函数（需在定时器中调用）；
		BSP负责将中间层的抽象函数指针read_pin与具体的读取按键电平函数绑定；
		Service层主要负责创建按键实例，进行初始化与回调函数绑定，并在回调函数中通过队列处理按键事件的发送，在软件定时器中调用Ticks以完成扫描。此外，服务层还提供接收按键包的接口，内含按键事件与按键ID。
	6. 添加公共宏头文件，存放一些公共宏，位于Common文件夹

