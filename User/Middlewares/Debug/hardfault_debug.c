#include "hardfault_debug.h"
#include "serial.h"
#include "stm32f1xx_hal.h"

/* ================================================================
 *  SCB 故障寄存器地址 (Cortex-M3)
 * ================================================================ */
#define SCB_CFSR    (*(volatile uint32_t *)0xE000ED28U)
#define SCB_HFSR    (*(volatile uint32_t *)0xE000ED2CU)
#define SCB_MMFAR   (*(volatile uint32_t *)0xE000ED34U)
#define SCB_BFAR    (*(volatile uint32_t *)0xE000ED38U)
#define SCB_AIRCR   (*(volatile uint32_t *)0xE000ED0CU)

/* CFSR 子字段掩码 */
#define CFSR_MMFSR_MASK   0x000000FFU
#define CFSR_BFSR_MASK    0x0000FF00U
#define CFSR_UFSR_MASK    0xFFFF0000U

/* ---- FLASH 崩溃日志相关 ---- */
#define CRASH_LOG_ADDR      0x0800FC00U // STM32F103C8T6 的最后 1KB 用于保存崩溃日志
#define CRASH_LOG_PAGE_SIZE 1024U
#define CRASH_MAGIC         0xDEADBEEFU // 用于标识有效的崩溃数据，避免误读垃圾数据，占用4字节
#define CRASH_DATA_MAX      (CRASH_LOG_PAGE_SIZE - 8U)

/* ---- FLASH 写入进度跟踪（调试用） ---- */
#if HARDFAULT_CFG_FLASH_DEBUG_TRACE
#define HFDBG_CHAR(c) do { \
    while (!(USART1->SR & USART_SR_TXE)) {} \
    USART1->DR = (uint8_t)(c); \
} while(0)

/* 输出 FLASH_SR 低 8 位的 hex 错误码 */
static void hfdbg_sr_err(void)
{
    uint8_t sr = (uint8_t)(FLASH->SR & 0xFFU);
    uint8_t hi = (sr >> 4) & 0x0FU;
    uint8_t lo = sr & 0x0FU;
    HFDBG_CHAR('X');
    HFDBG_CHAR((hi < 10) ? ('0' + hi) : ('A' + hi - 10));
    HFDBG_CHAR((lo < 10) ? ('0' + lo) : ('A' + lo - 10));
}
#else
#define HFDBG_CHAR(c)    ((void)0)
#define hfdbg_sr_err()   ((void)0)
#endif

/* ---- 前向声明 ---- */
static void dump_line(const char *s);
static void dump_hex_word(const char *label, uint32_t val);
static void dump_registers(const uint32_t *sf);
static void dump_fault_status(void);
static void dump_stack_region(const uint32_t *sf);

#if HARDFAULT_CFG_SERIAL_ENABLE
static void init_usart_for_hardfault(void);
#endif

#if HARDFAULT_CFG_FLASH_SAVE
static void write_crash_to_flash(void);
static void write_crash_to_flash_simple(void);
static char          crash_buffer[CRASH_DATA_MAX];
static uint32_t      crash_buf_idx;
#endif

/* ---- 内部辅助：十六进制字符串构造 ---- */
static char nibble_to_hex(uint8_t n)
{
    return (n < 10) ? (char)('0' + n) : (char)('A' + n - 10);
}

static void uint32_to_hex_str(uint32_t val, char *buf)
{
    int i;
    for (i = 7; i >= 0; i--) {
        buf[7 - i] = nibble_to_hex((uint8_t)((val >> (i * 4)) & 0x0FU));
    }
    buf[8] = '\0';
}

/* ---- 原始串口输出辅助（同时可选复制到 FLASH 缓冲区） ---- */
static void send_str(const char *s)
{
    while (*s) {
#if HARDFAULT_CFG_SERIAL_ENABLE
        while (!(USART1->SR & USART_SR_TXE)) {}
        USART1->DR = (uint8_t)(*s);
#endif
#if HARDFAULT_CFG_FLASH_SAVE
        if (crash_buf_idx < CRASH_DATA_MAX - 1U) {
            crash_buffer[crash_buf_idx++] = *s;
        }
#endif
        s++;
    }
#if HARDFAULT_CFG_FLASH_SAVE
    if (crash_buf_idx < CRASH_DATA_MAX) {
        crash_buffer[crash_buf_idx] = '\0';
    }
#endif
}

