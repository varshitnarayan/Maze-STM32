#ifndef MPU6050_H
#define MPU6050_H

#include "stm32f4xx_hal.h"

// MPU6050 I2C Address (Shifted by 1 bit left for STM32 HAL)
#define MPU6050_ADDR         (0x68 << 1)

// MPU6050 Registers
#define MPU6050_REG_WHO_AM_I     0x75
#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_SMPLRT_DIV   0x19
#define MPU6050_REG_CONFIG       0x1A
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_GYRO_CONFIG  0x1B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B

/**
  * @brief MPU6050 Data Structure containing Accelerometer and Gyroscope values
  */
typedef struct {
    float Ax;
    float Ay;
    float Az;
    float Gx;
    float Gy;
    float Gz;
} MPU6050_Data;

/* Function Prototypes */
HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef MPU6050_ReadAll(I2C_HandleTypeDef *hi2c, MPU6050_Data *data);

#endif /* MPU6050_H */
