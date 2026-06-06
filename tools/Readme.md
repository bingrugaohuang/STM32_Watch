# convert_encoding.sh — GBK / UTF-8 文件编码转换工具

## 运行环境

- **Git Bash**（Windows 自带 Git 安装后就有）
- **不能**在 PowerShell 或 CMD 中运行（bash 脚本）

## 快速开始

```bash
# 在项目根目录打开 Git Bash，然后：

# 单文件：GBK → UTF-8
./tools/convert_encoding.sh gbk2utf8 ./User/Drivers/oled_data.c

# 单文件：UTF-8 → GBK
./tools/convert_encoding.sh utf82gbk ./User/Drivers/i2c_test.c

# 批量：转换目录下所有 .c 文件
./tools/convert_encoding.sh batch ./User/Drivers gbk2utf8 c
./tools/convert_encoding.sh batch ./User/BSP utf82gbk c

# 批量：转换目录下所有 .h 文件
./tools/convert_encoding.sh batch ./User/Drivers gbk2utf8 h

# 批量：转换目录下所有 .c + .h（all）
./tools/convert_encoding.sh batch ./User/Services utf82gbk all

# 查看帮助
./tools/convert_encoding.sh --help
```

## 三个命令

| 命令 | 用法 | 用途 |
|------|------|------|
| `gbk2utf8` | `./tools/convert_encoding.sh gbk2utf8 <文件>` | 单个文件 GBK → UTF-8 |
| `utf82gbk` | `./tools/convert_encoding.sh utf82gbk <文件>` | 单个文件 UTF-8 → GBK |
| `batch` | `./tools/convert_encoding.sh batch <目录> <方向> <扩展名>` | 批量转换整个目录 |

## 参数说明

### 方向

| 值 | 含义 |
|----|------|
| `gbk2utf8` | GBK 编码 → UTF-8 编码 |
| `utf82gbk` | UTF-8 编码 → GBK 编码 |

### 扩展名

| 值 | 匹配文件 |
|----|----------|
| `c` | `*.c` |
| `h` | `*.h` |
| `cpp` | `*.cpp` |
| `hpp` | `*.hpp` |
| `all` | `*.c *.h *.cpp *.hpp` |
| 其他 | `*.<你填的值>` |

## 常见用例

### 用例 1：从旧项目（GBK）移植代码后，想在新 IDE 中正常显示中文注释

```bash
# 把从标准库项目拷过来的文件转为 UTF-8，VSCode 就能正常显示
./tools/convert_encoding.sh gbk2utf8 ./User/Drivers/oled_data.c
./tools/convert_encoding.sh gbk2utf8 ./User/Drivers/oled_strings_gbk.c
```

### 用例 2：ARM Compiler 5 编译报 `#870-D` 或 `#8` 编码错误

```bash
# AC5 不支持 UTF-8 源文件，把所有 .c 转为 GBK
./tools/convert_encoding.sh batch ./User/Drivers utf82gbk c
./tools/convert_encoding.sh batch ./User/BSP utf82gbk c
./tools/convert_encoding.sh batch ./User/Services utf82gbk c

# 注意：oled_strings_gbk.c 本身就是 GBK，不要反向转！
# 如果不小心转了，用下面命令改回来：
./tools/convert_encoding.sh utf82gbk ./User/Drivers/oled_strings_gbk.c
```

### 用例 3：提交 Git 前，统一文件编码为 UTF-8

```bash
# 把整个 User 目录转为 UTF-8，方便 Git diff 显示中文
./tools/convert_encoding.sh batch ./User gbk2utf8 all
# （注：编译前再按用例 2 转回 GBK）
```

### 用例 4：只转换指定的几个文件

```bash
./tools/convert_encoding.sh utf82gbk ./User/Drivers/oled_driver.c
./tools/convert_encoding.sh utf82gbk ./User/Drivers/i2c_test.c
./tools/convert_encoding.sh utf82gbk ./User/Drivers/serial.c
```

## 路径写法

脚本从**当前工作目录**查找文件，所以路径是相对的：

```bash
# ✅ 正确：在项目根目录运行
cd d:/Desk/Watch/STM32HAL/STM32_Watch_Rewrite
./tools/convert_encoding.sh gbk2utf8 ./User/Drivers/oled_data.c

# ✅ 也正确：使用绝对路径
./tools/convert_encoding.sh gbk2utf8 d:/Desk/Watch/STM32HAL/STM32_Watch_Rewrite/User/Drivers/oled_data.c

# ❌ 错误：在 tools 目录下运行，相对路径找不到
cd d:/Desk/Watch/STM32HAL/STM32_Watch_Rewrite/tools
./convert_encoding.sh gbk2utf8 ./User/Drivers/oled_data.c  # 找不到！
```

## 常见陷阱

| 症状 | 原因 | 解决 |
|------|------|------|
| `[ERR] 文件不存在` | 路径写错或不在项目根目录 | 检查文件名拼写，确保在项目根目录执行 |
| 转换后中文变问号 | 源文件编码与参数指定的不一致 | 确认文件当前是什么编码再转换 |
| `iconv: illegal input sequence` | 源文件已损坏或编码检测错误 | 用 VSCode 右下角查看实际编码 |
| 批量转换后编译报错 | `oled_strings_gbk.c` 被意外转成 UTF-8 | 单独把它转回 GBK |

## 本项目文件编码速查

| 文件 | 编码 | 原因 |
|------|:---:|------|
| `oled_data.c` | **GBK** | 字库数据，GB2312 索引 |
| `oled_strings_gbk.c` | **GBK** | OLED 显示用的中文字符串，必须匹配 GB2312 字库 |
| `oled_driver.c` | GBK | 中文注释，驱动层 |
| `i2c_test.c` | UTF-8/ASCII | 纯英文日志，无中文字符串 |
| `oled_driver.h` | UTF-8 | API 声明头文件 |
| `oled_common.h` | UTF-8 | 常量定义 |
| `oled_strings_gbk.h` | UTF-8 | extern 声明（仅 ASCII） |
| `docs/*.md` | UTF-8 | 文档 |

## 编码知识速记

| 编译器 | 支持编码 | 中文推荐 |
|--------|----------|----------|
| ARM Compiler 5 (armcc) | GBK / ANSI | .c 文件用 GBK |
| ARM Compiler 6 (armclang) | UTF-8 | .c 文件可用 UTF-8 |
| GCC (arm-none-eabi) | UTF-8 | .c 文件可用 UTF-8 |

**关键**：如果你用 ARMCC5，含有中文**字符串字面量**的 `.c` 文件必须是 GBK。只有**注释**中包含中文的文件不受影响（注释在编译时被丢弃）。
