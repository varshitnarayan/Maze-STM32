# Maze-STM32

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

