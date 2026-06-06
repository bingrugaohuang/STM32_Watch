#!/bin/bash
# ================================================================
#  convert_encoding.sh — GBK / UTF-8 文件编码转换工具
#
#  用法:
#    ./convert_encoding.sh gbk2utf8  <文件或目录>
#    ./convert_encoding.sh utf82gbk  <文件或目录>
#    ./convert_encoding.sh batch     <目录>  gbk2utf8|utf82gbk
#
#  示例:
#    # 单个文件 GBK→UTF-8
#    ./convert_encoding.sh gbk2utf8 oled_driver.c
#
#    # 批量转换目录下所有 .c 文件 UTF-8→GBK
#    ./convert_encoding.sh batch ./User/Drivers utf82gbk c
#
#    # 批量转换目录下所有 .h 文件 GBK→UTF-8
#    ./convert_encoding.sh batch ./User/Drivers gbk2utf8 h
#
#  依赖: iconv (Git Bash 自带)
# ================================================================

set -e

# ---- 颜色输出 ----
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_ok()  { echo -e "${GREEN}[OK]${NC} $1"; }
print_err() { echo -e "${RED}[ERR]${NC} $1"; }
print_info(){ echo -e "${YELLOW}[INFO]${NC} $1"; }

# ---- 使用说明 ----
usage() {
    echo "用法:"
    echo "  $0 gbk2utf8  <文件>          — 单个文件 GBK → UTF-8"
    echo "  $0 utf82gbk  <文件>          — 单个文件 UTF-8 → GBK"
    echo "  $0 batch <目录> <方向> <扩展名> — 批量转换"
    echo ""
    echo "方向: gbk2utf8 | utf82gbk"
    echo "扩展名: c | h | all (不带点)"
    echo ""
    echo "示例:"
    echo "  $0 gbk2utf8 main.c"
    echo "  $0 utf82gbk oled_driver.c"
    echo "  $0 batch ./User/Drivers gbk2utf8 c"
    echo "  $0 batch . utf82gbk all"
    exit 1
}

# ---- 检查 iconv ----
if ! command -v iconv &>/dev/null; then
    print_err "iconv 未安装。请使用 Git Bash 或安装 iconv。"
    exit 1
fi

# ---- 单文件转换 ----
convert_file() {
    local from_enc="$1"
    local to_enc="$2"
    local file="$3"

    if [ ! -f "$file" ]; then
        print_err "文件不存在: $file"
        return 1
    fi

    # 先检测当前编码
    local detected
    detected=$(file -b --mime-encoding "$file" 2>/dev/null || echo "unknown")

    local tmpfile="${file}.tmp.$$"

    if iconv -f "$from_enc" -t "$to_enc" "$file" > "$tmpfile" 2>/dev/null; then
        mv "$tmpfile" "$file"
        print_ok "$file  ($from_enc → $to_enc, 检测为 $detected)"
    else
        rm -f "$tmpfile"
        print_err "$file  转换失败（文件可能不是 ${from_enc} 编码？）"
        return 1
    fi
}

# ---- 批量转换 ----
batch_convert() {
    local dir="$1"
    local direction="$2"
    local ext="$3"

    if [ ! -d "$dir" ]; then
        print_err "目录不存在: $dir"
        return 1
    fi

    local from_enc to_enc
    case "$direction" in
        gbk2utf8) from_enc="GBK"; to_enc="UTF-8" ;;
        utf82gbk) from_enc="UTF-8"; to_enc="GBK" ;;
        *) print_err "无效方向: $direction（应为 gbk2utf8 或 utf82gbk）"; return 1 ;;
    esac

    local pattern
    case "$ext" in
        c)   pattern="*.c" ;;
        h)   pattern="*.h" ;;
        all) pattern="*.c *.h *.cpp *.hpp" ;;
        *)   pattern="*.$ext" ;;
    esac

    local count=0 fail=0
    print_info "批量转换: $dir 下所有 ${pattern} 文件  ${from_enc} → ${to_enc}"

    # 使用 find 处理子目录
    for ext_glob in $pattern; do
        while IFS= read -r -d '' file; do
            if convert_file "$from_enc" "$to_enc" "$file"; then
                ((count++))
            else
                ((fail++))
            fi
        done < <(find "$dir" -name "$ext_glob" -type f -print0 2>/dev/null)
    done

    echo ""
    print_info "完成: $count 个文件转换成功, $fail 个失败"
}

# ---- 主入口 ----
case "$1" in
    gbk2utf8)
        [ -z "$2" ] && usage
        convert_file "GBK" "UTF-8" "$2"
        ;;
    utf82gbk)
        [ -z "$2" ] && usage
        convert_file "UTF-8" "GBK" "$2"
        ;;
    batch)
        [ -z "$2" ] || [ -z "$3" ] || [ -z "$4" ] && usage
        batch_convert "$2" "$3" "$4"
        ;;
    -h|--help|help)
        usage
        ;;
    *)
        usage
        ;;
esac
