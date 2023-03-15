#!/bin/bash
LOG_FILE="/roms/logs/calibrate_tv.log"
echo "CALIBRATE TV: " | tee "$LOG_FILE"

GAMEDIR="/roms/ports/calibrate_tv"

if [ ! -d "$GAMEDIR" ]; then
	echo "Directory '$GAMEDIR' does NOT exist, skipping ..." | tee -a "$LOG_FILE"
fi

/usr/local/bin/controller_setup.sh

/opt/retroarch/bin/retroarch --config /home/ark/.config/retroarch/retroarch.cfg -L "$GAMEDIR/picodrive_libretro.so" "$GAMEDIR/240pSuite_32X_1.0.32x" 2>&1 | tee -a "$LOG_FILE"
