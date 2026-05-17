#!/bin/bash
# Скрипт для скачивания шрифта DejaVuSans с поддержкой кириллицы.
# Запусти один раз из папки build:
#   cd build && bash ../get_font.sh

FONT_URL="https://github.com/dejavu-fonts/dejavu-fonts/releases/download/version_2_37/dejavu-fonts-ttf-2.37.tar.bz2"
FONT_FILE="DejaVuSans.ttf"

if [ -f "$FONT_FILE" ]; then
    echo "Font already exists: $FONT_FILE"
    exit 0
fi

echo "Downloading DejaVu fonts..."

# Попробуем через curl
if command -v curl &>/dev/null; then
    curl -L "$FONT_URL" -o dejavu.tar.bz2
elif command -v wget &>/dev/null; then
    wget "$FONT_URL" -O dejavu.tar.bz2
else
    echo "Error: curl or wget not found."
    echo "Please download DejaVuSans.ttf manually from:"
    echo "https://dejavu-fonts.github.io/"
    echo "and place it next to the FoucaultPendulum executable."
    exit 1
fi

echo "Extracting..."
tar xjf dejavu.tar.bz2
cp dejavu-fonts-ttf-2.37/ttf/DejaVuSans.ttf .
rm -rf dejavu-fonts-ttf-2.37 dejavu.tar.bz2

echo "Done! DejaVuSans.ttf is ready."