static void send_strln(const char *s)
{
    send_str(s);
    send_str("\r\n");
}

static void dump_line(const char *s)
{
    send_strln(s);
}

static void dump_hex_word(const char *label, uint32_t val)
{
    char buf[9];
    uint32_to_hex_str(val, buf);
    send_str(label);
    send_str("0x");
    send_strln(buf);
}

/* ================================================================
 *  decode_fault_reason — 将 CFSR/HFSR 解码为可读字符串并输出
 * ================================================================ */
static void decode_fault_reason(void)
{
    uint32_t cfsr = SCB_CFSR;
    uint32_t hfsr = SCB_HFSR;
    uint8_t  mmfsr = (uint8_t)(cfsr & CFSR_MMFSR_MASK);
    uint8_t  bfsr  = (uint8_t)((cfsr & CFSR_BFSR_MASK) >> 8);
    uint16_t ufsr  = (uint16_t)((cfsr & CFSR_UFSR_MASK) >> 16);

    dump_line("--- Fault Reason ---");

    /* ---- HardFault 状态 ---- */
    if (hfsr & (1U << 30)) dump_line("  HFSR: FORCED  (fault escalated to HardFault)");
    if (hfsr & (1U << 1))  dump_line("  HFSR: VECTTBL (vector table read error)");
    if (hfsr & (1U << 31)) dump_line("  HFSR: DEBUGEVT (debug event)");

    /* ---- MemManage 故障 ---- */
    if (mmfsr) {
        if (mmfsr & (1U << 7)) dump_line("  MMFSR: MMARVALID  (MMFAR holds valid address)");
        if (mmfsr & (1U << 4)) dump_line("  MMFSR: MSTKERR    (stacking from exception failed)");
        if (mmfsr & (1U << 3)) dump_line("  MMFSR: MUNSTKERR  (unstacking from exception failed)");
        if (mmfsr & (1U << 1)) dump_line("  MMFSR: DACCVIOL   (data access violation)");
        if (mmfsr & (1U << 0)) dump_line("  MMFSR: IACCVIOL   (instruction access violation)");
    }

    /* ---- BusFault ---- */
    if (bfsr) {
        if (bfsr & (1U << 7)) dump_line("  BFSR:  BFARVALID  (BFAR holds valid address)");
        if (bfsr & (1U << 4)) dump_line("  BFSR:  STKERR     (stacking from exception failed)");
        if (bfsr & (1U << 3)) dump_line("  BFSR:  UNSTKERR   (unstacking from exception failed)");
        if (bfsr & (1U << 2)) dump_line("  BFSR:  IMPRECISERR (imprecise data bus error)");
        if (bfsr & (1U << 1)) dump_line("  BFSR:  PRECISERR  (precise data bus error)");
        if (bfsr & (1U << 0)) dump_line("  BFSR:  IBUSERR    (instruction bus error)");
    }

    /* ---- UsageFault ---- */
    if (ufsr) {
        if (ufsr & (1U << 9)) dump_line("  UFSR:  DIVBYZERO  (divide by zero)");
        if (ufsr & (1U << 8)) dump_line("  UFSR:  UNALIGNED  (unaligned memory access)");
        if (ufsr & (1U << 3)) dump_line("  UFSR:  NOCP       (no coprocessor)");
        if (ufsr & (1U << 2)) dump_line("  UFSR:  INVPC      (invalid PC loaded)");
        if (ufsr & (1U << 1)) dump_line("  UFSR:  INVSTATE   (invalid instruction state)");
        if (ufsr & (1U << 0)) dump_line("  UFSR:  UNDEFINSTR (undefined instruction)");
    }

    if (!mmfsr && !bfsr && !ufsr && !(hfsr & ~(1U << 31))) {
        dump_line("  (no configurable fault flags set)");
    }

    /* 故障地址 */
    if (mmfsr & (1U << 7)) dump_hex_word("  MMFAR = ", SCB_MMFAR);
    if (bfsr  & (1U << 7)) dump_hex_word("  BFAR  = ", SCB_BFAR);
}

