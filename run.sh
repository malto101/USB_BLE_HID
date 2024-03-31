#!/bin/bash

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check if West exists
if command_exists "west"; then
    # Check if nrfutil exists
    if command_exists "nrfutil"; then
        # Check if make exists (assuming make is used for building)
        if command_exists "make"; then
            # Run the commands
            west build --build-dir /build --pristine --board nrf52840dongle_nrf52840
            nrfutil pkg generate --hw-version 52 --sd-req=0x00 --application build/zephyr/zephyr.hex --application-version 1 usb_hid.zip
            nrfutil dfu usb-serial -pkg usb_hid.zip -p /dev/ttyACM0
        else
            echo "Error: 'make' command not found. Please install make."
            exit 1
        fi
    else
        echo "Error: 'nrfutil' command not found. Please install nrfutil."
        exit 1
    fi
else
    echo "Error: 'west' command not found. Please install west."
    exit 1
fi
