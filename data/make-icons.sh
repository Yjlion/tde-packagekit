#!/bin/bash
# Draws the application icon. Run inside the container: data/make-icons.sh
# The results are committed, so this only needs running to change the design.
set -euo pipefail
cd "$(dirname "$0")"
for size in 16 22 32 48 64 128; do
    convert -size 256x256 xc:none \
        -fill '#b98a4f' -stroke '#6b4a22' -strokewidth 8 -draw 'polygon 128,24 228,74 128,124 28,74' \
        -fill '#8f6431' -draw 'polygon 28,74 128,124 128,232 28,182' \
        -fill '#a97a42' -draw 'polygon 228,74 128,124 128,232 228,182' \
        -stroke none -fill '#e9d5a8' -draw 'polygon 68,54 168,104 188,94 88,44' \
        -fill '#d9c08c' -draw 'polygon 168,104 188,94 188,128 168,138' \
        -fill '#2f9e44' -stroke white -strokewidth 8 -draw 'circle 186,186 186,142' \
        -stroke white -strokewidth 16 -fill none -draw 'polyline 186,210 186,162' \
        -draw 'polyline 166,182 186,160 206,182' \
        -resize "${size}x${size}" "cr${size}-app-tdepackagekit.png"
done
