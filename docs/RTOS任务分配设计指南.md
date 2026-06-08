# RTOS 任务分配与优先级设计指南

> **目标读者**：正在学习 FreeRTOS 的嵌入式入门者  
> **配套项目**：STM32_Watch_Rewrite（STM32F103C8T6 + FreeRTOS v10.3.1）  
> **前置阅读**：已理解 FreeRTOS 基本 API（xTaskCreate、vTaskDelay、xQueueSend 等）

---

## 目录

1. [前言：为什么任务分配是最难的入门课](#1-前言为什么任务分配是最难的入门课)
2. [原项目回顾：11 个任务的得与失](#2-原项目回顾11-个任务的得与失)
3. [核心概念速览：抢占、优先级、IPC](#3-核心概念速览抢占优先级ipc)
4. [任务分配第一性原理](#4-任务分配第一性原理)
5. [新项目任务架构设计](#5-新项目任务架构设计)
6. [优先级分配策略](#6-优先级分配策略)
7. [链表菜单系统设计](#7-链表菜单系统设计)
8. [驱动与任务的集成](#8-驱动与任务的集成)
9. [IPC 选型指南](#9-ipc-选型指南)
10. [栈大小估算方法](#10-栈大小估算方法)
11. [渐进式实现路线图](#11-渐进式实现路线图)
12. [常见陷阱与调试技巧](#12-常见陷阱与调试技巧)

---

## 1. 前言：为什么任务分配是最难的入门课

### 1.1 从裸机到 RTOS 的思维跃迁

裸机编程时，你的代码是这样的：

```c
while (1) {
    read_button();       // 扫描按键
    update_display();    // 刷新屏幕
    read_sensor();       // 读取传感器
    delay_ms(10);        // 等待
}
```

一切是**顺序的、可预测的**。你知道 `read_button` 一定在 `update_display` 之前执行。

FreeRTOS 开启后，你面对的是：

```c
// 任务1：每 10ms 运行一次
void sensor_task() { while(1) { read_mpu6050(); vTaskDelay(10); } }

// 任务2：每 20ms 运行一次
void display_task() { while(1) { update_oled(); vTaskDelay(20); } }

// 任务3：事件驱动
void button_task() { while(1) { wait_for_press(); handle_button(); } }
```

这三个任务**并发运行**（至少看起来是这样）。它们的执行顺序取决于**优先级**和**时间**。这就是 RTOS 的核心挑战：

> **在裸机中，你控制执行顺序。在 RTOS 中，你只控制竞争规则。**

### 1.2 糟糕的任务分配长什么样

以下是一个真实的"反面教材"（很多初学者都会这样写）：

```c
// ❌ 反面案例：为每个功能创建一个任务
xTaskCreate(task_oled_init,     "oled_init",   256, NULL, 2, NULL);
xTaskCreate(task_oled_display,  "oled_disp",   512, NULL, 2, NULL);
xTaskCreate(task_mpu6050_read,  "mpu_read",    256, NULL, 3, NULL);
xTaskCreate(task_mpu6050_calc,  "mpu_calc",    512, NULL, 3, NULL);
xTaskCreate(task_menu_clock,    "menu_clk",    384, NULL, 2, NULL);
xTaskCreate(task_menu_setting,  "menu_set",    384, NULL, 2, NULL);
xTaskCreate(task_menu_game,     "menu_game",   512, NULL, 2, NULL);
xTaskCreate(task_battery_adc,   "bat_adc",     128, NULL, 1, NULL);
// ... 10+ tasks，每个栈 256-512 字
// 总栈消耗: 10 * 400 * 4 = 16KB（已经超过 SRAM！）
```

问题：
1. **栈空间爆炸**：STM32F103C8T6 只有 20KB SRAM，光是任务栈就耗尽了
2. **过度同步**：OLED 初始化和显示刷新强耦合，却分成了两个任务，需要通过信号量协调
3. **优先级混乱**：不知道哪个任务该更高，只能"感觉"来设
4. **上下文切换浪费**：10 个任务 = 频繁切换 = CPU 时间花在切换而非工作上

### 1.3 好的任务分配长什么样

```
任务数: 5 个
总栈:   ~2.5KB（vs 16KB）
IPC:    2 个队列 + 1 个互斥锁 + 1 个任务通知
每个任务都有清晰的职责边界
优先级有理有据
```

这就是我们要达到的目标。下面我们从原项目开始分析。

---

## 2. 原项目回顾：11 个任务的得与失

### 2.1 原项目任务全景

你第一次写的 STM32_Watch 项目创建了以下任务：

| # | 任务名 | 优先级 | 栈(字) | 职责 |
|---|--------|--------|--------|------|
| 1 | Key_Task | **最高** | 128 | 按键扫描+去抖+分发 |
| 2 | Alarm_Task | **次高** | 256 | RTC 闹钟响应 |
| 3 | UI_Task | 普通 | 192 | 主屏幕（时钟） |
| 4 | Menu_Task | 普通 | 128 | 动画菜单导航 |
| 5 | Set_Task | 普通 | 192 | 设置界面 |
| 6 | Time_Task | 普通 | 192 | 秒表+闹钟设置 |
| 7 | Flashlight_Task | 普通 | 128 | 手电筒 PWM 控制 |
| 8 | MPU6050_Task | 普通 | 256 | 角度显示+水平仪 |
| 9 | Game_Task | 普通 | 128 | 恐龙游戏 |
| 10 | SleepManager_Task | 普通 | 128 | 电源管理状态机 |
| 11 | StackMonitor_Task | 普通 | 192 | 每 5s 上报栈水位 |

另外还有：
- FreeRTOS 软件定时器守护任务（优先级 2，栈 256）
- 空闲任务（优先级 0，栈 128）
- `StartDefaultTask`（创建上述任务后自动删除）

### 2.2 架构模式：单活跃任务 + 挂起/恢复切换

原项目使用了一种特殊的调度方式——**TaskMgr 切换机制**：

```c
// 当前只有 ONE 任务在运行，其他全部挂起
void Menu_Task() {
    Key_GetNum(&key);  // 等待按键
    if (key == KEY_CONFIRM)
        TaskMgr_ApplySwitchPlan(TASKMGR_TASK_MPU6050);  // 挂起自己，恢复 MPU6050
}
```

**优点**：
- 节省栈空间（挂起的任务不消耗 CPU）
- 简化状态管理（一次只做一件事）
- 不需要复杂的 IPC

**缺点**：
- 任务启动/挂起耗时（每次切换都要调度器介入）
- 无法并行处理（比如 MPU6050 在后台读取数据时，前端 UI 冻结）
- 扩展性差（每增加一个菜单就要增加一个任务）
- 优先级失去了意义（所有任务同优先级，只是靠挂起来"切换"）

### 2.3 值得保留的设计

尽管架构有改进空间，原项目有几个闪光点值得在新项目中保留：

1. **Key_Task 的高优先级设计**：按键是最关键的输入，应该最高优先级响应。新项目中这个逻辑被 Button 中间件替代（10ms 定时器扫描），但思想一致。

2. **I2C 互斥锁保护**：`OLED_MutexHandle` 和 `MPU6050_MutexHandle` 确保了 I2C 总线的线程安全——这是正确的做法。

3. **任务通知用于 ISR→Task 通信**：比信号量更轻量、更快。新项目中的 DMA 完成回调也在使用这个模式。

4. **栈水位监控**：`uxTaskGetStackHighWaterMark()` 是调试栈溢出的利器，新项目应该保留。

---

## 3. 核心概念速览：抢占、优先级、IPC

### 3.1 抢占式调度器如何工作

FreeRTOS 使用**抢占式调度器**（`configUSE_PREEMPTION = 1`）：

```
时间线:
t=0ms    Task A (priority 3) 正在运行
t=5ms    Task B (priority 5) 被中断唤醒
         调度器立即暂停 A，切换 B 运行   ← 这就是"抢占"
t=15ms   Task B 调用 vTaskDelay() 阻塞
         调度器恢复 A 继续运行
```

关键规则：
- **高优先级就绪 = 低优先级立即暂停**
- **同优先级 = 时间片轮转**（如果 `configUSE_TIME_SLICING = 1`）
- **永远有任务在运行**（至少是空闲任务）

### 3.2 优先级反转：最经典的 RTOS 陷阱

```c
// 场景：低优先级任务持有互斥锁，高优先级任务在等待
Task_L (prio 1): 获取 Mutex → 被抢占
Task_M (prio 3): 运行中（跟 Mutex 无关，但它占着 CPU）
Task_H (prio 5): 尝试获取 Mutex → 阻塞！因为 Task_L 持有
// Task_H 优先级是 5，但必须等 Task_L (优先级 1) 释放锁
// 而 Task_M (优先级 3) 还在 CPU 上跑... Task_L 永远没机会释放锁！
```

**解决方案**：FreeRTOS 的互斥锁自带**优先级继承**——当 Task_H 等待 Mutex 时，Task_L 的优先级被临时提升到 5，确保它能尽快释放锁。

> **教训**：共享资源用互斥锁保护时，优先使用 `osal_recursive_mutex_create()`（OSAL 已封装），它有优先级继承机制。

### 3.3 IPC 工具选择决策树

```
需要在线程间传数据？
├── 数据是"事件"（按键按下、闹钟触发）
│   └── 用任务通知（最快）或队列（支持多消费者）
├── 数据是"流"（传感器读数、日志消息）
│   └── 用队列（FIFO，自然匹配生产者-消费者模式）
├── 需要保护共享资源（I2C 总线、显示缓冲区）
│   └── 用互斥锁（支持优先级继承）
├── 需要同步（A 完成后再做 B）
│   └── 用信号量（二值）或任务通知
└── 需要管理资源池（内存块、DMA 通道）
    └── 用计数信号量
```

### 3.4 configMAX_PRIORITIES 和理解你的优先级范围

新项目配置：`configMAX_PRIORITIES = 10`  
可用优先级：**0（最低）到 9（最高）**

但要注意：
- **优先级 9**：被软件定时器守护任务占用（`configTIMER_TASK_PRIORITY = 9`）
- **优先级 0**：空闲任务
- **实际可用**：优先级 1-8（共 8 个可用等级）

对于我们的项目来说绰绰有余——我们只需要 4-5 个不同的优先级。

---

## 4. 任务分配第一性原理

### 4.1 原则一：以数据流为中心，而非以功能为中心

初学者常犯的错误是"一个功能一个任务"。正确的思路是看**数据从哪里来、到哪里去**：

```
传感器 → [采集] → [处理] → [显示] → OLED
                        ↘ [存储]
按键   → [处理] → [UI 状态机] ↗
ADC    → [采集] → [显示] ↗
```

从这个数据流图中，我们可以识别出几个"流"：
- **传感器流**：MPU6050 → 数据采集 → 角度计算 → 显示
- **UI 流**：按键 → 状态机 → 画面渲染 → OLED
- **慢速流**：ADC → 电池百分比 → 显示
- **输出流**：日志/调试 → 串口

每个流可以是**一个任务**，也可以拆分成**多个任务**——取决于实时性要求。

### 4.2 原则二：任务 = 独立的执行节奏

一个任务应该有自己的**运行节奏**（周期性、事件驱动、或混合）：

| 任务类型 | 运行节奏 | 例子 |
|----------|----------|------|
| 周期性 | 每 N ms 运行一次 | 传感器采样（10ms）、屏幕刷新（20ms） |
| 事件驱动 | 有事才运行 | 按键处理、闹钟响应 |
| 混合型 | 平时等待事件，偶尔定时检查 | UI 状态机（等按键，偶尔刷新动画） |

**如果一个"任务"没有自己独立的节奏，它就不该是任务，而该是函数调用。**

### 4.3 原则三：I2C 等慢速外设要"归主"

I2C 总线是独占资源。如果两个任务都要访问同一个 I2C 总线：
```c
// ❌ 不好
Task_A: I2C1_Write(OLED, data);  // 正在写...
Task_B: [抢占] I2C1_Write(OLED, data2);  // 破坏传输！
```

**最佳实践**：让一个任务"拥有"I2C 总线，其他任务通过队列向它发送请求。
```c
// ✅ 好
Task_A: xQueueSend(oled_queue, &request);  // 提交请求
Task_Display: xQueueReceive(oled_queue, &request); I2C1_Write(OLED, request.data);
```

本项目硬件有两路 I2C：
- **I2C1（软件）→ OLED**：只有显示任务占用 → **不需要互斥锁**
- **I2C2（硬件）→ MPU6050**：只有传感器任务占用 → **不需要互斥锁**

但如果未来 I2C2 还被其他设备共享（比如扩展 EEPROM），就需要互斥锁。

### 4.4 原则四：栈是稀缺资源，要精打细算

STM32F103C8T6 只有 **20KB SRAM**。我们来算一笔账：

```
FreeRTOS 堆:    10KB (configTOTAL_HEAP_SIZE)
系统栈 (MSP):    ~1KB
任务栈总计:      ~2.5KB (我们的目标)
全局变量/静态:   ~3KB (OLED 帧缓冲区 1KB+ 其他)
─────────────────────────
剩余可用:        约 3.5KB
```

每个任务的栈主要被以下因素消耗：
- 局部变量（特别是大数组，如 `char buf[256]`）
- 函数调用深度（每层调用约 64-128 字节）
- 中断嵌套保存的上下文（64 字节/次）
- sprintf / 浮点运算（临时变量很多）

> **经验法则**：先用 `uxTaskGetStackHighWaterMark()` 测量实际使用量，再加 50% 安全余量。256 字 = 1KB 栈空间对于不做浮点运算的简单任务通常够用。

---

## 5. 新项目任务架构设计

### 5.1 推荐的任务分配方案

基于以上原则，结合 STM32F103C8T6 的硬件资源和你项目的实际需求，推荐以下任务架构：

| # | 任务名 | 优先级 | 栈(字) | 周期 | 职责 |
|---|--------|--------|--------|------|------|
| 1 | `serial_tx` | 5 | 256 | 事件驱动 | 串口 DMA 异步发送（已有） |
| 2 | `sensor_task` | 4 | 512 | 10ms | MPU6050 读取+姿态解算 |
| 3 | `display_task` | 3 | 768 | 20ms/事件 | OLED 渲染+UI 状态机+菜单系统 |
| 4 | `battery_task` | 2 | 256 | 1000ms | ADC 电池电压采样 |
| 5 | `sysmon_task` | 1 | 256 | 5000ms | 栈水位上报+系统健康检查 |

**总计**：5 个用户任务，栈占用约 2KB，堆占用约 800 字节（队列+互斥锁）。

### 5.2 为什么是 5 个任务而不是 3 个或 8 个？

**如果合并成 3 个任务**：
```
display_task 同时处理 UI + 传感器 + 电池
```
问题：传感器需要 10ms 精确周期，但 UI 渲染可能被 I2C 通信阻塞（每次 I2C 传输约 1ms，全屏刷新约 200ms）。把它们合在一起，传感器采样会严重抖动。

**如果拆成 8 个任务**：
```
sensor_read_task + sensor_calc_task + display_render_task + ui_logic_task + ...
```
问题：过度拆分导致大量 IPC 开销，而且栈空间紧张。对于 20KB SRAM 的芯片，省着点用。

**5 个任务是"刚好"的平衡点**：每个任务有独立的节奏和清晰的职责，IPC 简单高效。

### 5.3 每个任务的详细设计

#### 5.3.1 `serial_tx`（优先级 5，栈 256 字）—— 已有，无需修改

```c
// serial.c 中已实现
void serial_tx_task(void *pvParameters) {
    while (1) {
        SerialTxMsg_t msg;
        osal_queue_receive(s_tx_queue, &msg, OSAL_WAIT_FOREVER);
        // 等待上次 DMA 完成
        while (s_dma_busy) {
            osal_task_notify_wait(0, 0xFFFFFFFF, &notified_value, OSAL_WAIT_FOREVER);
            serial_dma_cleanup((uint8_t *)notified_value);
            s_dma_busy = 0;
        }
        s_dma_busy = 1;
        serial_start_dma(msg.pData, msg.size);
    }
}
```

**为什么优先级是 5 而不是更高？**
- 串口发送不是实时性要求很高的任务（日志晚几毫秒发送没关系）
- 但也不能太低，否则日志队列可能溢出（特别是调试时打印大量信息）
- 优先级 5 在 0-9 的范围内处于中上，比 UI 和传感器都高，确保日志及时发出

#### 5.3.2 `sensor_task`（优先级 4，栈 512 字，周期 10ms）

**职责**：
1. 通过 I2C2 读取 MPU6050 的 6 轴原始数据（加速度计 X/Y/Z + 陀螺仪 X/Y/Z）
2. 运行姿态解算算法（互补滤波或 Mahony/Madgwick AHRS）
3. 将解算后的姿态数据（Roll/Pitch/Yaw）通过队列发送给 display_task

**伪代码**：
```c
typedef struct {
    float roll, pitch, yaw;
    int16_t accel_x, accel_y, accel_z;
    int16_t gyro_x, gyro_y, gyro_z;
} SensorData_t;

void sensor_task(void *pvParameters) {
    SensorData_t data;
    TickType_t last_wake = osal_get_tick();

    while (1) {
        // 1. 读取原始数据（I2C 传输 ~0.5ms）
        mpu6050_read_all(&data.accel_x, /*...*/);

        // 2. 姿态解算（互补滤波 ~0.2ms）
        mpu6050_compute_attitude(&data);

        // 3. 发送给 display_task（非阻塞，队列满则丢弃旧数据）
        osal_queue_send(sensor_queue, &data, OSAL_NO_WAIT);

        // 4. 精确延时到下一个 10ms 周期
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10));
    }
}
```

**为什么优先级是 4？**
- 传感器数据是实时性的，采样周期必须稳定（10ms 一次，不能有大抖动）
- 优先级低于 serial_tx(5)，因为串口 DMA 中断会影响整个系统
- 优先级高于 display_task(3)，因为传感器数据必须及时更新
- 利用了 **Rate Monotonic Scheduling (RMS)** 原理——周期越短，优先级越高

**为什么栈是 512 字？**
- 姿态解算涉及浮点运算，编译器会产生大量的临时变量
- I2C HAL 库的函数调用链较深（`HAL_I2C_Mem_Read` → `I2C_WaitOnFlagUntilTimeout` → ...）
- 传感器数据结构体（~40 字节）和计算缓冲区

#### 5.3.3 `display_task`（优先级 3，栈 768 字，混合模式）

**这是最复杂的任务，合并了三个职责**：

1. **OLED 渲染引擎**：管理帧缓冲区（1024 字节），负责所有图形绘制
2. **UI 状态机/菜单系统**：基于链表的多级菜单管理（详见第 7 节）
3. **事件处理**：处理按键事件、传感器数据更新、电池数据更新

**为什么把 UI + 渲染合并？**
- 显示任务和 UI 逻辑是强耦合的——UI 状态直接决定渲染内容
- OLED 的 I2C1 专属于此任务，没有竞争
- 合并后不需要额外的队列来传递"渲染命令"
- 栈可以共享（渲染缓冲区和 UI 状态机共用同一个栈空间）

**伪代码**：
```c
void display_task(void *pvParameters) {
    Btn_pkg_t    btn_event;
    SensorData_t sensor_data;
    uint8_t      battery_pct;
    MenuNode_t  *current_menu = &main_menu;  // 链表入口

    oled_init();     // 初始化 OLED（I2C1 专用）
    oled_clear();    // 清屏

    while (1) {
        // 1. 非阻塞检查所有输入源
        //    按键事件（100ms 超时 — 如果没有按键，也会超时以刷新屏幕）
        if (osal_queue_receive(btn_queue, &btn_event, pdMS_TO_TICKS(100)) == OSAL_OK) {
            menu_handle_input(current_menu, &btn_event);
        }

        // 2. 非阻塞获取最新传感器数据
        if (osal_queue_receive(sensor_queue, &sensor_data, OSAL_NO_WAIT) == OSAL_OK) {
            // 更新显示用的传感器缓存
        }

        // 3. 非阻塞获取最新电池数据
        if (osal_queue_receive(battery_queue, &battery_pct, OSAL_NO_WAIT) == OSAL_OK) {
            // 更新电池图标
        }

        // 4. 渲染当前画面
        menu_render(current_menu);
        oled_flush();  // 将帧缓冲区通过 I2C 发送到 OLED（~200ms）
    }
}
```

**为什么优先级是 3？**
- UI 不需要很高的实时性（人眼对 50ms 以下的延迟不敏感）
- 低于 sensor_task(4)，传感器采样不会被 UI 渲染阻塞
- 但是也不能太低——否则按键响应会感到"迟钝"

**为什么栈是 768 字？**
- 需要容纳 OLED 的局部渲染缓冲区（`uint8_t line_buf[128]` = 128 字节）
- 菜单链表遍历（递归或深度遍历会产生多层函数调用栈）
- 字体绘制函数（特别是中文，需要查表）
- 这 768 字是一个相对安全的初始值，后续可以通过栈水位监控来优化

#### 5.3.4 `battery_task`（优先级 2，栈 256 字，周期 1000ms）

**职责**：
1. 启动 ADC 采样（读取电池分压后的电压）
2. 将 ADC 值换算为电压和电量百分比
3. 通过队列发送给 display_task

**伪代码**：
```c
void battery_task(void *pvParameters) {
    uint32_t adc_value;
    uint8_t  battery_pct;
    TickType_t last_wake = osal_get_tick();

    while (1) {
        // 读取 ADC（假设使用 ADC1 通道，查询模式）
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
            adc_value = HAL_ADC_GetValue(&hadc1);
        }
        HAL_ADC_Stop(&hadc1);

        // 换算: 假设电池 3.7V~4.2V，经 1:1 分压后进 ADC
        // ADC 参考电压 3.3V，12 位分辨率
        // battery_pct = ...
        
        osal_queue_send(battery_queue, &battery_pct, OSAL_NO_WAIT);
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(1000));
    }
}
```

**为什么优先级是 2？**
- 电量变化极其缓慢（秒级），对实时性零要求
- 使用 vTaskDelayUntil（而非普通 delay）确保 1 秒的精确周期

#### 5.3.5 `sysmon_task`（优先级 1，栈 256 字，周期 5000ms）

**职责**：
1. 每 5 秒收集各任务栈高水位
2. 计算 FreeRTOS 堆剩余空间
3. 通过 LOG_I 输出调试信息
4. 检查异常状态（如某个任务栈水位过低）

```c
void sysmon_task(void *pvParameters) {
    while (1) {
        // 栈水位监测
        UBaseType_t highWater;
        highWater = uxTaskGetStackHighWaterMark(sensor_task_handle);
        LOG_D("SYS", "Sensor stack: %u/%u", highWater, 512);

        // 堆剩余空间
        size_t free_heap = xPortGetFreeHeapSize();
        LOG_D("SYS", "Free heap: %u/%u", free_heap, configTOTAL_HEAP_SIZE);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

**为什么优先级最低？**
- 纯诊断用途，任何其他任务都应该优先于它
- 即使长时间得不到运行也不影响系统功能

### 5.4 任务架构全景图

```
优先级
  9  ═══ Timer Daemon ═══ (FreeRTOS 内部，驱动按键扫描 10ms)
  8  ═══ (预留) ═══
  7  ═══ (预留) ═══
  6  ═══ (预留) ═══
  5  ═══ serial_tx ═══       串口 DMA 发送
  4  ═══ sensor_task ═══     MPU6050@10ms
  3  ═══ display_task ═══    OLED+UI+菜单
  2  ═══ battery_task ═══    ADC@1s
  1  ═══ sysmon_task ═══     系统监控@5s
  0  ═══ IDLE ═══            空闲（可做低功耗）
```

```
IPC 数据流:
                          ┌─────────────┐
                          │ button_scan │ (定时器回调)
                          └──────┬──────┘
                                 │ btn_queue
                                 ▼
  ┌──────────┐   sensor_queue   ┌──────────────┐
  │ sensor   │ ───────────────► │              │
  │ @I2C2    │                  │   display    │ ──► OLED (I2C1)
  └──────────┘                  │   (UI+渲染)  │
  ┌──────────┐  battery_queue   │              │
  │ battery  │ ───────────────► │              │
  │ @ADC     │                  └──────────────┘
  └──────────┘                        │
                               log_print()
                                      │ s_tx_queue
                                      ▼
                               ┌──────────────┐
                               │  serial_tx   │ ──► USART1 (DMA)
                               └──────────────┘
```

---

## 6. 优先级分配策略

### 6.1 Rate Monotonic Scheduling (RMS) — 理论依据

RMS 是实时系统最经典的优先级分配算法：

> **周期越短的任务，优先级越高。**

数学原理：对于一组周期性任务，如果满足以下条件，RMS 能保证所有任务都在截止时间前完成：

```
Σ(Ci / Ti) ≤ n × (2^(1/n) - 1)
其中 Ci = 任务 i 的最坏执行时间，Ti = 周期，n = 任务数
```

应用到我们的项目：

| 任务 | 周期 Ti | 估算执行时间 Ci | Ci/Ti |
|------|---------|-----------------|-------|
| sensor_task | 10ms | ~0.7ms | 7% |
| display_task | 20ms | ~10ms | 50% |
| battery_task | 1000ms | ~1ms | 0.1% |
| sysmon_task | 5000ms | ~2ms | 0.04% |

总利用率 = 7% + 50% + 0.1% + 0.04% ≈ 57.1%  
4 个任务的理论上限 = 4 × (2^(1/4)-1) ≈ 75.7%  
**57.1% < 75.7% → 系统可调度** ✅

> **但是**，display_task 的 50% CPU 占用看起来很高——这是因为 I2C 发送是阻塞的（每刷新一帧约 200ms）。实际上在 I2C 等待期间，CPU 大部分时间在空转轮询。这意味着实际 CPU 负载远低于 50%。

### 6.2 为什么不用"感觉"来分配优先级

初学者常见做法：
```c
// "我觉得 OLED 很重要，给它高优先级吧"
display_task prio = 6;
sensor_task prio = 3;
```

问题：当 display_task 正在渲染（200ms I2C 阻塞）时，sensor_task 被延迟了整整 200ms！传感器数据出现严重抖动。

**正确做法**：用数据说话——画出每个任务的时间线，找到最苛刻的实时性要求，然后反推优先级。

### 6.3 优先级间隔：为什么要留空位

注意到我们的优先级不是连续的（1,2,3,4,5 而非 1,3,5,7,9）：

```c
#define PRIO_SERIAL   5
#define PRIO_SENSOR   4
#define PRIO_DISPLAY  3
#define PRIO_BATTERY  2
#define PRIO_SYSMON   1
```

**为什么不留间隔？**
- 对于只有 5 个任务的项目，不需要间隔
- 间隔的用途是方便未来插入新任务（比如 PRIO_SENSOR 和 PRIO_DISPLAY 之间插入一个数据处理任务）
- 但如果确定不需要插入，连续分配更直观

**什么时候需要间隔？** 大型项目（20+ 任务），或者你在设计一个会被其他人扩展的框架。对于学习项目，连续即可。

---

## 7. 链表菜单系统设计

### 7.1 为什么链表比多个任务好

原项目每个菜单一个任务：
```
UI_Task → Menu_Task → Set_Task → Time_Task → ...
```
问题：添加新菜单 = 添加新任务 + 修改 TaskMgr 切换表 + 调整优先级。

链表方案：
```c
// 菜单节点
typedef struct MenuNode {
    const char    *title;          // 菜单标题
    struct MenuNode *parent;       // 父菜单（返回上级）
    struct MenuNode *children;     // 子菜单链表头
    struct MenuNode *next;         // 同级下一个菜单
    void (*on_enter)(void);        // 进入此菜单的回调
    void (*on_render)(void);       // 渲染此菜单的回调
    void (*on_input)(BtnEvent_t);  // 处理按键输入
    uint8_t         child_count;   // 子菜单数量
} MenuNode_t;
```

**添加新菜单只需要创建一个新的 MenuNode 并链接到链表中**，不需要创建任务。

### 7.2 链表菜单树结构

```
main_menu (主屏幕)
├── settings_menu (设置)
│   ├── time_setting (时间设置)
│   ├── brightness_setting (亮度设置)
│   └── about_screen (关于)
├── sensor_menu (传感器)
│   ├── angle_display (角度显示)
│   └── level_meter (水平仪)
├── tools_menu (工具)
│   ├── stopwatch (秒表)
│   ├── flashlight (手电筒)
│   └── game (游戏)
└── power_menu (电源)
    └── sleep_setting (休眠设置)
```

### 7.3 菜单状态机

```c
typedef enum {
    MENU_EVENT_NONE = 0,
    MENU_EVENT_UP,          // 上一个菜单项
    MENU_EVENT_DOWN,        // 下一个菜单项
    MENU_EVENT_CONFIRM,     // 进入子菜单
    MENU_EVENT_BACK,        // 返回父菜单
} MenuEvent_t;

// 在 display_task 中：
void menu_handle_input(MenuNode_t **current, Btn_pkg_t *btn) {
    MenuEvent_t event = btn_to_menu_event(btn);  // 按键映射到菜单事件
    
    switch (event) {
    case MENU_EVENT_UP:
        if ((*current)->prev)
            *current = (*current)->prev;
        break;
    case MENU_EVENT_DOWN:
        if ((*current)->next)
            *current = (*current)->next;
        break;
    case MENU_EVENT_CONFIRM:
        if ((*current)->children) {
            *current = (*current)->children;  // 进入子菜单
            if ((*current)->on_enter) (*current)->on_enter();
        }
        break;
    case MENU_EVENT_BACK:
        if ((*current)->parent) {
            *current = (*current)->parent;    // 返回父菜单
            if ((*current)->on_enter) (*current)->on_enter();
        }
        break;
    }
}
```

### 7.4 键值 vs 事件 —— 为什么菜单不应该直接读按键值

原项目中的 UI 逻辑直接读取按键值：
```c
// ❌ 紧耦合：UI 逻辑与物理按键绑定
if (key == KEY_NEXT) { cursor++; }
```

新设计使用 **事件抽象**：
```c
// ✅ 松耦合：UI 逻辑只关心"确认"、"上下"等抽象事件
// 按键映射在 button_service 层完成，UI 层完全不知道物理按键布局
```

这样的好处：如果以后把"确认"键从 PA0 改到 PA6，只需要修改 BSP 层的按键映射，UI 逻辑完全不变。

---

## 8. 驱动与任务的集成

### 8.1 OLED 驱动（SSD1306/SH1106 + I2C1）

**谁拥有它**：`display_task` 独占。没有其他任务需要直接操作 OLED。

**初始化流程**：
```c
// 在 display_task 创建后立即执行（一次性）
void oled_init(void) {
    const I2C_Driver_t *i2c = I2C1_SW_GetDriver();  // 获取 I2C1 软件驱动
    
    // 发送初始化命令序列
    oled_write_cmd(i2c, 0xAE);  // Display OFF
    oled_write_cmd(i2c, 0xD5);  // Set display clock
    oled_write_cmd(i2c, 0x80);  // 100 FPS
    // ... 更多初始化命令
    oled_write_cmd(i2c, 0xAF);  // Display ON
}
```

**帧缓冲策略**：由于 STM32F103C8T6 内存紧张（20KB SRAM），建议使用**页缓冲**而非全帧缓冲：

```c
// 全帧缓冲: 128*64/8 = 1024 字节
uint8_t framebuffer[1024];

// 页缓冲（推荐）: 只缓冲当前页 128 字节
uint8_t page_buffer[128];
// 每次 flush 一页，共 8 页，每页 I2C 传输 ~25ms
// 全屏刷新 8 页 = ~200ms
```

如果使用全帧缓冲，display_task 栈需要增大到至少 1024+512=1536 字。使用页缓冲，768 字足够。

### 8.2 MPU6050 驱动（I2C2）

**谁拥有它**：`sensor_task` 独占。

**初始化**：
```c
void mpu6050_init(I2C_HandleTypeDef *hi2c) {
    // 唤醒 MPU6050
    mpu6050_write_reg(hi2c, 0x6B, 0x00);  // PWR_MGMT_1: 唤醒
    // 配置陀螺仪 ±2000dps
    mpu6050_write_reg(hi2c, 0x1B, 0x18);
    // 配置加速度计 ±16g
    mpu6050_write_reg(hi2c, 0x1C, 0x18);
    // DLPF 配置
    mpu6050_write_reg(hi2c, 0x1A, 0x03);  // 42Hz 带宽
}
```

### 8.3 ADC 电池检测

**谁拥有它**：`battery_task` 独占。

**硬件连接**：
```
电池正极 ──┬── [100KΩ] ──┬── ADC 输入 (如 PA1)
           │              │
           └── [100KΩ] ──┴── GND
```

这是一个简单的 1:1 分压。如果电池是 4.2V 满电，分压后进 ADC 的是 2.1V（在 3.3V 参考电压范围内）。

---

## 9. IPC 选型指南

### 9.1 本项目使用的 IPC 一览

| IPC 对象 | 生产者 | 消费者 | 类型 | 大小 | 阻塞策略 |
|----------|--------|--------|------|------|----------|
| `btn_queue` | button_scan 定时器回调 | display_task | Queue | 8 × Btn_pkg_t | 非阻塞写/带超时读 |
| `sensor_queue` | sensor_task | display_task | Queue | 2 × SensorData_t (60B) | 覆盖写(满时丢弃旧) |
| `battery_queue` | battery_task | display_task | Queue | 2 × uint8_t | 覆盖写 |
| `s_tx_queue` | log_print / 各种任务 | serial_tx_task | Queue | 16 × SerialTxMsg_t | 阻塞读 |
| `s_buf_sem` | serial 内存池 | serial_send_async | 计数信号量 | max=8 | 非阻塞 |
| `btn_scan_timer` | FreeRTOS 定时器 | button_ticks() | 软件定时器 | 10ms 周期 | N/A |
| `s_tx_task` 通知 | DMA 完成 ISR | serial_tx_task | 任务通知 | N/A | 覆盖写 |

### 9.2 传感器队列：为什么要"覆盖写"而不是"阻塞写"

传感器以 10ms 周期产生数据，但 display_task 可能正忙于 I2C 传输（200ms）。如果队列满了：
- **阻塞写**：sensor_task 被阻塞 200ms → 传感器采样周期被打乱 → 姿态解算出大错
- **覆盖写**：丢弃旧数据，写入新数据 → display_task 永远读最新姿态 → 传感器周期不受影响

```c
// 覆盖写实现：先尝试接收（清出空间），再发送
void sensor_queue_overwrite(SensorData_t *data) {
    SensorData_t dummy;
    while (osal_queue_receive(sensor_queue, &dummy, OSAL_NO_WAIT) == OSAL_OK) {
        // 丢弃旧数据
    }
    osal_queue_send(sensor_queue, data, OSAL_NO_WAIT);  // 一定有空间
}
```

### 9.3 按钮队列：为什么用"带超时的阻塞读"

display_task 使用带超时的接收来处理按键：
```c
osal_queue_receive(btn_queue, &btn_event, pdMS_TO_TICKS(100));
```

这样即使没有按键，任务也会每 100ms 醒一次——恰好可以用来刷新屏幕上的动态内容（比如时钟秒数、动画）。

---

## 10. 栈大小估算方法

### 10.1 理论估算

```
栈消耗 = 上下文保存 + 局部变量 + 函数调用深度 × 每层开销

每个任务的上下文保存 ≈ 64 字节（SVC 中断压栈）
armcc 每层函数调用 ≈ 8-16 字节（链接寄存器 + 少量局部变量）
局部变量取决于任务——数组是最大的消费者
```

### 10.2 实践方法：先用 uxTaskGetStackHighWaterMark 测量

```c
// 运行时获取栈水位
UBaseType_t remaining = uxTaskGetStackHighWaterMark(NULL);
// 返回值为"从未被使用过的栈空间"（字）
// 如果返回 0，说明栈可能已经溢出！

// 好的做法：首次运行时给 2 倍估算值，然后看实际使用量来优化
LOG_I("STACK", "display_task high water: %u words free from %u",
      uxTaskGetStackHighWaterMark(display_handle), STACK_DISPLAY);
```

**安全余量建议**：实际最大使用量 × 1.5 = 推荐栈大小。

### 10.3 本项目建议栈大小总结

| 任务 | 初始栈 | 依据 |
|------|--------|------|
| serial_tx | 256 | DMA 回调浅、无大数组；已验证可用 |
| sensor_task | 512 | MPU6050 HAL 调用深 + 浮点运算 + 姿态解算栈 |
| display_task | 768 | OLED 页缓冲 128B + 菜单递归 + 字体查表 |
| battery_task | 256 | HAL ADC 简单轮询，几乎无线程调用 |
| sysmon_task | 256 | 纯函数调用，无大数组 |

---

## 11. 渐进式实现路线图

任务分配方案确定后，建议按以下顺序逐步实现：

### 第 1 阶段：搭建骨架（已完成 ✅）

- [x] OSAL 抽象层
- [x] 串口驱动（serial_tx 任务，优先级 5）
- [x] 按键中间件（10ms 定时器扫描）
- [x] I2C 抽象接口 + 软件 I2C1 驱动
- [x] 日志系统
- [ ] 按钮队列创建（`prvCreateObjects` 中）

### 第 2 阶段：点亮屏幕 🔲 当前

1. 编写 OLED 驱动（基于 `I2C_Driver_t` 接口）
2. 创建 `display_task`（优先级 3，栈 768）
3. 在 OLED 上显示 "Hello World"
4. 对接按钮——按键后屏幕内容变化
5. **验证点**：按键能控制 OLED 显示

### 第 3 阶段：传感器接入 🔲

1. 编写 MPU6050 驱动（HAL I2C2）
2. 创建 `sensor_task`（优先级 4，栈 512）
3. 实现互补滤波或 Mahony 姿态解算
4. 通过 `sensor_queue` 发送数据到 display_task
5. 在 OLED 上显示 Roll/Pitch 角度
6. **验证点**：旋转 MPU6050，OLED 实时更新角度

### 第 4 阶段：链表菜单 🔲

1. 设计 `MenuNode_t` 数据结构
2. 将已有功能（时钟、传感器、设置）组织成菜单树
3. 实现 `menu_handle_input` 和 `menu_render`
4. **验证点**：按键浏览多级菜单，动态切换界面

### 第 5 阶段：完善系统 🔲

1. 创建 `battery_task`（优先级 2，栈 256）
2. 创建 `sysmon_task`（优先级 1，栈 256）
3. 对接日志系统
4. 运行 24 小时稳定性测试
5. **验证点**：栈水位报告正常，无溢出

---

## 12. 常见陷阱与调试技巧

### 12.1 栈溢出 —— 最难排查的 Bug

**症状**：
- 系统随机死机（HardFault）
- 某个任务"莫名其妙"改变了自己的局部变量
- 上下文切换后程序跳转到奇怪的地方

**原因**：栈溢出会破坏相邻的 TCB（任务控制块），导致整个 RTOS 状态混乱。

**检测方法**：
```c
// 方法1: 运行时检测（推荐）
configCHECK_FOR_STACK_OVERFLOW = 2;  // 已在 FreeRTOSConfig.h 中启用
// 溢出时会调用 vApplicationStackOverflowHook

// 方法2: 定期检查
UBaseType_t highwater = uxTaskGetStackHighWaterMark(handle);
if (highwater < 32)  // 少于 32 字剩余
    LOG_W("SYS", "Task %s stack low!", pcTaskGetName(handle));
```

### 12.2 优先级反转 —— 你的系统为什么"卡住"

如果出现了"某个低优先级任务不释放互斥锁，导致高优先级任务永久阻塞"的情况，检查：
1. 是否使用了递归互斥锁（`osal_recursive_mutex_create`）——它有优先级继承
2. 互斥锁持有时间是否过长——锁内不要调用 `vTaskDelay`
3. 是否存在死锁——A 等 B 的锁，B 等 A 的锁

### 12.3 不要在 ISR 中调用阻塞 API

```c
// ❌ 致命错误
void EXTI_IRQHandler() {
    xQueueSend(queue, &data, portMAX_DELAY);  // ISR 中不能永远等待!
}

// ✅ 正确
void EXTI_IRQHandler() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(queue, &data, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
```

### 12.4 系统滴答（Tick）的注意事项

你的项目使用 TIM4 作为 HAL 时基（因为 SysTick 被 FreeRTOS 占用）：
```c
// main.c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM4)
        HAL_IncTick();  // HAL_Delay 等的时基
}
```

如果 TIM4 中断优先级设置不当（太高），可能干扰 FreeRTOS 的临界区。确保 TIM4 的中断优先级不低于 `configMAX_SYSCALL_INTERRUPT_PRIORITY`（你的项目是 5）。

### 12.5 调试工具推荐

1. **栈水位 API**：`uxTaskGetStackHighWaterMark()` + `vTaskList()`（需要启用 `configUSE_TRACE_FACILITY` 和 `configUSE_STATS_FORMATTING_FUNCTIONS`——你的项目已启用 ✅）

2. **HardFault 调试器**：你的项目已经实现了 `hardfault_debug`，能打印异常栈帧和故障寄存器——这是嵌入式开发中非常有价值的工具 ✅

3. **运行时任务列表**：
```c
char buffer[1024];
vTaskList(buffer);  // 生成可读的任务列表
LOG_I("SYS", "\n%s", buffer);
// 输出:
// Name          State  Prio  Stack  Num
// display_task  B      3     412    3
// sensor_task   B      4     386    2
// serial_tx     B      5     198    1
// ...
```

---

## 附录 A：原项目 vs 新项目对比

| 维度 | 原项目 | 新项目 |
|------|--------|--------|
| 任务数量 | 11 个（+内部） | 5 个（+内部） |
| 调度方式 | 手动挂起/恢复 | 抢占式调度 |
| UI 架构 | 每菜单一个任务 | 链表 + 单任务状态机 |
| I2C 驱动 | HAL 硬件 I2C + 互斥锁 | I2C 抽象接口 + 软件/硬件可选 |
| 按键处理 | ISR 通知 + 定时器去抖 | 定时器扫描 + 状态机去抖 |
| 日志系统 | 直接 printf（轮询） | 分级日志 + DMA 异步发送 |
| 模块抽象 | 无（直接调用 HAL） | OSAL + I2C_Driver_t 接口 |
| 崩溃调试 | 无 | HardFault 诊断 + FLASH 持久化 |

## 附录 B：关键宏定义速查

```c
// FreeRTOSConfig.h
configTICK_RATE_HZ          1000    // 1ms 每 tick
configMAX_PRIORITIES        10      // 0-9
configTOTAL_HEAP_SIZE       10240   // 10KB 堆
configCHECK_FOR_STACK_OVERFLOW 2    // 栈溢出检测开启
configUSE_RECURSIVE_MUTEXES  1      // 递归互斥锁（优先级继承）

// 任务优先级宏（建议放在 osal.h 或 common 头文件）
#define PRIO_SERIAL     5
#define PRIO_SENSOR     4
#define PRIO_DISPLAY    3
#define PRIO_BATTERY    2
#define PRIO_SYSMON     1

// 任务栈大小宏
#define STACK_SERIAL    256
#define STACK_SENSOR    512
#define STACK_DISPLAY   768
#define STACK_BATTERY   256
#define STACK_SYSMON    256
```

---

> **最后的话**：任务分配没有"绝对正确"的答案。这份指南提供的是一个**经过验证的合理起点**——基于你的硬件资源、FreeRTOS 理论、以及实战踩坑经验。在实现过程中，用 `uxTaskGetStackHighWaterMark` 验证栈大小，用逻辑分析仪或 GPIO 翻转来测量任务执行时间，根据实测数据来微调。这才是从"会用 RTOS"到"理解 RTOS"的关键一步。加油 🚀