/* ================================================================
 *  dump_registers — 输出异常帧中的所有寄存器
 * ================================================================ */
static void dump_registers(const uint32_t *sf)
{
    dump_line("--- Exception Stack Frame ---");
    dump_hex_word("  R0  = ", sf[0]);
    dump_hex_word("  R1  = ", sf[1]);
    dump_hex_word("  R2  = ", sf[2]);
    dump_hex_word("  R3  = ", sf[3]);
    dump_hex_word("  R12 = ", sf[4]);
    dump_hex_word("  LR  = ", sf[5]);
    dump_hex_word("  PC  = ", sf[6]);
    dump_hex_word("  xPSR= ", sf[7]);
}

/* ================================================================
 *  dump_stack_region — 输出栈附近区域，用于手动回溯
 * ================================================================ */
static void dump_stack_region(const uint32_t *sf)
{
    const uint32_t *base = sf - 16;
    const uint32_t *end  = sf + 24;
    const uint32_t *p;

    dump_line("--- Stack Dump (32 words around exception frame) ---");
    for (p = base; p < end; p += 4) {
        char buf[64];
        uint32_t a0 = p[0], a1 = p[1], a2 = p[2], a3 = p[3];
        char h0[9], h1[9], h2[9], h3[9];
        uint32_to_hex_str(a0, h0);
        uint32_to_hex_str(a1, h1);
        uint32_to_hex_str(a2, h2);
        uint32_to_hex_str(a3, h3);

        uint32_t addr = (uint32_t)p;
        char addr_str[9];
        uint32_to_hex_str(addr, addr_str);

        send_str("[0x");
        send_str(addr_str);
        send_str("]  ");
        send_str(h0);
        send_str("  ");
        send_str(h1);
        send_str("  ");
        send_str(h2);
        send_str("  ");
        send_str(h3);
        send_str("\r\n");
    }
}

#if HARDFAULT_CFG_SERIAL_ENABLE
/* ================================================================
 *  init_usart_for_hardfault — HardFault 发生时 USART1 最低限度初始化
 * ================================================================ */
static void init_usart_for_hardfault(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN | RCC_APB2ENR_IOPAEN;

    GPIOA->CRH = (GPIOA->CRH & 0xFFFFFF0F) | 0x000000B0;

    if ((USART1->CR1 & USART_CR1_UE) == 0) {
        USART1->BRR = 0x45;
        USART1->CR1 = USART_CR1_UE | USART_CR1_TE;
    }
}
#endif /* HARDFAULT_CFG_SERIAL_ENABLE */

#if HARDFAULT_CFG_FLASH_SAVE
/* ================================================================
 *  write_crash_to_flash — 纯裸机操作将 crash_buffer 写入 FLASH
 *
 *  注意：此函数在 HardFault 上下文中被调用，绝不能依赖 HAL 库或
 *        SysTick。所有超时均使用轮询 BSY 标志的方式。
 * ================================================================ */
