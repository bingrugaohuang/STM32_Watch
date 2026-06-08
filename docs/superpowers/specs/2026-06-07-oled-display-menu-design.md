# OLED 驱动同步 & 显示任务 + 多级菜单架构设计

> 日期：2026-06-07
> 状态：待评审

---

## 一、背景与问题

本项目（STM32F103C8T6 + FreeRTOS）的 OLED 驱动层目前使用**软件 I2C 位打**（`bsp_i2c1_sw.c`），该实现是同步阻塞的——`write()` 返回时数据已发送完毕，不存在缓冲区生命周期问题。

即将引入**硬件 I2C + DMA**（`bsp_i2c1_hw.c`），`write()` 只负责启动 DMA 就立即返回。此时存在两个层次的竞态问题：

| 层次 | 竞态描述 |
|------|---------|
| **I2C 写操作级** | `WriteCmd(&cmd)` 中的 `cmd` 是局部变量，函数返回后栈回收，DMA 还在读 |
| **帧缓冲级** | `OLED_Update()` 发送 DisplayBuf 期间，其他代码修改 DisplayBuf → 发送脏数据 |

同时，上层**显示任务 + 多级链式菜单**的架构尚未设计，用户对以下问题缺乏清晰思路：

- `display_task` 的任务函数长什么样？
- 队列收到的消息如何与当前菜单状态协调？
- 如何减少各层之间的耦合？

---

## 二、整体分层架构

```
┌──────────────────────────────────────────────────┐
│  应用层 (Application)                             │
│  ┌─────────┐  ┌──────────┐  ┌─────────────────┐  │
│  │ 手表主屏 │  │ 设置菜单 │  │ 传感器数据显示   │  │
│  │ on_render│  │ on_render│  │ on_render       │  │
│  └────┬─────┘  └────┬─────┘  └───────┬─────────┘  │
│       └──────────────┼───────────────┘             │
│                      │ 调用 OLED_xxx() 绘图函数     │
├──────────────────────┼─────────────────────────────┤
│  服务层 (Service)    │                             │
│  ┌───────────────────┴──────────────────────────┐  │
│  │  menu_engine.c/h  菜单引擎                    │  │
│  │    - MenuNode 链表遍历                        │  │
│  │    - 状态机（UP/DOWN/CONFIRM/BACK）            │  │
│  │    - 事件 → 当前菜单分发                       │  │
│  └──────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────┐  │
│  │  oled_service.c/h  OLED 帧同步服务            │  │
│  │    - OLED_UpdateSync()  带信号量的全屏刷新     │  │
│  │    - OLED_UpdateAreaSync()  带信号量的局部刷新 │  │
│  └──────────────────┬───────────────────────────┘  │
├─────────────────────┼──────────────────────────────┤
│  驱动层 (Drivers)    │                              │
│  ┌──────────────────┴───────────────────────────┐  │
│  │  oled_driver.c/h  OLED 驱动                   │  │
│  │    - OLED_ShowString / OLED_DrawPoint / ...   │  │
│  │    - OLED_WriteCmd()  ←── DMA 信号量同步      │  │
│  │    - OLED_WriteData() ←── DMA 信号量同步      │  │
│  │    - OLED_DisplayBuf[8][128]  全局帧缓冲       │  │
│  └──────────────────┬───────────────────────────┘  │
│  ┌──────────────────┴───────────────────────────┐  │
│  │  i2c_interface.h  I2C 抽象接口 (I2C_Driver_t) │  │
│  │  bsp_i2c1_sw.c  /  bsp_i2c1_hw.c              │  │
│  └──────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────┘
```

**核心设计原则：**

1. **驱动层不与 RTOS 耦合** — 信号量由服务层创建，DMA 回调在驱动层注册服务层传入的回调函数
2. **服务层封装同步逻辑** — `OLED_UpdateSync()` 内部调用驱动层的 `OLED_Update()`，前后加帧级保护
3. **菜单引擎独立于显示内容** — 只管理链表遍历和事件路由，不关心渲染细节

---

## 三、Layer 1：I2C DMA 同步（驱动层内部）

### 3.1 信号量与 DMA 回调

```
启动DMA → WriteCmd/WriteData等待信号量 → DMA搬运中...
                                            ↓
                                    DMA完成中断
                                        ↓
                                    I2C回调: Give信号量
                                        ↓
                                    下次WriteCmd/WriteData拿到信号量 → 安全覆盖缓冲区
```

