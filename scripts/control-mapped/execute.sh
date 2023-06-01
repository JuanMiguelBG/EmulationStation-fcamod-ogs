#!/bin/bash

do_help() {
    echo "$0 <DEVICE_ID> <DEVICE_NAME> <DEVICE_GUID>" >&2
}

get_ra_turbo_button_property()
{
    local es_turbo_btn="$(get_es_settings_property 'TurboHotkeyButton')"
    #print_log $LOG_FILE "DEBUG" "Exit 'get_ra_turbo_button_property()' - ES turbo button value: '$es_turbo_btn'"

    case "${es_turbo_btn}" in
        "LeftShoulder")
            echo "input_l_btn"
        ;;
        "RightShoulder")
            echo "input_r_btn"
        ;;
        "LeftTrigger")
            echo "input_l2_btn"
        ;;
        "RightTrigger")
            echo "input_r2_btn"
        ;;
        "LeftThumb")
            echo "input_l3_btn"
        ;;
        "RightThumb")
            echo "input_r3_btn"
        ;;
        "A")
            echo "input_a_btn"
        ;;
        "B")
            echo "input_b_btn"
        ;;
        "X")
            echo "input_x_btn"
        ;;
        "Y")
            echo "input_y_btn"
        ;;
        *)
            echo ""
    esac
}

