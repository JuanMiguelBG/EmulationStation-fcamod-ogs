#!/bin/bash

do_help() {
    echo "$0 <DEVICE_ID> <DEVICE_NAME> <DEVICE_GUID>" >&2
}

declare -A LIBRETRO_BUTTON=(
    [left]="input_left_btn"
    [right]="input_right_btn"
    [up]="input_up_btn"
    [down]="input_down_btn"
    [a]="input_a_btn"
    [b]="input_b_btn"
    [x]="input_x_btn"
    [y]="input_y_btn"
    [l1]="input_l_btn"
    [leftshoulder]="input_l_btn"
    [pageup]="input_l_btn"
    [r1]="input_r_btn"
    [rightshoulder]="input_r_btn"
    [pagedown]="input_r_btn"
    [lefttrigger]="input_l2_btn"
    [l2]="input_l2_btn"
    [righttrigger]="input_r2_btn"
    [r2]="input_r2_btn"
    [leftthumb]="input_l3_btn"
    [l3]="input_l3_btn"
    [rightthumb]="input_r3_btn"
    [r3]="input_r3_btn"
    [select]="input_select_btn"
    [start]="input_start_btn"
    [leftanalogdown]="input_l_y_plus_axis"
    [leftanalogup]="input_l_y_minus_axis"
    [joystick1up]="input_l_y_minus_axis"
    [leftanalogleft]="input_l_x_minus_axis"
    [joystick1left]="input_l_x_minus_axis"
    [leftanalogright]="input_l_x_plus_axis"
    [rightanalogdown]="input_r_y_plus_axis"
    [rightanalogup]="input_r_y_minus_axis"
    [joystick2up]="input_r_y_minus_axis"
    [rightanalogleft]="input_r_x_minus_axis"
    [joystick2left]="input_r_x_minus_axis"
    [rightanalogright]="input_r_x_plus_axis"
    [hotkeyenable]="input_hotkey_btn"
    [hotkey]="input_hotkey_btn"
)


declare -A SDL_BUTTON=(
    [left]="dpleft"
    [right]="dpright"
    [up]="dpup"
    [down]="dpdown"
    [a]="a"
    [b]="b"
    [x]="x"
    [y]="y"
    [l1]="leftshoulder"
    [leftshoulder]="leftshoulder"
    [pageup]="leftshoulder"
    [r1]="rightshoulder"
    [rightshoulder]="rightshoulder"
    [pagedown]="rightshoulder"
    [lefttrigger]="lefttrigger"
    [l2]="lefttrigger"
    [righttrigger]="righttrigger"
    [r2]="righttrigger"
    [leftthumb]="leftstick"
    [l3]="leftstick"
    [rightthumb]="rightstick"
    [r3]="rightstick"
    [select]="back"
    [start]="start"
    [leftanalogdown]=""
    [leftanalogup]="lefty"
    [joystick1up]="lefty"
    [leftanalogleft]="leftx"
    [joystick1left]="leftx"
    [leftanalogright]=""
    [rightanalogdown]=""
    [rightanalogup]="righty"
    [joystick2up]="righty"
    [rightanalogleft]="rightx"
    [joystick2left]="rightx"
    [rightanalogright]=""
    [hotkeyenable]="guide"
    [hotkey]="guide"
)


do_get_es_controller_declaration()
{
    local controller_guid="$1"
    cat ${ES_INPUT_CFG_FILE} | grep "${controller_guid}" -m 1 -A 50 | grep "</inputConfig>" -m 1 -B 50
}


isXboxController()
{
    local device_name="${1,,}" # to lowercase
    local device_guid="$2"

    if [[ "$device_name" == *"xbox"* ]] || [[ "$device_name" == *"x-box"* ]]
       || [[ "$device_name" == *"microsoft"* ]] || [[ "$device_guid" == *"5e040000"* ]]; then
        echo 1
    fi
    echo 0
}

isPsxController()
{
    local device_name="${1,,}" # to lowercase
    local device_guid="$2"

    if [[ "$device_name" == *"playstation"* ]] || [[ "$device_name" == *"sony"* ]]
       || [[ "$device_name" == *"ps(r) ga"* ]] || [[ "$device_name" == *"ps2"* ]]
       || [[ "$device_name" == *"ps3"* ]] || [[ "$device_name" == *"ps4"* ]]
       || [[ "$device_name" == *"ps5"* ]] || [[ "$device_name" == *"dualShock"* ]]
       || [[ "$device_guid" == *"4c050000"* ]] || [[ "$device_guid" == *"6b140000"* ]]; then
        echo 1
    fi
    echo 0
}