static void write_crash_to_flash(void)
{
    uint32_t data_len = crash_buf_idx;
    uint32_t total_halfwords;
    uint32_t i;
    uint16_t *src;
    volatile uint16_t *dst;

    HFDBG_CHAR('A');  /* 入口 */

    if (data_len == 0) {
        HFDBG_CHAR('Z');  /* 无数据，跳过 */
        return;
    }

    /* 1. 确保 FLASH CR 无残留编程/擦除模式 */
    if (FLASH->CR & (FLASH_CR_PG | FLASH_CR_PER)) {
        FLASH->CR &= ~(FLASH_CR_PG | FLASH_CR_PER);
    }

    /* 2. 解锁 FLASH（仅当确实被锁定时才写解锁序列） */
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = 0x45670123;
        FLASH->KEYR = 0xCDEF89AB;
    }

    /* 3. 等待 FLASH 空闲，并清除任何残留错误标志 */
    while ((FLASH->SR & FLASH_SR_BSY) != 0U) {}
    FLASH->SR = (FLASH_SR_PGERR | FLASH_SR_WRPRTERR);

    HFDBG_CHAR('B');  /* 解锁完成 */

    /* 4. 擦除目标页 */
    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR  = CRASH_LOG_ADDR;
    FLASH->CR |= FLASH_CR_STRT;
    while ((FLASH->SR & FLASH_SR_BSY) != 0U) {}
    FLASH->CR &= ~FLASH_CR_PER;

    /* 检查擦除是否成功 */
    if (FLASH->SR & (FLASH_SR_PGERR | FLASH_SR_WRPRTERR)) {
        hfdbg_sr_err();  /* 擦除失败 */
        FLASH->SR = (FLASH_SR_PGERR | FLASH_SR_WRPRTERR);
        FLASH->CR |= FLASH_CR_LOCK;
        return;
    }

    HFDBG_CHAR('C');  /* 擦除完成 */

    /* 5. 开启编程模式 */
    FLASH->CR |= FLASH_CR_PG;

    /* 6. 写入魔数 (4 字节，半字编程分两步) */
    dst = (volatile uint16_t *)(CRASH_LOG_ADDR);
    *dst = (uint16_t)(CRASH_MAGIC & 0xFFFFU);
    while ((FLASH->SR & FLASH_SR_BSY) != 0U) {}
    dst++;
    *dst = (uint16_t)((CRASH_MAGIC >> 16) & 0xFFFFU);
    while ((FLASH->SR & FLASH_SR_BSY) != 0U) {}
    dst++;

    /* 7. 写入数据长度 (4 字节) */
    *dst = (uint16_t)(data_len & 0xFFFFU);
    while ((FLASH->SR & FLASH_SR_BSY) != 0U) {}
    dst++;
    *dst = (uint16_t)((data_len >> 16) & 0xFFFFU);
    while ((FLASH->SR & FLASH_SR_BSY) != 0U) {}
    dst++;

    /* 检查头部编程是否有错误 */
    if (FLASH->SR & (FLASH_SR_PGERR | FLASH_SR_WRPRTERR)) {
        hfdbg_sr_err();  /* 头部写入失败 */
        FLASH->SR = (FLASH_SR_PGERR | FLASH_SR_WRPRTERR);
        FLASH->CR &= ~FLASH_CR_PG;
        FLASH->CR |= FLASH_CR_LOCK;
        return;
    }

    HFDBG_CHAR('D');  /* 头部写入完成 */

    /* 8. 写入崩溃数据（按半字编程） */
    src = (uint16_t *)crash_buffer;
    total_halfwords = (data_len + 1U) / 2U;
    for (i = 0; i < total_halfwords; i++) {
        *dst = src[i];
        while ((FLASH->SR & FLASH_SR_BSY) != 0U) {}
        dst++;
    }

    /* 检查数据写入是否有错误 */
    if (FLASH->SR & (FLASH_SR_PGERR | FLASH_SR_WRPRTERR)) {
        hfdbg_sr_err();  /* 数据写入失败 */
        FLASH->SR = (FLASH_SR_PGERR | FLASH_SR_WRPRTERR);
        FLASH->CR &= ~FLASH_CR_PG;
        FLASH->CR |= FLASH_CR_LOCK;
        return;
    }

    HFDBG_CHAR('E');  /* 数据写入完成 */

    /* 9. 关闭编程模式 */
    FLASH->CR &= ~FLASH_CR_PG;

    /* 10. 写后验证：读回魔数确认写入真正生效 */
    if (*(volatile uint32_t *)CRASH_LOG_ADDR != CRASH_MAGIC) {
        HFDBG_CHAR('V');  /* 验证失败 */
        FLASH->CR |= FLASH_CR_LOCK;
        return;
    }

    HFDBG_CHAR('F');  /* 验证通过，写入成功 */

    /* 11. 锁定 FLASH */
    FLASH->CR |= FLASH_CR_LOCK;
}

/* ================================================================
 *  write_crash_to_flash_simple — 简化版 FLASH 写入（无错误检查 + 重试）
 *
 *  适用于带错误检查版间歇性失败的场景，作为备选方案。
 *  写入失败时最多重试 2 次（共 3 次写入机会）。
 * ================================================================ */