### 3.2 实现代码（在 `oled_driver.c` 中）

```c
/* ========== oled_driver.c ========== */

static SemaphoreHandle_t i2c_done_sem = NULL;

/* DMA 传输完成回调 — 由 I2C BSP 层在 DMA ISR 中调用 */
void OLED_I2C_DMA_DoneCallback(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(i2c_done_sem, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void OLED_Init(void)
{
    i2c_done_sem = xSemaphoreCreateBinary();
    I2C_Driver_t *drv = (I2C_Driver_t *)I2C1_SW_GetDriver();
    drv->init();

    /* 向 BSP 层注册 DMA 完成回调 */
    I2C1_RegisterDMACallback(OLED_I2C_DMA_DoneCallback);

    /* 初始给一次，首次写无需等待 */
    xSemaphoreGive(i2c_done_sem);

    /* ... OLED 初始化命令序列使用同步写入 ... */
}

/* ===== 关键：同步的 I2C 写函数 ===== */

static void OLED_WriteCmd(uint8_t cmd)
{
    static uint8_t cmd_buf;  /* 静态承接局部变量，DMA 安全 */

    xSemaphoreTake(i2c_done_sem, portMAX_DELAY);
    cmd_buf = cmd;
    i2c_drv->write(OLED_I2C_ADDR, OLED_CTRL_CMD, &cmd_buf, 1);
    /* DMA 回调中 Give，不在此处 Give */
}

static void OLED_WriteData(uint8_t *data, uint8_t len)
{
    xSemaphoreTake(i2c_done_sem, portMAX_DELAY);
    i2c_drv->write(OLED_I2C_ADDR, OLED_CTRL_DATA, data, len);
    /* data 指向 OLED_DisplayBuf[]，全局数组，DMA 安全 */
}
```

### 3.3 I2C 驱动层需要的改动

在 `i2c_interface.h` 的 `I2C_Driver_t` 中新增一个回调注册函数指针：

```c
typedef struct {
    void     (*init)(void);
    uint8_t  (*write)(uint8_t dev_addr, uint8_t ctrl, uint8_t *data, uint8_t len);
    uint8_t  (*read)(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint8_t len);
    void     (*delay_us)(uint32_t us);
    void     (*register_dma_cb)(void (*cb)(void));  /* 新增：注册DMA完成回调 */
} I2C_Driver_t;
```

对于软件 I2C（`bsp_i2c1_sw.c`），该函数为空实现（软件 I2C 同步阻塞，不需要回调）。

---

## 四、Layer 2：OLED 帧同步服务（服务层）

### 4.1 设计动机

`OLED_Update()` 内部串行调用 32 次 `WriteCmd`/`WriteData`（8 页 × 4 次），每次都等信号量。这保证了**I2C 写操作级**的安全。

但如果在 `OLED_Update()` 执行期间（即两次 WriteCmd 之间），其他任务或中断调用了 `OLED_ShowString()` 等绘图函数修改了 DisplayBuf，就会造成部分页是旧内容、部分页是新内容的撕裂现象。

**解决方案**：在服务层提供 `OLED_UpdateSync()`，用帧信号量保护整个刷新+绘制周期。

### 4.2 实现代码（新文件 `User/Services/oled_service.c`）

```c
/* ========== oled_service.c ========== */
#include "oled_driver.h"
#include "FreeRTOS.h"
#include "semphr.h"

static SemaphoreHandle_t frame_sem = NULL;  /* 帧缓冲访问互斥 */

void OLED_Service_Init(void)
{
    frame_sem = xSemaphoreCreateMutex();

    /* 驱动层初始化（内部创建 i2c_done_sem、注册DMA回调） */
    OLED_Init();
}

/* ===== 安全的全屏刷新 ===== */
void OLED_UpdateSync(void)
{
    xSemaphoreTake(frame_sem, portMAX_DELAY);
    OLED_Update();      /* 驱动层内部已有 I2C 写同步 */
    xSemaphoreGive(frame_sem);
}

/* ===== 安全的局部刷新 ===== */
void OLED_UpdateAreaSync(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
    xSemaphoreTake(frame_sem, portMAX_DELAY);
    OLED_UpdateArea(X, Y, Width, Height);
    xSemaphoreGive(frame_sem);
}

/* ===== 安全的绘图层（供菜单等外部模块使用） ===== */
void OLED_DrawSync_Begin(void)
{
    xSemaphoreTake(frame_sem, portMAX_DELAY);
}

void OLED_DrawSync_End(void)
{
    xSemaphoreGive(frame_sem);
}
```