update_device_buttons_labels_libretro()
{
    local device_name="$1"
    local device_guid="$2"
    local is_trigger_axis=$3
    [ -z "$is_trigger_axis" ] && is_trigger_axis=0
    #print_log $LOG_FILE "INFO" "Executing 'update_device_buttons_labels_libretro()' - device name: '$device_name', GUID: '$device_guid'"

    local ra_config_paths=( "$RA_CONFIG_FOLDER" "$RA32_CONFIG_FOLDER" )
    local labels_def="\n# Button labels\n"

    if [ $(isXboxController "$device_name" "$device_guid") -eq 1 ]; then
       labels_def+="input_b_btn_label = \"A\"\n"
       labels_def+="input_y_btn_label = \"X\"\n"
       labels_def+="input_select_btn_label = \"Back\"\n"
       labels_def+="input_start_btn_label = \"Start\"\n"
       labels_def+="input_a_btn_label = \"B\"\n"
       labels_def+="input_x_btn_label = \"Y\"\n"
       labels_def+="input_l_btn_label = \"LB\"\n"
       labels_def+="input_r_btn_label = \"RB\"\n"
       if [ $is_trigger_axis - eq 1 ]; then
          labels_def+="input_l2_axis_label = \"LT\"\n"
          labels_def+="input_r2_axis_label = \"RT\"\n"
       else
          labels_def+="input_l2_btn_label = \"LT\"\n"
          labels_def+="input_r2_btn_label = \"RT\"\n"
       fi
       labels_def+="input_l3_btn_label = \"Left Thumb\"\n"
       labels_def+="input_r3_btn_label = \"Right Thumb\"\n"
       labels_def+="input_l_x_plus_axis_label = \"Left Analog X+\"\n"
       labels_def+="input_l_x_minus_axis_label = \"Left Analog X-\"\n"
       labels_def+="input_l_y_plus_axis_label = \"Left Analog Y+\"\n"
       labels_def+="input_l_y_minus_axis_label = \"Left Analog Y-\"\n"
       labels_def+="input_r_x_plus_axis_label = \"Right Analog X+\"\n"
       labels_def+="input_r_x_minus_axis_label = \"Right Analog X-\"\n"
       labels_def+="input_r_y_plus_axis_label = \"Right Analog Y+\"\n"
       labels_def+="input_r_y_minus_axis_label = \"Right Analog Y-\"\n"
       labels_def+="input_menu_toggle_btn_label = \"Y\"\n"
    elif [ $(isPsxController "$device_name" "$device_guid") -eq 1 ]; then
       labels_def+="input_b_btn_label = \"Cross\"\n"
       labels_def+="input_y_btn_label = \"Square\"\n"
       labels_def+="input_select_btn_label = \"Select/Share\"\n"
       labels_def+="input_start_btn_label = \"Start/Options\"\n"
       labels_def+="input_a_btn_label = \"Circle\"\n"
       labels_def+="input_x_btn_label = \"Triangle\"\n"
       labels_def+="input_l_btn_label = \"L1\"\n"
       labels_def+="input_r_btn_label = \"R1\"\n"
       if [ $is_trigger_axis - eq 1 ]; then
          labels_def+="input_l2_axis_label = \"L2\"\n"
          labels_def+="input_r2_axis_label = \"R2\"\n"
       else
          labels_def+="input_l2_btn_label = \"L2\"\n"
          labels_def+="input_r2_btn_label = \"R2\"\n"
       fi
       labels_def+="input_l3_btn_label = \"L3\"\n"
       labels_def+="input_r3_btn_label = \"R3\"\n"
       labels_def+="input_l_x_plus_axis_label = \"Left Analog Right\"\n"
       labels_def+="input_l_x_minus_axis_label = \"Left Analog Left\"\n"
       labels_def+="input_l_y_plus_axis_label = \"Left Analog Down\"\n"
       labels_def+="input_l_y_minus_axis_label = \"Left Analog Up\"\n"
       labels_def+="input_r_x_plus_axis_label = \"Right Analog Right\"\n"
       labels_def+="input_r_x_minus_axis_label = \"Right Analog Left\"\n"
       labels_def+="input_r_y_plus_axis_label = \"Right Analog Down\"\n"
       labels_def+="input_r_y_minus_axis_label = \"Right Analog Up\"\n"
       labels_def+="input_menu_toggle_btn_label = \"Triangle\"\n"
    fi

    labels_def+="input_up_btn_label = \"D-Pad Up\"\n"
    labels_def+="input_down_btn_label = \"D-Pad Down\"\n"
    labels_def+="input_left_btn_label = \"D-Pad Left\"\n"
    labels_def+="input_right_btn_label = \"D-Pad Right\"\n"

    for raconfig in "${ra_config_paths[@]}"
    do
        local ra_autoconfig_file="${raconfig}/autoconfig/udev/${device_name}.cfg"
        #print_log $LOG_FILE "INFO" "Executing 'update_device_buttons_labels_libretro()' - Removing previous mapping '$ra_autoconfig_file'"
        # remove previous mapping
        [ -f "$ra_autoconfig_file" ] && sudo rm -f "$ra_autoconfig_file"
        touch "$ra_autoconfig_file"
        sudo chmod 666 "$ra_autoconfig_file"
        echo -e "$labels_def" >> "$ra_autoconfig_file"
    done
}

