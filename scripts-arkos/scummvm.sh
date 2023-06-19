#!/bin/bash

LOG_FILE="/roms/logs/scummvm.log"
echo "SCUMMVM: " | tee $LOG_FILE

#if [[ -e "/dev/input/by-path/platform-ff300000.usb-usb-0:1.2:1.0-event-joystick" ]]; then
#  param_device="anbernic"
#elif [[ -e "/dev/input/by-path/platform-odroidgo2-joypad-event-joystick" ]]; then
#  if [[ ! -z $(cat /etc/emulationstation/es_input.cfg | grep "190000004b4800000010000001010000") ]]; then
#    param_device="oga"
#  else
#    param_device="rk2020"
#  fi
#elif [[ -e "/dev/input/by-path/platform-odroidgo3-joypad-event-joystick" ]]; then
#  param_device="ogs"
#elif [[ -e "/dev/input/by-path/platform-singleadc-joypad-event-joystick" ]]; then
  param_device="rg503"
#else
#  param_device="chi"
#fi

EMULATOR="$1"

echo "Loading Emulator: SCUMMVM ($EMULATOR)" | tee -a $LOG_FILE

if  [[ $EMULATOR == "standalone" ]]; then
   echo "Loading ROM: $2" | tee -a $LOG_FILE
	 # -x forces exact match
   sudo /opt/quitter/oga_controls "-x scummvm" $param_device &
   rom=`echo $2 | grep -oP "\/(?:.(?!\/))+$"`
   rom="${rom:1:-8}"
   echo "Loading SCUMMVM ROM: $rom" | tee -a $LOG_FILE

   # launching emulator
   echo -e "COMMAND: /opt/scummvm/scummvm \"$rom\"\n" | tee -a $LOG_FILE

   /usr/local/bin/es-splash_screen quit_splash_before_game
   /opt/scummvm/scummvm "$rom" 2>&1 | tee -a $LOG_FILE
   /usr/local/bin/es-splash_screen load_splash_after_game

   if [[ ! -z $(pidof oga_controls) ]]; then
      sudo kill -9 $(pidof oga_controls)
   fi
else
   echo "Loading Core: $2" | tee -a $LOG_FILE
   echo "Loading ROM: $3" | tee -a $LOG_FILE
   rom=`echo $3 | grep -oP "\/(?:.(?!\/))+$"`
   rom="${3}${rom}"
   echo "Loading SCUMMVM ROM: $rom" | tee -a $LOG_FILE

   # launching emulator
   echo "COMMAND: /usr/local/bin/retroarch -L /home/ark/.config/retroarch/cores/$2_libretro.so \"$rom\"" | tee -a $LOG_FILE

   /usr/local/bin/retroarch -L /home/ark/.config/retroarch/cores/"$2"_libretro.so "$rom"
fi

sudo systemctl restart oga_events &

echo -e "\nExiting...\n" | tee -a $LOG_FILE
