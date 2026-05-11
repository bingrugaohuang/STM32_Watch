#ifndef COMMON_MACRO_H
#define COMMON_MACRO_H

#define COMMON_ERR_OK           0   /* 成功 */
#define COMMON_ERR_PARAM       -1   /* 参数错误（空指针、长度非法等） */
#define COMMON_ERR_MEM         -2   /* 内存不足（分配失败） */
#define COMMON_ERR_QUEUE_FULL  -3   /* 队列/缓冲满，无法投递 */

#endif