### 4.3 如何防止上层直接调用驱动层的 Update？

**策略**：`oled_driver.h` 中**不声明** `OLED_Update` 和 `OLED_UpdateArea`，只暴露给 `oled_service.c` 通过 `extern` 使用。

```c
/* ========== oled_driver.h ========== */
/* 公开接口 — 绘图函数，外部可直接调用 */
void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowString(int16_t X, int16_t Y, char *String, uint8_t FontSize);
void OLED_DrawPoint(int16_t X, int16_t Y);
void OLED_DrawLine(...);
void OLED_ShowNum(...);
/* ... 其他绘图函数 ... */

/* 注意：OLED_Update 和 OLED_UpdateArea 不在此暴露，
   上层应使用 oled_service.h 中的 OLED_UpdateSync / OLED_UpdateAreaSync */
```

```c
/* ========== oled_service.c ========== */
/* 通过 extern 引用驱动层内部函数 */
extern void OLED_Update(void);
extern void OLED_UpdateArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);
```

---

## 五、Layer 3：菜单引擎 + 显示任务

### 5.1 核心数据结构

```c
/* ========== menu_engine.h ========== */

/* 抽象菜单事件 — 解耦物理按键 */
typedef enum {
    MENU_EVENT_NONE    = 0,
    MENU_EVENT_UP,
    MENU_EVENT_DOWN,
    MENU_EVENT_CONFIRM,
    MENU_EVENT_BACK,
} MenuEvent_t;

/* 转发声明 */
typedef struct MenuNode MenuNode_t;

/* 菜单节点 */
struct MenuNode {
    const char  *title;             /* 菜单标题 */
    MenuNode_t  *parent;            /* 上级菜单（NULL = 根节点） */
    MenuNode_t  **children;         /* 子菜单数组 */
    uint8_t      child_count;       /* 子菜单数量 */
    uint8_t      selected_index;    /* 当前选中项（光标位置） */

    /* ---- 回调函数 ---- */
    void      (*on_enter)(MenuNode_t *self);       /* 进入菜单时调用 */
    void      (*on_exit)(MenuNode_t *self);        /* 离开菜单时调用 */
    void      (*on_render)(MenuNode_t *self);      /* 绘制菜单内容（写 DisplayBuf） */
    MenuEvent_t (*on_input)(MenuNode_t *self, MenuEvent_t event); /* 处理事件 */
};
```

### 5.2 菜单状态机

```
                    ┌──────────┐
          ┌────────│  当前菜单  │────────┐
          │        └─────┬────┘        │
     UP/DOWN       CONFIRM          BACK
          │              │               │
          ▼              ▼               ▼
    ┌──────────┐  ┌──────────┐   ┌──────────────┐
    │ 移动光标  │  │ 进入子菜单 │   │ 返回父菜单    │
    │ selected │  │ selected  │   │ if parent    │
    │ = (i±1)  │  │ = 0       │   │ != NULL      │
    │ % count  │  │ on_enter  │   │ on_exit+enter│
    └──────────┘  └──────────┘   └──────────────┘
```

状态机的关键在于 `on_input` 回调的返回值语义：

```c
/* on_input 返回值约定 */
#define MENU_RET_CONSUMED   0   /* 事件已消费，无需引擎处理 */
#define MENU_RET_NAV_ENTER  1   /* 需要引擎执行"进入子菜单" */
#define MENU_RET_NAV_BACK   2   /* 需要引擎执行"返回上级" */
```

多数常规菜单只需返回 `MENU_RET_CONSUMED`，导航逻辑由引擎统一处理。特殊菜单（如时间设置界面）可以在 `on_input` 中自定义处理逻辑。

### 5.3 display_task 任务函数

