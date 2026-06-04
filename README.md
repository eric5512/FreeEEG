# FreeEEG

FreeEEG is a low-cost, open-source electroencephalography (EEG) acquisition platform designed for educational, experimental, and research-oriented applications. The project aims to provide an accessible and modifiable EEG system for researchers, students, and hobbyists who need direct control over the acquisition hardware, firmware, and data interface.

The platform is based on the **Texas Instruments ADS1299** analog front-end and an **ESP32-S3** microcontroller. It supports multichannel EEG acquisition, real-time wireless data transmission, onboard storage, and motion sensing through an integrated IMU.

> **Safety notice:** FreeEEG is not a medical device. It has not been medically certified and must not be used for clinical diagnosis, treatment, or any safety-critical medical application.

## Project Overview

The system was developed as a Master Thesis project at the Institut d'Investigacions Biomèdiques August Pi i Sunyer (IDIBAPS). The main objective is to create an open-source and open-hardware EEG platform that is cost-effective, configurable, and easy to adapt to different experimental scenarios.

The current prototype integrates:

- 8-channel EEG acquisition
- ADS1299 24-bit analog front-end
- ESP32-S3 microcontroller
- WiFi access point mode
- UDP real-time data streaming
- HTTP-based configuration interface
- SD card storage through SDIO
- MPU6050 accelerometer and gyroscope
- Battery-powered operation
- Python client library for receiving and decoding data

## Repository Structure

```text
FreeEEG/
├── Client/
│   └── Python library and tools for interacting with the device
│
├── Electronics/
│   └── KiCad schematic, PCB layout, and manufacturing files
│
├── Firmware/
│   └── ESP32-S3 firmware based on ESP-IDF and FreeRTOS
│
└── README.md
```

## Hardware Architecture

The hardware is centered around the ADS1299, an 8-channel, 24-bit analog front-end designed for EEG and biopotential measurements. The ESP32-S3 handles acquisition control, wireless communication, SD card storage, configuration, and system management.

Main hardware components:

| Subsystem | Component |
|---|---|
| Analog front-end | ADS1299 |
| Microcontroller | ESP32-S3 |
| IMU | MPU6050 |
| Storage | microSD card |
| Wireless communication | ESP32-S3 WiFi |
| Configuration interface | HTTP server |
| Real-time streaming | UDP |
| PCB design tool | KiCad |

The PCB prototype is a two-layer board of approximately 55 mm × 90 mm. The design separates sensitive analog circuitry from noisy digital and switching sections, places the ADS1299 close to the electrode connector, and includes RC low-pass input filtering for each EEG channel.

> **Warning:** The PCB still presents some issues in its current form. More modifications need to be introduced to release the first fully functional version

## Firmware

The firmware is developed using **ESP-IDF** and **FreeRTOS**. It is organized into concurrent tasks to separate time-critical acquisition from communication, storage, and configuration logic.

Main firmware functions:

- ADS1299 initialization and register configuration
- EEG acquisition synchronized with the ADS1299 `DRDY` signal
- IMU acquisition over I2C
- UDP packet transmission in online mode
- CSV data logging to SD card in offline mode
- HTTP configuration server
- Mode selection through a physical switch

The firmware supports two operating modes:

### Online Mode

In online mode, the ESP32-S3 creates a WiFi access point and waits for UDP commands from the client. EEG and IMU data are transmitted in real time using UDP packets.

### Offline Mode

In offline mode, acquisition starts automatically and data are stored locally on the SD card in CSV format. WiFi is disabled to reduce power consumption.

## Client Library

The `Client/` folder contains the Python interface used to communicate with the device. The client library provides a higher-level interface for:

- Connecting to the FreeEEG device
- Sending start and stop commands
- Receiving UDP data packets
- Decoding EEG, IMU, and event data
- Exporting samples to CSV
- Building custom visualization or processing tools

Basic usage example:

```python
from freeeeg import FreeEEG
import signal

if __name__ == "__main__":
    eeg = FreeEEG()

    def signal_handler(sig, frame):
        f.close()
        eeg.stop()
        print("Stopping measurements")
        exit(0)

    signal.signal(signal.SIGINT, signal_handler)

    data_stream = eeg.get_data_generator(blocking=True)

    f = open("test.csv", "w+")
    eeg.start()

    print("Starting EEG...")
    f.write("timestamp,e1,e2,e3,e4,e5,e6,e7,e8,ax,ay,az,gx,gy,gz,event\n")

    for sample in data_stream():
        f.write(sample.as_csv() + "\n")
```

## Electronics

The `Electronics/` folder contains the KiCad project files needed to inspect, modify, and manufacture the PCB.

This folder may include:

- KiCad schematic files
- KiCad PCB layout files
- Symbol and footprint libraries
- Gerber files
- Drill files
- Bill of materials
- Assembly files

The current PCB revision is an engineering prototype. Before manufacturing or using the board, review the schematic, layout, footprints, and known issues.

## Current Prototype Status

The current version should be considered a functional engineering prototype. Several subsystems have been validated, including ADS1299 communication, EEG acquisition using the evaluation platform, WiFi access point creation, UDP transmission, HTTP configuration, and Python packet decoding.

Preliminary validation demonstrated:

- EEG acquisition with dry electrodes
- Resting-state EEG activity with a visible alpha-band peak around 10 Hz
- Shorted-input RMS noise around 0.2 µV RMS
- Stable UDP transmission at approximately 100 packets/s
- Packet loss at higher packet rates around 250 packets/s
- Approximate current consumption of 110 mA in online mode and 60 mA in offline mode
- Indoor wireless range around 10 m

## Known Limitations

The current prototype has several limitations:

- Not medically certified
- Not intended for clinical or diagnostic use
- Firmware validation still ongoing
- First PCB revision required manual correction of reset/boot button circuitry
- No integrated electrode impedance measurement
- No dedicated external synchronization connector in the current revision
- Dry electrodes are more sensitive to motion artifacts
- Long-duration validation is still required

## Planned Improvements

Future development may include:

- Complete firmware validation
- Full integration testing
- Improved PCB revision
- Corrected reset and boot circuitry
- I2C pull-up resistors for the IMU
- External synchronization connector
- Rechargeable battery management
- Enclosure design
- Electrode impedance measurement
- Improved analog filtering
- Bluetooth or mobile interface support
- Real-time visualization tools
- Signal processing and neurofeedback utilities

## Development Requirements

### Firmware

Recommended tools:

- ESP-IDF
- FreeRTOS
- ESP32-S3 toolchain
- Python tools for flashing and monitoring

### Electronics

Recommended tools:

- KiCad
- Gerber viewer

### Client

Recommended tools:

- Python 3
- NumPy
- Matplotlib
- Standard socket library

Exact Python dependencies should be listed in a future `requirements.txt`.

## Contributing

Contributions are welcome, especially in the following areas:

- Firmware validation
- Python client improvements
- Real-time plotting tools
- PCB review
- Documentation
- Signal processing examples
- Neurofeedback experiments
- Long-duration acquisition testing

When contributing hardware changes, document the PCB revision clearly and include updated manufacturing files.

## Disclaimer

This project is provided for educational, research, and experimental use only. It is not designed, certified, or validated as a medical device. Use only battery-powered operation during human measurements, and follow appropriate electrical safety practices.

The authors and contributors are not responsible for misuse of the hardware, firmware, or software.

## TODO list:
- [ ] Solve the PCB issues.
- [ ] Implement SD card drivers.
- [ ] Implement offline acquisition mode.
- [ ] Add more functionality to the client, like ".as_csv()" method to save data.
- [ ] Add biofeedback example scripts.
- [ ] Clean repo.