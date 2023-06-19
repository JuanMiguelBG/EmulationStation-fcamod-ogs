#!/bin/bash

LOG_FILE="/roms/logs/n64.log"
echo "Nintendo64: " | tee $LOG_FILE

# ONLY ArkOSNoid or Arale

#if [ -f "/boot/rk3566.dtb" ] || [ -f "/boot/rk3566-OC.dtb" ] || [ -f "/boot/rk3326-rg351v-linux.dtb" ] || [ -f "/boot/rk3326-rg351mp-linux.dtb" ] || [ -f "/boot/rk3326-gameforce-linux.dtb" ]; then
  xres="$(cat /sys/class/graphics/fb0/modes | grep -o -P '(?<=:).*(?=p-)' | cut -dx -f1)"
  yres="$(cat /sys/class/graphics/fb0/modes | grep -o -P '(?<=:).*(?=p-)' | cut -dx -f2)"
#else
#  xres="$(cat /sys/class/graphics/fb0/modes | grep -o -P '(?<=:).*(?=p-)' | cut -dx -f2)"
#  yres="$(cat /sys/class/graphics/fb0/modes | grep -o -P '(?<=:).*(?=p-)' | cut -dx -f1)"
#fi

#if [ -f "/boot/rk3566.dtb" ] || [ -f "/boot/rk3566-OC.dtb" ] || [ -f "/boot/rk3326-odroidgo2-linux.dtb" ] || [ -f "/boot/rk3326-odroidgo2-linux-v11.dtb" ] || [ -f "/boot/rk3326-odroidgo3-linux.dtb" ]; then
#  game="${3}"
#else
#  game="${2}"
#fi

EMULATOR="$1"
CORE="$2"
GAME="$3"

EMULATOR_MSG="Loading Emulator: "
[[ $EMULATOR == *"standalone"* ]] && EMULATOR_MSG+="Mupen64Plus"
EMULATOR_MSG+=" $EMULATOR"
echo "$EMULATOR_MSG" | tee -a $LOG_FILE
echo "Loading Core: $CORE" | tee -a $LOG_FILE
echo "Loading ROM: $GAME" | tee -a $LOG_FILE


M64P_PARAMETERS=""
if [[ $EMULATOR == *"standalone"* ]]; then

  if [[ $EMULATOR == *"Glide64mk2"* ]]; then

    M64P_PARAMETERS="--set Video-Glide64mk2[aspect]="
    if [[ $CORE == "Widescreen_Aspect" ]]; then
      M64P_PARAMETERS+="1"
    else
      M64P_PARAMETERS+="-1"
    fi
    M64P_PARAMETERS+=" --resolution \"${xres}x${yres}\" --gfx mupen64plus-video-glide64mk2.so"

  elif [[ $EMULATOR == *"GlideN64"* ]]; then

    M64P_PARAMETERS=" --set Video-GLideN64[AspectRatio]="
    if [[ $CORE == "Widescreen_Aspect" ]]; then
      M64P_PARAMETERS+="3"
    else
      M64P_PARAMETERS+="1"
    fi
    M64P_PARAMETERS+=" --resolution \"${xres}x${yres}\" --gfx mupen64plus-video-GLideN64.so"

  else #if [[ $EMULATOR == *"Rice"* ]]; then
    # set horizontal resolution
    sudo sed -i '/ResolutionWidth \=/c\ResolutionWidth \= '"${xres}"'' "/home/ark/.config/mupen64plus/mupen64plus.cfg"

    M64P_PARAMETERS="--resolution"
    if [[ $CORE == "Widescreen_Aspect" ]]; then
      M64P_PARAMETERS+=" \"${xres}"
    else
      ricewidthhack=$(((yres * 4) / 3))
      M64P_PARAMETERS+=" \"${ricewidthhack}"
    fi
    M64P_PARAMETERS+="x${yres}\" --gfx mupen64plus-video-rice.so"

  fi

  echo "M64P_PARAMETERS: $M64P_PARAMETERS" | tee -a $LOG_FILE
  # launching emulator
  echo -e "COMMAND: /opt/mupen64plus/mupen64plus $M64P_PARAMETERS --plugindir /opt/mupen64plus --corelib /opt/mupen64plus/libmupen64plus.so.2 --datadir /opt/mupen64plus \"$GAME\"\n" | tee -a $LOG_FILE

  /usr/local/bin/es-splash_screen quit_splash_before_game
  /opt/mupen64plus/mupen64plus $M64P_PARAMETERS --plugindir /opt/mupen64plus --corelib /opt/mupen64plus/libmupen64plus.so.2 --datadir /opt/mupen64plus "$GAME" | tee -a $LOG_FILE
  /usr/local/bin/es-splash_screen load_splash_after_game
else
   # launching emulator
   echo "COMMAND: /usr/local/bin/$EMULATOR -L /home/ark/.config/$EMULATOR/cores/$CORE_libretro.so \"$GAME\"" | tee -a $LOG_FILE
  /usr/local/bin/"$EMULATOR" -L /home/ark/.config/"$EMULATOR"/cores/"$CORE"_libretro.so "$GAME"
fi

echo -e "\nExiting...\n" | tee -a $LOG_FILE