```c
/* ========== display_task.c ========== */

/* 外部队列句柄（在 task_main.c 中创建） */
extern QueueHandle_t btn_queue;
extern QueueHandle_t sensor_queue;
extern QueueHandle_t battery_queue;

static MenuNode_t *current_menu = NULL;   /* 当前活跃菜单 */
static SensorData_t cached_sensor;        /* 最新传感器数据缓存 */
static uint8_t      cached_battery;       /* 最新电量缓存 */

static MenuEvent_t translate_button(BtnEvent_t btn);

void display_task(void *pvParameters)
{
    (void)pvParameters;

    /* ---- 初始化 ---- */
    OLED_Service_Init();
    current_menu = &menu_watch_face;       /* 启动后显示手表主屏 */
    current_menu->on_enter(current_menu);

    /* ---- 主循环 ---- */
    while (1)
    {
        MenuEvent_t menu_event = MENU_EVENT_NONE;

        /* ① 阻塞等待按键（100ms 超时 = 周期性刷新时钟/动画） */
        BtnEvent_t btn;
        if (xQueueReceive(btn_queue, &btn, pdMS_TO_TICKS(100)) == pdTRUE) {
            menu_event = translate_button(btn);
        }
        /* 超时 → menu_event 保持 NONE，触发周期性刷新 */

        /* ② 非阻塞轮询传感器数据 */
        SensorData_t sensor;
        while (xQueueReceive(sensor_queue, &sensor, 0) == pdTRUE) {
            cached_sensor = sensor;  /* 只保留最新值 */
        }

        /* ③ 非阻塞轮询电量数据 */
        uint8_t battery;
        while (xQueueReceive(battery_queue, &battery, 0) == pdTRUE) {
            cached_battery = battery;
        }

        /* ④ 将事件交给当前菜单处理 */
        MenuEvent_t ret = current_menu->on_input(current_menu, menu_event);

        /* ⑤ 菜单导航 */
        if (ret == MENU_RET_NAV_ENTER) {
            uint8_t idx = current_menu->selected_index;
            if (idx < current_menu->child_count && current_menu->children[idx]) {
                MenuNode_t *child = current_menu->children[idx];
                if (child->on_enter) child->on_enter(child);
                current_menu = child;
            }
        }
        else if (ret == MENU_RET_NAV_BACK) {
            if (current_menu->parent) {
                if (current_menu->on_exit) current_menu->on_exit(current_menu);
                current_menu = current_menu->parent;
                if (current_menu->on_enter) current_menu->on_enter(current_menu);
            }
        }
        /* MENU_RET_CONSUMED → 无需引擎干预 */

        /* ⑥ 渲染 */
        OLED_Clear();
        current_menu->on_render(current_menu);
        OLED_UpdateSync();
    }
}
```

### 5.4 按键 → 菜单事件的翻译

```c
static MenuEvent_t translate_button(BtnEvent_t btn)
{
    switch (btn.type) {
        case BTN_SHORT_PRESS:
            switch (btn.key) {
                case KEY_UP:      return MENU_EVENT_UP;
                case KEY_DOWN:    return MENU_EVENT_DOWN;
                case KEY_CONFIRM: return MENU_EVENT_CONFIRM;
                case KEY_BACK:    return MENU_EVENT_BACK;
                default:          return MENU_EVENT_NONE;
            }
        case BTN_LONG_PRESS:
            /* 长按可映射为特殊事件，或直接转给菜单处理 */
            break;
        default:
            break;
    }
    return MENU_EVENT_NONE;
}
```

### 5.5 一个具体菜单的实现示例

