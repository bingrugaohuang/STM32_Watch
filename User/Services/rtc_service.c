#include "rtc_service.h"
#include "rtc.h"
#include <stdio.h> /* 用于 sprintf */

static char s_time_buf[9];  /* 用于存储格式化后的时间字符串 */
static char s_date_buf[11]; /* 用于存储格式化后的日期字符串 */
static RTCTime_t s_time;    /* 用于存储当前时间 */
static RTCDate_t s_date;    /* 用于存储当前日期 */

static void rtc_service_refresh(void)
{
    /* 获取当前时间和日期 */
    RTC_TimeTypeDef ht;
    RTC_DateTypeDef hd;
    HAL_RTC_GetTime(&hrtc, &ht, RTC_FORMAT_BIN);//顺序不能颠倒，必须先获取时间再获取日期，否则日期可能会不正确（因为读取时间可能会导致日期寄存器更新）
    HAL_RTC_GetDate(&hrtc, &hd, RTC_FORMAT_BIN);

    /* 更新全局时间和日期变量的结构体封装 */
    s_time = (RTCTime_t){.hours = ht.Hours, .minutes = ht.Minutes, .seconds = ht.Seconds};
    s_date = (RTCDate_t){.week_day = hd.WeekDay, .month = hd.Month, .date = hd.Date, .year = hd.Year};    

    static uint8_t s_last_bkp_day = 0xFF; //上次备份的日期，初始值为0xFF表示未初始化
    if(s_date.date != s_last_bkp_day){
        s_last_bkp_day = s_date.date;
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, 
            (((uint16_t)s_date.year) << 8) | s_date.month); //将年月写入备份寄存器，方便后续读取
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR3, 
            (((uint16_t)s_date.date) << 8) | s_date.week_day); //将日和星期写入备份寄存器，方便后续读取
    }

    /* 格式化时间字符串 HH:MM:SS */
    snprintf(s_time_buf, sizeof(s_time_buf),"%02d:%02d:%02d", s_time.hours, s_time.minutes, s_time.seconds);
    /* 格式化日期字符串 DD/MM/YY */
    snprintf(s_date_buf, sizeof(s_date_buf),"%02d/%02d/%02d", s_date.date, s_date.month, s_date.year);
}

/* 获取当前时间字符串 */
const char* rtc_service_get_time_str(void)
{
    rtc_service_refresh(); /* 刷新时间和日期 */
    return s_time_buf; /* 返回格式化后的时间字符串 */
}

/* 获取当前日期字符串 */
const char* rtc_service_get_date_str(void)
{
    return s_date_buf; /* 返回格式化后的日期字符串 */
}

/* 获取当前时间结构体 */
RTCTime_t rtc_service_get_time_struct(void)
{
    rtc_service_refresh(); /* 刷新时间和日期 */
    return s_time; /* 返回当前时间的结构体封装 */
}

/* 获取当前日期结构体 */
RTCDate_t rtc_service_get_date_struct(void)
{
    return s_date; /* 返回当前日期的结构体封装 */
}

/* 设置当前时间和日期 */
void rtc_service_set_time(const RTCTime_t *time)
{
    RTC_TimeTypeDef ht = {
        .Hours = time->hours,
        .Minutes = time->minutes,
        .Seconds = time->seconds
    };
    HAL_RTC_SetTime(&hrtc, &ht, RTC_FORMAT_BIN);
}

/* 设置当前日期 */
void rtc_service_set_date(const RTCDate_t *date)
{
    RTC_DateTypeDef hd = {
        .WeekDay = date->week_day,
        .Month = date->month,
        .Date = date->date,
        .Year = date->year
    };
    HAL_RTC_SetDate(&hrtc, &hd, RTC_FORMAT_BIN);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, 
        (((uint16_t)date->year) << 8) | date->month); //将年月写入备份寄存器，方便后续读取
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR3, 
        (((uint16_t)date->date) << 8) | date->week_day); //将日和星期写入备份寄存器，方便后续读取
}
