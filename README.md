# Zephyr RTOS Mouse Control via HOG

## Overview
This project utilizes Zephyr RTOS to create a system that controls the mouse of a host computer using the Human Interface Device (HID) Over GATT (HOG) protocol. When the device is plugged into the host computer, it reads data from the serial port and translates it into mouse movements.

## Dependencies
- Zephyr RTOS: The project relies on Zephyr RTOS to handle tasks, threads, and device interactions.
- nrfutils
- Device with USB and BLE support from Zephyr

## Code Structure
The codebase is organized into the following files and directories:

1. `main.c`: Entry point of the application. Initializes peripherals and starts the main control loop.
2. `hog.c`: Contains the initialization for BLE and translates it into mouse movements.
3. `hog.h`: Header file declaring functions and structures used in `mouse_control.c`.
4. `CMakeLists.txt`: CMake configuration file for building the project with Zephyr RTOS.

## Installation and Setup
Note: For this setup, I have used the nRF52840 Dongle
1. Clone the repository to your development environment.
2. Navigate to the project directory.
3. Build by `` west build --build-dir /build  --pristine --board nrf52840dongle_nrf52840 ``
4. Zip the build by `` nrfutil pkg generate --hw-version 52 --sd-req=0x00         --application build/zephyr/zephyr.hex         --application-version 1 usb_hid.zip ``
5. Flash the compiled binary to your device by `` nrfutil dfu usb-serial -pkg usb_hid.zip -p /dev/ttyACM0 ``

## Usage
1. Connect the device to the host computer via USB.
2. Ensure that the host computer recognizes the device.
3. Run the compiled binary on the device.
4. The device will start reading data from the serial port and controlling the mouse accordingly.

## Contributions
Contributions to this project are welcome! Feel free to submit bug reports, feature requests, or pull requests via GitHub.

## Acknowledgments
Special thanks to the Zephyr RTOS community for their support and contributions.
