#include "modules/TranAnime.h"

//转场动画
void TranAnime(uint8_t dir)
{
	uint8_t offset;

	for (offset = 0; offset <= 128; offset += TRANSITION_STEP)
	{
		if (dir == TRAN_DIR_LEFT)
		{
			/* 进入应用：画面左移，右侧空白逐步扩大 */
			OLED_ClearArea(128 - offset, 0, offset, 64);
		}
		else
		{
			/* 退出设置：画面右移，左侧空白逐步扩大 */
			OLED_ClearArea(0, 0, offset, 64);
		}
		OLED_Update();
		vTaskDelay(pdMS_TO_TICKS(TRANSITION_DELAY_MS));
	}
}