update_device_name_mapping()
{
    local device_id=$1
    local device_name=$2
    local device_guid=$3
    #print_log $LOG_FILE "INFO" "Executing 'update_device_name_mapping()' - ID: '$device_id', name: '$device_name', GUID: '$device_guid'"

    local es_input_file_location="$ES_ETC_PATH/es_input.cfg"
    local ra_config_paths=( "$RA_CONFIG_FOLDER" "$RA32_CONFIG_FOLDER" )

    #print_log $LOG_FILE "INFO" "Executing 'update_device_name_mapping()' - Adding retroarch device_name configuration for '$device_name'"

    # remove previous mapping
    #print_log $LOG_FILE "INFO" "Executing 'update_device_name_mapping()' - Removing previous mapping"
    for raconfigcreate in "${ra_config_paths[@]}"
    do
      if [[ -f "${raconfigcreate}/autoconfig/udev/${device_name}.cfg" ]]; then
        sudo rm -f "${raconfigcreate}/autoconfig/udev/${device_name}.cfg"
      fi
    done

    #local is_default_controller="$(tac ${es_input_file_location} | grep "${device_name}" -m 1 -B 9999 | tac  | grep "deviceDefault=\"true\"")"

    local button_list=( left \
                        right \
                        up \
                        down \
                        a \
                        b \
                        x \
                        y \
                        leftshoulder \
                        pageup \
                        rightshoulder \
                        pagedown \
                        lefttrigger \
                        l2 \
                        righttrigger \
                        r2 \
                        leftthumb \
                        l3 \
                        rightthumb \
                        r3 \
                        select \
                        start \
                        leftanalogdown \
                        leftanalogup \
                        joystick1up \
                        leftanalogleft \
                        joystick1left \
                        leftanalogright \
                        rightanalogdown \
                        rightanalogup \
                        joystick2up \
                        rightanalogleft \
                        joystick2left \
                        rightanalogright )


    for button in "${button_list[@]}"
    do
        local controler_info="$(tac ${es_input_file_location} | grep "${device_name}" -m 1 -B 9999 | tac)"
        local isitahatoranalog="$(echo "${controler_info}" | grep "name=\"${button}\"" | grep -o -P -w "type=.{0,6}" | cut -c5- | cut -d '"' -f2)"

        unset value
        unset axis
        unset reverse_axis

        if [[ "$isitahatoranalog" == "hat" ]]; then
          value="hat found"
        elif [[ "$isitahatoranalog" == "axis" ]]; then
          axis="$(echo "${controler_info}" | grep "name=\"${button}\"" | grep -o -P -w "value=.{0,5}" | cut -c5- | cut -d '"' -f2)"
          if [[ "${axis}" == *"-"* ]]; then
            axis="-"
            reverse_axis="+"
          else
            axis="+"
            reverse_axis="-"
          fi
        fi

        get_button="$(echo "${controler_info}" | grep "name=\"${button}\"" | grep -o -P -w 'id="\K[^"]+')"

        if test -z "$get_button"; then
          #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping()' - Skip '${button}', empty value"
          continue;
        fi

        #let a="${#button} + 3"
        #test_button="$(timeout 0.1s sdljoytest | grep 'retrogame_joypad,')"
        #test_buttons="$(echo ${test_button}_button | tr , '\n' | grep -o -P -w "${button}:.{0,4}" | cut -c${a}-)"

        if test -z "$axis"; then
          if test ! -z "$value"; then
            #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping()' - $button = h${get_button}${button}"
            retropad+=("h${get_button}${button}")
          else
            #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping()' - $button = $get_button"
            retropad+=("${get_button}")
          fi
        else
          if [[ "$button" == "joystick1up" ]]; then
            #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping()' - joystick1down = $reverse_axis$get_button"
            retropad+=("${reverse_axis}${get_button}")
          elif [[ "$button" == "joystick2up" ]]; then
            #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping()' - joystick2down = $reverse_axis$get_button"
            retropad+=("${reverse_axis}${get_button}")
          fi

          #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping()' - $button = $axis$get_button"
          retropad+=("${axis}${get_button}")

          if [[ "$button" == "joystick1left" ]]; then
            #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping()' - joystick1right = $reverse_axis$get_button"
            retropad+=("${reverse_axis}${get_button}")
          elif [[ "$button" == "joystick2left" ]]; then
            #print_log $LOG_FILE "DEBUG" "Executing 'update_device_name_mapping()' - joystick2right = $reverse_axis$get_button"
            retropad+=("${reverse_axis}${get_button}")
          fi
        fi
    done

    local vendor="$(cat /proc/bus/input/devices | grep -B1 -m 1 "${device_name}" | grep -o -P -w 'Vendor=\K[^ ]+')"
    vendor="$((0x${vendor}))"
    local product="$(cat /proc/bus/input/devices | grep -B1 -m 1 "${device_name}" | grep -o -P -w 'Product=\K[^ ]+')"
    product="$((0x${product}))"

    let i=0
    for raconfigcreate in "${ra_config_paths[@]}"
    do
        local ra_config_device_file="${raconfigcreate}/autoconfig/udev/${device_name}".cfg
        echo "# ArkOS" > ${ra_config_device_file}
        echo "input_driver = \"udev\"" >> ${ra_config_device_file}
        echo "input_device = \"${device_name}\"" >> ${ra_config_device_file}
        echo "input_vendor_id = \"${vendor}\"" >> ${ra_config_device_file}
        echo "input_product_id = \"${product}\"" >> ${ra_config_device_file}
    done

    local ra_turbo_button_property="$(get_ra_turbo_button_property)"
    #print_log $LOG_FILE "DEBUG" "Exit 'update_device_name_mapping()' - turbo button RA porperty: '$ra_turbo_button_property'"

    for retroA in "${retropad[@]}"
    do
        local retropad_list=( input_left_btn \
                              input_right_btn \
                              input_up_btn \
                              input_down_btn \
                              input_a_btn \
                              input_b_btn \
                              input_x_btn \
                              input_y_btn \
                              input_l_btn \
                              input_r_btn \
                              input_l2_btn \
                              input_r2_btn \
                              input_l3_btn \
                              input_r3_btn \
                              input_select_btn \
                              input_start_btn \
                              input_l_y_plus_axis \
                              input_l_y_minus_axis \
                              input_l_x_minus_axis \
                              input_l_x_plus_axis \
                              input_r_y_plus_axis \
                              input_r_y_minus_axis \
                              input_r_x_minus_axis \
                              input_r_x_plus_axis )

        for raconfig in "${ra_config_paths[@]}"
        do
            if [ ! -z "${retroA}" ]; then
                echo "${retropad_list[$i]} = \"${retroA}\"" >> "${raconfig}/autoconfig/udev/${device_name}.cfg"
                if [ ! -z "${ra_turbo_button_property}" ] && [ "${retropad_list[$i]}" = "${ra_turbo_button_property}" ]; then
                    echo "input_turbo_btn = \"${retroA}\"" >> "${raconfigcreate}/autoconfig/udev/${device_name}".cfg
                fi
            fi
        done
        let i++
    done

    #print_log $LOG_FILE "INFO" "Exit 'update_device_name_mapping()' - ID: '$device_id', name: '$device_name', GUID: '$device_guid'"
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

update_device_name_mapping "$1" "$2" "$3" &

exit_execution 0 "$1" "$2" "$3"