```c
/* ========== 手表主屏（Watch Face）========== */
static void watch_face_on_enter(MenuNode_t *self)
{
    self->selected_index = 0;  /* 默认选中第一项 */
}

static void watch_face_on_render(MenuNode_t *self)
{
    (void)self;
    extern SensorData_t cached_sensor;   /* 来自 display_task.c */
    extern uint8_t      cached_battery;

    /* 时间显示（居中） */
    OLED_ShowString(20, 0, get_time_string(), OLED_8X16);

    /* 电量图标（右上角） */
    OLED_ShowNum(110, 0, cached_battery, 3, OLED_6X8);
    OLED_ShowString(128, 0, "%", OLED_6X8);

    /* 传感器数据（底部） */
    OLED_Printf(0, 48, OLED_6X8, "X:%.1f Y:%.1f", cached_sensor.roll, cached_sensor.pitch);
}

static MenuEvent_t watch_face_on_input(MenuNode_t *self, MenuEvent_t event)
{
    switch (event) {
        case MENU_EVENT_CONFIRM:
            return MENU_RET_NAV_ENTER;  /* 进入子菜单（应用列表） */
        case MENU_EVENT_UP:
        case MENU_EVENT_DOWN:
            /* 主屏不需要光标，忽略 */
            return MENU_RET_CONSUMED;
        default:
            return MENU_RET_CONSUMED;
    }
}

/* ========== 通用列表菜单 ========== */
static void list_menu_on_render(MenuNode_t *self)
{
    /* 渲染标题栏 */
    OLED_ShowString(0, 0, (char *)self->title, OLED_8X16);
    OLED_DrawLine(0, 16, 127, 16);  /* 分隔线 */

    /* 渲染子项列表 */
    for (uint8_t i = 0; i < self->child_count; i++) {
        uint8_t y = 20 + i * 12;
        /* 光标指示器 */
        if (i == self->selected_index) {
            OLED_ShowString(0, y, ">", OLED_6X8);
        }
        OLED_ShowString(10, y, (char *)self->children[i]->title, OLED_6X8);
    }
}

static MenuEvent_t list_menu_on_input(MenuNode_t *self, MenuEvent_t event)
{
    switch (event) {
        case MENU_EVENT_UP:
            self->selected_index = (self->selected_index == 0)
                ? self->child_count - 1
                : self->selected_index - 1;
            return MENU_RET_CONSUMED;

        case MENU_EVENT_DOWN:
            self->selected_index = (self->selected_index + 1) % self->child_count;
            return MENU_RET_CONSUMED;

        case MENU_EVENT_CONFIRM:
            return MENU_RET_NAV_ENTER;

        case MENU_EVENT_BACK:
            return MENU_RET_NAV_BACK;

        default:
            return MENU_RET_CONSUMED;
    }
}
```

### 5.6 菜单树构建

```c
/* ========== 菜单定义（静态分配，所有节点编译期确定） ========== */

/* 叶子菜单 */
MenuNode_t menu_time_setting  = { .title = "时间设置", .parent = &menu_settings, ... };
MenuNode_t menu_brightness    = { .title = "亮度调节", .parent = &menu_settings, ... };
MenuNode_t menu_about         = { .title = "关于",     .parent = &menu_settings, ... };
MenuNode_t menu_angle_display = { .title = "角度显示", .parent = &menu_sensors,  ... };

/* 子菜单数组 */
MenuNode_t *settings_children[] = { &menu_time_setting, &menu_brightness, &menu_about };
MenuNode_t *sensors_children[]  = { &menu_angle_display, &menu_level_meter };

/* 中间菜单 */
MenuNode_t menu_settings = {
    .title       = "设置",
    .parent      = &menu_main,
    .children    = settings_children,
    .child_count = 3,
    .on_render   = list_menu_on_render,   /* 复用通用列表渲染器 */
    .on_input    = list_menu_on_input,
};

MenuNode_t menu_sensors = {
    .title       = "传感器",
    .parent      = &menu_main,
    .children    = sensors_children,
    .child_count = 2,
    .on_render   = list_menu_on_render,
    .on_input    = list_menu_on_input,
};

MenuNode_t *main_children[] = { &menu_settings, &menu_sensors, &menu_tools, &menu_power };

/* 根菜单（手表主屏） */
MenuNode_t menu_main = {
    .title       = "主屏幕",
    .parent      = NULL,
    .children    = main_children,
    .child_count = 4,
    .selected_index = 0,
    .on_render   = watch_face_on_render,   /* 主屏用自定义渲染 */
    .on_input    = watch_face_on_input,
};
```

---

## 六、数据流全景