static void write_crash_to_flash_simple(void)
{
    uint32_t data_len = crash_buf_idx;
    uint32_t total_halfwords;
    uint32_t attempt;
    uint32_t i;
    uint16_t *src;
    volatile uint16_t *dst;

    HFDBG_CHAR('a');  /* 简化版入口 */

    if (data_len == 0) {
        return;
    }

    for (attempt = 0; attempt < 3; attempt++) {
        /* 1. 解锁 FLASH */
        FLASH->KEYR = 0x45670123;
        FLASH->KEYR = 0xCDEF89AB;

        /* 2. 等待空闲 */
        while ((FLASH->SR & FLASH_SR_BSY) != 0U) {}

        /* 3. 擦除目标页 */
        FLASH->CR |= FLASH_CR_PER;
        FLASH->AR  = CRASH_LOG_ADDR;
        FLASH->CR |= FLASH_CR_STRT;
        while ((FLASH->SR & FLASH_SR_BSY) != 0U) {}
        FLASH->CR &= ~FLASH_CR_PER;

        /* 4. 开启编程模式 */
        FLASH->CR |= FLASH_CR_PG;

        /* 5. 写入魔数 */
        dst = (volatile uint16_t *)(CRASH_LOG_ADDR);
        *dst = (uint16_t)(CRASH_MAGIC & 0xFFFFU);
        while ((FLASH->SR & FLASH_SR_BSY) != 0U) {}
        dst++;
        *dst = (uint16_t)((CRASH_MAGIC >> 16) & 0xFFFFU);
        while ((FLASH->SR & FLASH_SR_BSY) != 0U) {}
        dst++;

        /* 6. 写入数据长度 */
        *dst = (uint16_t)(data_len & 0xFFFFU);
        while ((FLASH->SR & FLASH_SR_BSY) != 0U) {}
        dst++;
        *dst = (uint16_t)((data_len >> 16) & 0xFFFFU);
        while ((FLASH->SR & FLASH_SR_BSY) != 0U) {}
        dst++;

        /* 7. 写入崩溃数据 */
        src = (uint16_t *)crash_buffer;
        total_halfwords = (data_len + 1U) / 2U;
        for (i = 0; i < total_halfwords; i++) {
            *dst = src[i];
            while ((FLASH->SR & FLASH_SR_BSY) != 0U) {}
            dst++;
        }

        /* 8. 关闭编程模式 */
        FLASH->CR &= ~FLASH_CR_PG;

        /* 9. 写后验证 */
        if (*(volatile uint32_t *)CRASH_LOG_ADDR == CRASH_MAGIC) {
            HFDBG_CHAR('f');  /* 写入成功 */
            FLASH->CR |= FLASH_CR_LOCK;
            return;
        }

        /* 验证失败，重试 */
        HFDBG_CHAR('r');  /* retry marker */
        FLASH->CR |= FLASH_CR_LOCK;
    }

    HFDBG_CHAR('v');  /* 3 次重试全部失败 */
    FLASH->CR |= FLASH_CR_LOCK;
}
#endif /* HARDFAULT_CFG_FLASH_SAVE */

/* ================================================================
 *  hardfault_debug_init — 在系统启动时调用，配置相关寄存器
 * ================================================================ */
void hardfault_debug_init(void)
{
    /* 开启 UsageFault, BusFault, MemManageFault */
    SCB->SHCSR |= (1 << 18) | (1 << 17) | (1 << 16);

    /* 特别注意：Cortex-M3 默认除以 0 是不会报错的！会直接返回 0。
    如果你希望除以 0 时触发 UsageFault 并被你的代码捕获，需要额外开启这个位 */
    SCB->CCR |= (1 << 4); // 开启 DIV_0_TRP
}

/* ================================================================
 *  hardfault_dump — 主诊断函数，由 HardFault_Handler 调用
 * ================================================================ */
