# Maze-STM32


# Maze-STM32: Tilt-Controlled Labyrinth Game

An embedded systems project that brings a classic ball-in-a-maze game to life using the **STM32F429I-DISC1** development board. Instead of traditional button or touch controls (as the board's display is a standard non-touch LCD), this game is fully controlled by physically tilting the board, using real-time data from an **MPU6050** sensor.

## 🚀 Features
* **Motion-Based Physics:** Utilizes the MPU6050 (Accelerometer & Gyroscope) to calculate pitch and roll, translating real-world board tilt into in-game ball movement.
* **Real-Time Rendering:** Custom LCD drawing functions render the maze walls, the goal, and the moving ball smoothly on the STM32F429I-DISC1 display.
* **Collision Detection:** Programmed boundary logic prevents the ball from passing through maze walls.
* **Hardware Abstraction:** Built using STM32 HAL (Hardware Abstraction Layer) libraries for efficient I2C communication and peripheral management.

## 🛠️ Hardware Requirements
* **STM32F429I-DISC1** Discovery Board
* **MPU6050** 6-axis Accelerometer and Gyroscope Module
* Jumper wires for I2C connections

> **Note:** The project runs entirely via the board's primary USB power/programming port; no secondary power cables are required.

## 💻 Software & Tools
* C / C++
* STM32 HAL Drivers
* System Workbench for STM32 (SW4STM32) / STM32CubeIDE

## 🔌 Wiring Guide (MPU6050 to STM32)
Ensure your I2C pins are connected correctly before powering on the board:
* **VCC** -> 3.3V (or 5V depending on your specific MPU module)
* **GND** -> GND
* **SCL** -> I2C SCL Pin (e.g., PB8)
* **SDA** -> I2C SDA Pin (e.g., PB9)

*(Check `mpu6050.h` or your `.ioc` configuration to verify the exact I2C pins used in this build).*

## ⚙️ How to Build and Run
1. **Clone the repository:**
   ```bash
   git clone [https://github.com/varshitnarayan/Maze-STM32.git](https://github.com/varshitnarayan/Maze-STM32.git)


## FOLDER/FILES DIRECTORIES
```text
Maze-master/
├── README.md
├── .gitignore
├── .project                         # Top-level Eclipse project file
├── docs/
│   └── westworld.png
└── src/
    ├── Drivers/                     # STM32 HAL + CMSIS + BSP drivers
    │   ├── CMSIS/
    │   ├── STM32F4xx_HAL_Driver/
    │   ├── BSP/
    │   └── Utilities/
    │
    └── Projects/
        └── STM32F429I-Discovery/
            └── Examples/
                └── BSP/
                    ├── Inc/         # Header files
                    ├── Src/         # main.c, lcd.c, etc.
                    │
                    └── SW4STM32/
                        └── STM32F429I-Discovery/
                            ├── .project
                            ├── .cproject
                            ├── .settings/
                            ├── STM32F429ZITx_FLASH.ld
                            ├── startup_stm32f429xx.s
                            ├── syscalls.c
                            ├── STM32F429I-Discovery Debug.launch
                            │
                            └── Example/
                                └── User/
                                    ├── main.c        # MPU6050 integration
                                    ├── maze.c
                                    ├── maze.h
                                    ├── mpu6050.c     # Custom MPU6050 driver
                                    └── mpu6050.h
```

## Key Source Files Summary

| File         | Purpose |
|--------------|---------|
| `main.c`     | Game loop, MPU6050 tilt reading, ball movement, and collision detection |
| `maze.c`     | Maze generation using recursive carving and LCD rendering |
| `maze.h`     | Function declarations and maze-related definitions |
| `mpu6050.c`  | I2C driver for MPU6050 including initialization and sensor data reading (`Ax`, `Ay`, `Az`, `Gx`, `Gy`, `Gz`) |
| `mpu6050.h`  | MPU6050 register definitions and sensor data structures |

