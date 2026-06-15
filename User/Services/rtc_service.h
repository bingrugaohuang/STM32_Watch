#ifndef RTC_SERVICE_H
#define RTC_SERVICE_H

#include <stdint.h>

/* 时间结构体，包含时、分、秒 */
typedef struct {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
} RTCTime_t;

/* 日期结构体，包含年、月、日和星期 */
typedef struct {
    uint8_t year;
    uint8_t month;
    uint8_t date;
    uint8_t week_day;
} RTCDate_t;

/* 获取当前时间字符串，格式为 "HH:MM:SS" */
const char* rtc_service_get_time_str(void);
/* 获取当前日期字符串，格式为 "DD/MM/YY" */
const char* rtc_service_get_date_str(void);

/* 获取当前时间和日期的结构体封装 */
RTCTime_t rtc_service_get_time_struct(void);
RTCDate_t rtc_service_get_date_struct(void);

/* 设置当前时间和日期 */
void rtc_service_set_time(const RTCTime_t *time);
void rtc_service_set_date(const RTCDate_t *date);

#endif /* RTC_SERVICE_H */