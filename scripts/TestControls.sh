#!/bin/bash
LOG_FILE="/roms/logs/controls.log"
echo "CONTROL PAD TEST: " | tee "$LOG_FILE"

GAMEDIR="/roms/ports/pad/"

if [ ! -d "$GAMEDIR" ]; then
	echo "Directory '$GAMEDIR' does NOT exist, skipping ..." | tee -a "$LOG_FILE"
fi

/usr/local/bin/controller_setup.sh

/opt/retroarch/bin/retroarch32 --config /home/ark/.config/retroarch32/retroarch.cfg -L "$GAMEDIR/pcsx_rearmed_libretro.so" "$GAMEDIR/ControlTEST.chd" 2>&1 | tee -a "$LOG_FILE"
