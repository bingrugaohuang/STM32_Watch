#include "modules/Set_StackMonitor.h"

//根据Cursor判断是挂起还是恢复栈监控任务
static void JugdeStackMonitorState(int8_t Cursor)
{
    if(Cursor == 0){
        if(StackMonitorTaskHandle != NULL && eTaskGetState(StackMonitorTaskHandle) != eSuspended)
        {
            vTaskSuspend(StackMonitorTaskHandle);//挂起栈监控任务，退出显示栈高水位界面
        }
    }else if(Cursor == 1){
        if(StackMonitorTaskHandle != NULL && eTaskGetState(StackMonitorTaskHandle) == eSuspended)
        {
            vTaskResume(StackMonitorTaskHandle);//恢复栈监控任务，退出显示栈高水位界面
        }
    }
}

//根据Cursor显示不同的反色内容
static void ShowReverse(int8_t Cursor)
{
    switch (Cursor)
    {
        case 0:
            OLED_ShowString(104, 48, "OFF", OLED_8X16);
            OLED_ReverseArea(104, 48, 24, 16);
            break;
        case 1:
            OLED_ShowString(112, 48, "ON", OLED_8X16);
            OLED_ReverseArea(112, 48, 16, 16);
            break;
        default:
            break;
    }
}

//设置栈高水位界面
uint8_t Menu_Third_ShowStackWatermark(void)
{
	static int8_t Cursor = 0;
	OLED_ReverseArea(16, 32, 96, 16);
	while(1)
	{
		uint8_t Key = Key_GetNum();
		if(Key == KEY_CONFIRM){
			JugdeStackMonitorState(Cursor);
			return 0;
		}else if(Key == KEY_NEXT){
			Cursor = (Cursor + 1) % 2;
		}else if(Key == KEY_LAST){
			Cursor = (Cursor - 1 + 2) % 2;
		}

		OLED_ClearArea(0, 48, 128, 16);//清除选项下方
		
		ShowFrames();
		ShowReverse(Cursor);

		OLED_Update();
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}
