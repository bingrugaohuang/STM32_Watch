#ifndef OLED_STRINGS_GBK_H
#define OLED_STRINGS_GBK_H

/* ================================================================
 *  oled_strings_gbk.h — OLED 显示用中文字符串资源声明
 *
 *  对应的字符串定义在 oled_strings_gbk.c 中（GBK 编码文件）。
 *  其他 UTF-8 文件通过 extern 引用这些字符串，避免编码冲突。
 * ================================================================ */

extern const char str_hello[];
extern const char str_oled_driver[];
extern const char str_freertos_watch[];
extern const char str_all_tests_passed[];

#endif /* OLED_STRINGS_GBK_H */