update_device_name_mapping_libretro()
{
    local device_id=$1
    local device_name="$2"
    local device_guid="$3"
    #print_log $LOG_FILE "INFO" "Executing 'update_device_name_mapping_libretro()' - ID: '$device_id', name: '$device_name', GUID: '$device_guid'"

    local ra_config_paths=( "$RA_CONFIG_FOLDER" "$RA32_CONFIG_FOLDER" )

    #print_log $LOG_FILE "INFO" "Executing 'update_device_name_mapping_libretro()' - Adding retroarch device_name configuration for '$device_name', GUID: '$device_guid'"

    local controller_info="$(do_get_es_controller_declaration "${device_guid}")"
    if [ -z "$controller_info" ]; then
       #print_log $LOG_FILE "WARNING" "Executing 'update_device_name_mapping_libretro()' - ES input config is empty, skipping..."
       return
    fi

    #local is_default_controller="$(echo "$controller_info" | grep "deviceDefault=\"true\"")"

    local vendor="$(cat /proc/bus/input/devices | grep -B1 -m 1 "${device_name}" | grep -o -P -w 'Vendor=\K[^ ]+')"
    vendor="$((0x${vendor}))"
    local product="$(cat /proc/bus/input/devices | grep -B1 -m 1 "${device_name}" | grep -o -P -w 'Product=\K[^ ]+')"
    product="$((0x${product}))"

    local device_def="# ArkOS\n"
    device_def+="input_driver = \"udev\"\n"
    device_def+="input_device = \"${device_name}\"\n"
    device_def+="input_vendor_id = \"${vendor}\"\n"
    device_def+="input_product_id = \"${product}\"\n\n"

    local controler_def=""
    local hotkey_value=""
    local is_trigger_axis=0

    # Printf '%s\n' "$var" is necessary because printf '%s' "$var" on a
    # variable that doesn't end with a newline then the while loop will
    # completely miss the last line of the variable.
    while IFS= read -r input_line
    do
        #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping_libretro()' - input line '$input_line'"
        [[ "$input_line" != *"<input "* ]] && continue

        input_name="$(echo "${input_line}" | grep -o -P -w 'name="\K[^"]+')"
        input_type="$(echo "${input_line}" | grep -o -P -w 'type="\K[^"]+')"
        input_id="$(echo "${input_line}" | grep -o -P -w 'id="\K[^"]+')"
        input_value="$(echo "${input_line}" | grep -o -P -w 'value="\K[^"]+')"
        #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping_libretro()' - input data: name=[$input_name], type=[$input_type], id=[$input_id], value=[$input_value]"

        if [ -z "$input_value" ]; then
          #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping_libretro()' - ES button '${input_name}' is empty, skipping..."
          continue;
        fi

        local lr_button="${LIBRETRO_BUTTON[$input_name]}"
        if [ -z "$lr_button" ]; then
          #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping_libretro()' - libretro button for '${input_name}' is empty, skipping..."
          continue;
        fi

        if [[ "$lr_button" = "input_l2_"* ]] || [[ "$lr_button" = "input_r2_"* ]]; then
           if [ "$input_type" = "axis" ]; then
              lr_button="${lr_button/btn/axis}"
              is_trigger_axis=1
           fi
        fi

        controler_def+="${lr_button} = "

        if [ "$input_type" = "hat" ]; then
            #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping_libretro()' - config HAT '$input_name = h${input_id}${input_name}'"
            controler_def+="\"h${input_id}${input_name}\""
        elif [ "$input_type" = "axis" ]; then

           local axis="-"
           local reverse_axis="+"
           local extra_axis_controler_def=""

           if [[ "${input_value}" == *"+"* ]]; then
              axis="+"
              reverse_axis="-"
           fi

           #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping_libretro()' - config AXIS '$input_name = $axis$input_id'"
           controler_def+="\"${axis}${input_id}\""

           if [[ "$input_name" == "joystick"* ]]; then
              if [[ "$input_name" == *"1up" ]]; then
                 #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping_libretro()' - config AXIS 'joystick1down' as 'input_l_y_plus_axis = $reverse_axis$input_id'"
                 controler_def+="\input_l_y_plus_axis = \"${reverse_axis}${input_id}\""
              elif [[ "$input_name" == *"2up" ]]; then
                 #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping_libretro()' - config AXIS 'joystick2down' as 'input_r_y_plus_axis = $reverse_axis$input_id'"
                 controler_def+="\input_r_y_plus_axis = \"${reverse_axis}${input_id}\""
              elif [[ "$input_name" == *"1left" ]]; then
                 #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping_libretro()' - config AXIS 'joystick1right' as 'input_l_x_plus_axis = $reverse_axis$input_id'"
                 controler_def+="\input_l_x_plus_axis = \"${reverse_axis}${input_id}\""
              elif [[ "$input_name" == *"2left" ]]; then
                 #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping_libretro()' - config AXIS 'joystick2right' as 'input_r_x_plus_axis = $reverse_axis$input_id'"
                 controler_def+="\input_r_x_plus_axis = \"${reverse_axis}${input_id}\""
              fi
           fi

        else # [ "$input_type" = "button" ]; then

            if [[ "$input_name" == "hotkey"* ]]; then
                hotkey_value="$input_value"
                #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping_sdl()' - Skip '${input_name}', hotkey button, value '$input_value'"
                continue;
            fi

            #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping_libretro()' - config BUTTON '$input_name = $input_value'"
            controler_def+="\"${input_value}\""

        fi

        controler_def+="\n"

    done < <(printf '%s\n' "$controller_info")

    if [ ! -z "$hotkey_value" ] && [[ $(echo "$controler_def" | grep -c "\"${hotkey_value}\"" ) -eq 0 ]; then
        #print_log $LOG_FILE "INFO" "Executing 'update_device_name_mapping_libretro()' - Adding 'input_hotkey_btn' button with value '$hotkey_value'"
        controler_def+="${LIBRETRO_BUTTON[hotkey]} = \"${hotkey_value}\"\n"
    fi
    device_def+="\n${controler_def}"

    for raconfig in "${ra_config_paths[@]}"
    do
        local ra_autoconfig_file="${raconfig}/autoconfig/udev/${device_name}.cfg"
        #print_log $LOG_FILE "INFO" "Executing 'update_device_name_mapping_libretro()' - Removing previous mapping '$ra_autoconfig_file'"
        # remove previous mapping
        [ -f "$ra_autoconfig_file" ] && sudo rm -f "$ra_autoconfig_file"
        touch "$ra_autoconfig_file"
        sudo chmod 666 "$ra_autoconfig_file"
        echo -e "$device_def" > "$ra_autoconfig_file"
    done

    update_device_buttons_labels_libretro "$device_name" "$device_guid" $is_trigger_axis

    #print_log $LOG_FILE "INFO" "Exit 'update_device_name_mapping_libretro()' - ID: '$device_id', name: '$device_name', GUID: '$device_guid'"
}

updata_system_gamecontrollerdb_files()
{
    #print_log $LOG_FILE "INFO" "Executing 'updata_system_gamecontrollerdb_files()'"

    local directories=( "/roms" "/roms2" "/opt" "/home/ark" )

    for directory in "${directories[@]}"
    do
      if [ -d "$directory" ]; then
        #print_log $LOG_FILE "DEBUG" "Executing 'updata_system_gamecontrollerdb_files()' - roms_dir: '$directory'"

        while IFS= read -r file; do
            #print_log $LOG_FILE "DEBUG" "Executing 'updata_system_gamecontrollerdb_files()' - file: '$file'"
            if [ -z "$file" ] || [ ! -e "$file" ]; then
                #print_log $LOG_FILE "DEBUG" "Executing 'updata_system_gamecontrollerdb_files()' - empty or does not exist file, skipping ..."
                continue
            fi

            [ "$file" = "$SDL_GAMECONTROLLERDB_FILE" ] && continue
            [[ "$file" == *"/opt/system/Tools/"* ]] && continue

            if [[ "$file" == "/roms"* ]]; then
                #print_log $LOG_FILE "DEBUG" "Executing 'updata_system_gamecontrollerdb_files()' - cloning file '$SDL_GAMECONTROLLERDB_FILE' on file '$file'"
                sudo rm -f "${file}.save"
                sudo mv -f "$file" "${file}.save"
                sudo cp -f "$SDL_GAMECONTROLLERDB_FILE" "$file"
                sudo chmod 666 "$file"
            else
                if [ ! -L "$file" ]; then
                   #print_log $LOG_FILE "DEBUG" "Executing 'updata_system_gamecontrollerdb_files()' - creating link of file '$SDL_GAMECONTROLLERDB_FILE' on path '$file'"
                   sudo rm -f "${file}.save"
                   sudo mv -f "$file" "${file}.save"
                   sudo ln -sf "$SDL_GAMECONTROLLERDB_FILE" "$file"
                elif [ "$(readlink $file)" != "$SDL_GAMECONTROLLERDB_FILE" ]; then
                   #print_log $LOG_FILE "DEBUG" "Executing 'updata_system_gamecontrollerdb_files()' - creating link of file '$SDL_GAMECONTROLLERDB_FILE' on path '$file'"
                   sudo ln -sf "$SDL_GAMECONTROLLERDB_FILE" "$file"
                fi
            fi
        done <<< "$(sudo find "$directory" -name gamecontrollerdb.txt 2> /dev/null)"

      fi
    done

    #print_log $LOG_FILE "INFO" "Exit 'updata_system_gamecontrollerdb_files()'"
}

update_device_name_mapping_sdl()
{
    local device_id=$1
    local device_name="$2"
    local device_guid="$3"
    #print_log $LOG_FILE "INFO" "Executing 'update_device_name_mapping_sdl()' - ID: '$device_id', name: '$device_name', GUID: '$device_guid'"

    local controller_info="$(do_get_es_controller_declaration "${device_guid}")"
    if [ -z "$controller_info" ]; then
       #print_log $LOG_FILE "WARNING" "Executing 'update_device_name_mapping_sdl()' - ES input config is empty, skipping..."
       return
    fi

    #print_log $LOG_FILE "INFO" "Executing 'update_device_name_mapping_sdl()' - Adding SDL device_name configuration for '$device_name', GUID: '$device_guid' on '$SDL_GAMECONTROLLERDB_FILE' file"
    #local is_default_controller="$(echo "$controller_info" | grep "deviceDefault=\"true\"")"

    local device_def="${device_guid},${device_name},"

    local controler_def=""
    local hotkey_value=""

    # Printf '%s\n' "$var" is necessary because printf '%s' "$var" on a
    # variable that doesn't end with a newline then the while loop will
    # completely miss the last line of the variable.
    while IFS= read -r input_line
    do
        #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping_sdl()' - input line '$input_line'"
        [[ "$input_line" != *"<input "* ]] && continue

        input_name="$(echo "${input_line}" | grep -o -P -w 'name="\K[^"]+')"
        input_type="$(echo "${input_line}" | grep -o -P -w 'type="\K[^"]+')"
        input_id="$(echo "${input_line}" | grep -o -P -w 'id="\K[^"]+')"
        input_value="$(echo "${input_line}" | grep -o -P -w 'value="\K[^"]+')"
        #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping_sdl()' - input data: name=[$input_name], type=[$input_type], id=[$input_id], value=[$input_value]"

        if [ -z "$input_value" ]; then
          #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping_sdl()' - Skip '${input_name}', empty value"
          continue;
        fi

        local sdl_button="${SDL_BUTTON[$input_name]}"
        if [ -z "$sdl_button" ]; then
          #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping_sdl()' - SDL button for '${input_name}' is empty, skipping..."
          continue;
        fi

        controler_def+="${sdl_button}:"

        if [ "$input_type" = "hat" ]; then
            #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping_sdl()' - config HAT '$input_name = h${input_id}.${input_value}'"
            controler_def+="h${input_id}.${input_value}"
        elif [ "$input_type" = "axis" ]; then
            #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping_sdl()' - config AXIS '$input_name = a$input_id'"
            controler_def+="a${input_id}"
        else # [ "$input_type" = "button" ]; then
            #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping_sdl()' - config BUTTON '$input_name = b$input_value'"

            if [[ "$input_name" == "hotkey"* ]]; then
                hotkey_value="$input_value"
                #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping_sdl()' - Skip '${input_name}', hotkey button, value '$input_value'"
                continue;
            fi

            controler_def+="b${input_value}"
        fi

        controler_def+=","

    done < <(printf '%s\n' "$controller_info")

    if [ ! -z "$hotkey_value" ] && [[ $(echo "$controler_def" | grep -c ":b${hotkey_value}," ) -eq 0 ]; then
        #print_log $LOG_FILE "INFO" "Executing 'update_device_name_mapping_sdl()' - Adding 'guide' button with value '$hotkey_value'"
        controler_def+="${SDL_BUTTON[hotkey]}:b${hotkey_value},"
    fi

    device_def+="${controler_def}platform:Linux,"

    #print_log $LOG_FILE "INFO" "Executing 'update_device_name_mapping_sdl()' - Searching SDL device_name configuration for '$device_name', GUID: '$device_guid' on '$SDL_GAMECONTROLLERDB_FILE' file"
    if [[ $(cat "$SDL_GAMECONTROLLERDB_FILE" | grep -c $device_guid) -ge 1 ]]; then
        # remove previous mapping
        #print_log $LOG_FILE "INFO" "Executing 'update_device_name_mapping_sdl()' - Removing previous mapping of device with '$device_guid' GUID on '$SDL_GAMECONTROLLERDB_FILE' file"
        sudo sed -i -e "/${device_guid}.*$/d" "$SDL_GAMECONTROLLERDB_FILE" > /dev/null
    fi

    #print_log $LOG_FILE "INFO" "Executing 'update_device_name_mapping_sdl()' - Adding SDL device_name configuration for '$device_name', GUID: '$device_guid' on '$SDL_GAMECONTROLLERDB_FILE' file, value: '$device_def'"
    sudo sed -i -e '$a'"$device_def"'' "$SDL_GAMECONTROLLERDB_FILE" > /dev/null

    updata_system_gamecontrollerdb_files

    #print_log $LOG_FILE "INFO" "Exit 'update_device_name_mapping_sdl()' - ID: '$device_id', name: '$device_name', GUID: '$device_guid'"
}

do_config_controllers()
{
    #print_log $LOG_FILE "INFO" "Executing 'do_config_controllers()' - ID: '$device_id', name: '$device_name', GUID: '$device_guid'"

    update_device_name_mapping_libretro "$1" "$2" "$3" &
    update_device_name_mapping_sdl "$1" "$2" "$3" &

    #print_log $LOG_FILE "DEBUG" "Executing 'do_config_controllers()' - waiting asynchonous calls"
    wait
    #print_log $LOG_FILE "INFO" "Exiting 'do_config_controllers()' - ID: '$device_id', name: '$device_name', GUID: '$device_guid'"
}

# ******************************************************************************************

# Configure ES commons variables
. /usr/local/bin/es-log_scripts

LOG_FILE="$ES_SCRIPT_LOGS_DIR/control-mapped.log"

#if [ -f "$LOG_FILE" ]; then
#  mv "$LOG_FILE" "$LOG_FILE.bak"
#fi

exit_execution()
{
    local return=$1
    local param1=$2
    local param2=$3
    local param3=$4
    #print_log $LOG_FILE "INFO" "##### Exit executing control-mapped: '$param1' '$param2' '$param3', exit code: '$return' #####"
    exit $return
}

if [ $# -lt 3 ]; then
  do_help
  exit 1
fi

#print_log $LOG_FILE "INFO" "##### Executing control-mapped: '$1 $2 $3' #####"

do_config_controllers "$1" "$2" "$3" &

exit_execution 0 "$1" "$2" "$3"