```
┌──────────────┐   按键队列(8深)    ┌──────────────────┐
│ btn_scan     │ ─────────────────→ │                  │
│ (定时器ISR)  │                    │   display_task   │
└──────────────┘                    │                  │
                                    │  ①等按键/超时    │
┌──────────────┐   传感器队列(2深)   │  ②轮询传感器/电量│
│ sensor_task  │ ─────────────────→ │  ③当前菜单.on_input│
│ (10ms周期)   │   xQueueOverwrite  │  ④当前菜单.on_render│
└──────────────┘                    │  ⑤OLED_UpdateSync│
                                    └────────┬─────────┘
┌──────────────┐   电量队列(2深)              │
│ battery_task │ ─────────────────→         │
│ (1000ms周期) │   xQueueOverwrite          │
└──────────────┘                            ▼
                                    ┌──────────────────┐
                                    │  oled_service     │
                                    │  frame_sem Take   │
                                    │  OLED_Update()    │
                                    │  frame_sem Give   │
                                    └────────┬─────────┘
                                             │
                                    ┌────────┴─────────┐
                                    │  oled_driver      │
                                    │  WriteCmd/Data    │
                                    │  i2c_done_sem同步 │
                                    └────────┬─────────┘
                                             │
                                    ┌────────┴─────────┐
                                    │  I2C + DMA        │
                                    │  回调Give信号量    │
                                    └──────────────────┘
```

---

## 七、关键设计决策

| 决策 | 选择 | 理由 |
|------|:----:|------|
| I2C 写同步放在哪层？ | oled_driver 内部 | 耦合最小，上层绘图函数无感 |
| WriteCmd 局部变量问题 | static 变量承接 | 简单可靠，信号量保证互斥 |
| 帧级同步放在哪层？ | oled_service（服务层） | 驱动层无 RTOS 依赖，服务层封装同步 |
| 如何防止绕过同步？ | .h 不暴露 Update 声明 | oled_service.c 通过 extern 调用 |
| 菜单遍历方式 | 链表（MenuNode） | 灵活，静态分配无动态内存 |
| 事件翻译在哪？ | display_task 内部 | 解耦物理按键布局与菜单逻辑 |
| 菜单渲染模型 | 回调 + 通用渲染器复用 | 主屏等特殊菜单自定义，设置类复用列表渲染器 |
| 传感器/电量更新方式 | 队列 overwrite + 缓存 | 显示任务取最新值，不阻塞传感器采集 |

---

## 八、文件清单

| 文件 | 作用 | 状态 |
|------|------|:----:|
| `User/Drivers/oled_driver.c/h` | OLED 驱动：帧缓冲、绘图函数、I2C 写同步 | 修改 |
| `User/Drivers/oled_common.h` | OLED 常量定义（I2C地址、字体大小等） | 不变 |
| `User/Services/oled_service.c/h` | OLED 帧同步服务：UpdateSync、DrawSync | **新建** |
| `User/Services/menu_engine.c/h` | 菜单引擎：MenuNode 结构、状态机、导航 | **新建** |
| `User/Tasks/display_task.c/h` | 显示任务：事件循环、数据缓存、菜单驱动 | **新建** |
| `User/BSP/bsp_i2c1_hw.c/h` | 硬件 I2C + DMA 驱动 | 修改（新增回调注册） |
| `User/Middlewares/I2C/i2c_interface.h` | I2C 抽象接口（新增 register_dma_cb） | 修改 |
| `User/Common/common_macro.h` | 编译宏（I2C1_SW_ENABLE 等） | 不变 |

---

## 九、验证方案

### 9.1 DMA 同步验证

1. 将 `I2C1_SW_ENABLE` 设为 `0`（启用硬件 I2C + DMA）
2. 在 `i2c_test.c` 中运行 `OLED_ComprehensiveTest()`
3. 检查项：
   - 全屏填充显示完整无撕裂
   - 进度条动画流畅
   - 日志中无 I2C 错误
   - 使用逻辑分析仪抓 I2C 波形，确认每帧数据完整

### 9.2 菜单导航验证

1. 连接硬件按键（UP/DOWN/CONFIRM/BACK）
2. 在主屏按 CONFIRM → 进入应用列表
3. UP/DOWN 移动光标
4. CONFIRM 进入子菜单
5. BACK 返回上级
6. 检查每个菜单切换时 OLED 显示正确

### 9.3 多任务并发验证

1. 同时运行 sensor_task（10ms 周期写传感器队列）、battery_task（1s 周期）、display_task
2. 在菜单之间快速导航
3. 检查显示屏是否出现撕裂、花屏
4. 检查 FreeRTOS 任务栈高水位标记，确保 display_task 768 字栈够用

---

> **下一步**：评审通过后，使用 writing-plans 技能生成详细的实现计划（分步骤、分文件、可执行的任务清单）。
