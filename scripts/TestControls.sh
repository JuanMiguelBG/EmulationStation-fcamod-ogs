#!/bin/bash
LOG_FILE="/roms/logs/test_controls.log"
echo "CONTROL PAD TEST: " | tee "$LOG_FILE"

GAMEDIR="/roms/ports/pad/"

if [ ! -d "$GAMEDIR" ]; then
	echo "Directory '$GAMEDIR' does NOT exist, skipping ..." | tee -a "$LOG_FILE"
	exit 1
fi

/opt/retroarch/bin/retroarch32 --config /home/ark/.config/retroarch32/retroarch.cfg -L "$GAMEDIR/pcsx_rearmed_libretro.so" "$GAMEDIR/ControlTEST.chd" 2>&1 | tee -a "$LOG_FILE"

echo "Exiting..." | tee -a "$LOG_FILE"
