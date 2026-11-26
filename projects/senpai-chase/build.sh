#!/bin/bash
# Compila (e con UPLOAD=1 carica) lo sketch sulla CYD (ESP32-2432S0xx).
# ARDUINO_CLI=percorso di arduino-cli (default: arduino-cli nel PATH), PORT=porta seriale (default /dev/ttyUSB0)
CLI=${ARDUINO_CLI:-arduino-cli}
DRIVER=${DRIVER:-ST7789_DRIVER}   # pannello di questa scheda: ST7789, ordine BGR, niente inversione
FLAGS="-DUSER_SETUP_LOADED=1 -D$DRIVER=1 -DTFT_RGB_ORDER=${RGB_ORDER:-0} -DINVERT_COLORS=${INVERT:-0} -DTFT_WIDTH=240 -DTFT_HEIGHT=320 -DUSE_HSPI_PORT=1 \
 -DTFT_MISO=12 -DTFT_MOSI=13 -DTFT_SCLK=14 -DTFT_CS=15 -DTFT_DC=2 -DTFT_RST=-1 \
 -DTFT_BL=21 -DTFT_BACKLIGHT_ON=HIGH -DSPI_FREQUENCY=55000000 -DSPI_READ_FREQUENCY=20000000 \
 -DTOUCH_CS=33 -DSPI_TOUCH_FREQUENCY=2500000 -DLOAD_GLCD=1 -DLOAD_FONT2=1 -DLOAD_FONT4=1 -DLOAD_GFXFF=1 -DSMOOTH_FONT=1 $EXTRA"
cd "$(dirname "$0")"
$CLI compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app --build-property "build.extra_flags=-DESP32 $FLAGS" ${UPLOAD:+--upload -p ${PORT:-/dev/ttyUSB0}} ${SKETCH:-senpai}