void hardfault_dump(uint32_t *stack_frame)
{
    __disable_irq();

#if HARDFAULT_CFG_FLASH_SAVE
    crash_buf_idx = 0;
    crash_buffer[0] = '\0';
#endif

#if HARDFAULT_CFG_SERIAL_ENABLE
    init_usart_for_hardfault();
#endif

    dump_line("");
    dump_line("================================================");
    dump_line("        HARDFAULT CRASH DUMP");
    dump_line("================================================");
    dump_line("");

    /* 1. 异常栈帧寄存器 */
    dump_registers(stack_frame);

    /* 2. 故障状态寄存器解码 */
    dump_line("");
    dump_hex_word("CFSR = ", SCB_CFSR);
    dump_hex_word("HFSR = ", SCB_HFSR);
    decode_fault_reason();

    /* 3. 栈区域转储（含调用链信息） */
    dump_line("");
    dump_stack_region(stack_frame);

    dump_line("");
    dump_line("================================================");
    dump_line("        END OF CRASH DUMP");
    dump_line("================================================");

#if HARDFAULT_CFG_FLASH_SAVE
    /* 调试模式：输出空行分隔 FLASH 写入进度标记 */
    dump_line("");
#if HARDFAULT_CFG_FLASH_MODE
    write_crash_to_flash();
#else
    write_crash_to_flash_simple();
#endif
    dump_line("");
#endif

    /* 死循环 —— 等待用户手动复位，留时间连接串口 */
    while (1) {}
}

/* ================================================================
 *  check_crash_log_on_startup — 启动时检查 FLASH 中是否有崩溃日志
 *
 *  注意：此函数在正常启动上下文中调用，此时 SysTick 已运行，
 *        但为了保持自包含，仍使用裸机寄存器操作。
 * ================================================================ */
void check_crash_log_on_startup(void)
{
    uint32_t          magic;
    uint32_t          data_len;
    const char       *saved_log;

    /* 读取魔数 */
    magic = *(volatile uint32_t *)CRASH_LOG_ADDR;

    /* 检查是否为有效的崩溃数据 */
    if (magic != CRASH_MAGIC) {
#if HARDFAULT_CFG_DIAGNOSTIC
        /* 诊断模式：输出 FLASH 页前 16 字节供调试 */
        char diag_buf[64];
        uint8_t i;
        const volatile uint8_t *p = (const volatile uint8_t *)CRASH_LOG_ADDR;
        serial_send_blocking((const uint8_t *)"\r\n[HFDBG] No crash log, FLASH dump: ", 38);
        for (i = 0; i < 16; i++) {
            uint8_t hi = (p[i] >> 4) & 0x0FU;
            uint8_t lo = p[i] & 0x0FU;
            diag_buf[i * 3]     = (hi < 10) ? ('0' + hi) : ('A' + hi - 10);
            diag_buf[i * 3 + 1] = (lo < 10) ? ('0' + lo) : ('A' + lo - 10);
            diag_buf[i * 3 + 2] = ' ';
        }
        diag_buf[47] = '\r';
        diag_buf[48] = '\n';
        serial_send_blocking((const uint8_t *)diag_buf, 49);
#endif
        return;
    }

    /* 读取数据长度 */
    data_len = *(volatile uint32_t *)(CRASH_LOG_ADDR + 4U);
    saved_log = (const char *)(CRASH_LOG_ADDR + 8U);


    /* 长度安全检查 */
    if (data_len > CRASH_DATA_MAX) {
        data_len = CRASH_DATA_MAX;
    }

    /* 通过串口输出保存的崩溃日志 */
    serial_send_blocking((const uint8_t *)"\r\n", 2);
    serial_send_blocking((const uint8_t *)"=== PREVIOUS CRASH DUMP (from FLASH) ===\r\n", 43);
    serial_send_blocking((const uint8_t *)saved_log, data_len);
    serial_send_blocking((const uint8_t *)"\r\n=== END OF CRASH DUMP ===\r\n\r\n", 34);

    /* 擦除该 FLASH 页，清除崩溃标记（防止每次启动重复打印） */
    while ((FLASH->SR & FLASH_SR_BSY) != 0U) {}
    FLASH->KEYR = 0x45670123;   // 秘钥1
    FLASH->KEYR = 0xCDEF89AB;   // 秘钥2
    while ((FLASH->SR & FLASH_SR_BSY) != 0U) {}
    FLASH->CR |= FLASH_CR_PER;  // 设置页擦除
    FLASH->AR  = CRASH_LOG_ADDR;// 设置要擦除的页地址
    FLASH->CR |= FLASH_CR_STRT; // 启动擦除
    while ((FLASH->SR & FLASH_SR_BSY) != 0U) {}
    FLASH->CR &= ~FLASH_CR_PER; // 清除页擦除位
    FLASH->CR |= FLASH_CR_LOCK; // 重新锁定 FLASH
}
