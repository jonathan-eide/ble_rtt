# Ranging with Round Trip Timing using BluetoothLow Energy

This project is created to do ranging measurements between two nRF52840 Development Kits. It is written as a part of a student project at Norges Teknisk-Naturvitenskapelige Universitet (NTNU) in collaboration with Nordic Semiconductor. 

The ranging algorithm is based on Round Trip Timing (RTT) and uses Time of Flight calculations to estimate distance.

## How to use

### Get the necessary files and code
1. Download the nRF5 SDK v16.0.0
2. Create a new folder at SDK_ROOT (example naming: /my_projects)
    - Option 1: Create a new folder inside that folder again (inside /my_projects), an clone this repository into that folder.
    - Option 2: Clone this repository in to /my_projects and change the Makefiles so that the libraries are found.
3. You may need to edit the nRF5_SDK_16.0.0_98a08e2/components/toolchain/gcc/Makefile.posix so that it can find the GNU install root, version and prefix. 

### Flash devices
1. Step in to central/ble_app_blinky_c/pca10056/s140/armgcc
    - Connect the first nRF52840 DK to your computer
    - Run `make FLASH_SOFTDEVICE`
    - Run `make flash`
2.  Step in to peripheral/ble_app_blinky/pca10056/s140/armgcc
    - Connect the first nRF52840 DK to your computer
    - Run `make FLASH_SOFTDEVICE`
    - Run `make flash`

When both DKs are powered on, they should connect. This is indicated by the DKs LED1 switches off and LED2 powers on. By pressing Button 1 on the central device the DKs will start performing RTT measurements. Both the DKs LED3 and LED4 will start blinking to indicate RTT measurements are performed. Also by pressing Button 1 on the central device will make LED3 light up. This can be hard to notice unless the RTT LED blinking is turned off. 

To visualise and print the result one can add NRF_LOG_INFO at the end of the do_rtt_measurements function on the central side. The measurments can then be printed in a terminal window such as Putty.

## License
MIT License Copyright (c) 2020 Martin Aalien