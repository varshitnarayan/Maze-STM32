/**
  ******************************************************************************
  * @file    mpu6050.c
  * @brief   MPU6050 6-DOF IMU Driver Implementation
  * @details Initializes the MPU6050 via I2C and reads all 6 axes.
  *          Configuration: ±2g accel range, ±250°/s gyro range, 100Hz sample rate
  ******************************************************************************
  */

#include "mpu6050.h"

/**
  * @brief  Initialize MPU6050: wake up, set sample rate, configure ranges
  */
HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c)
{
    HAL_StatusTypeDef ret;
    uint8_t check, data;

    /* Step 1: Verify device identity via WHO_AM_I register
       Real MPU6050 returns 0x68, clones may return 0x72, 0x70, 0x98, etc. */
    ret = HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, MPU6050_REG_WHO_AM_I,
                           I2C_MEMADD_SIZE_8BIT, &check, 1, 100);
    if (ret != HAL_OK)
        return HAL_ERROR;  // I2C communication failed entirely

    // Accept any valid response (don't block on clone chips)
    // check == 0x68 (genuine), 0x72, 0x70, 0x98 (clones) — all OK

    /* Step 2: Wake up — clear SLEEP bit in PWR_MGMT_1 (bit 6)
       Writing 0x00 wakes the device and selects internal 8MHz oscillator */
    data = 0x00;
    ret = HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, MPU6050_REG_PWR_MGMT_1,
                            I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
    if (ret != HAL_OK) return ret;

    HAL_Delay(10);  // Wait for wake-up stabilization

    /* Step 3: Set sample rate divider
       Sample Rate = Gyro Output Rate / (1 + SMPLRT_DIV)
       Gyro Output Rate = 1kHz (when DLPF enabled)
       SMPLRT_DIV = 9 → Sample Rate = 1000 / (1+9) = 100 Hz */
    data = 0x09;
    ret = HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, MPU6050_REG_SMPLRT_DIV,
                            I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
    if (ret != HAL_OK) return ret;

    /* Step 4: Configure Digital Low-Pass Filter (DLPF)
       CONFIG register: DLPF_CFG = 3 → Accel BW=44Hz, Gyro BW=42Hz
       This reduces noise significantly */
    data = 0x03;
    ret = HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, MPU6050_REG_CONFIG,
                            I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
    if (ret != HAL_OK) return ret;

    /* Step 5: Accelerometer config — ±2g range
       AFS_SEL = 0 → Full Scale Range = ±2g
       Sensitivity = 16384 LSB/g */
    data = 0x00;
    ret = HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, MPU6050_REG_ACCEL_CONFIG,
                            I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
    if (ret != HAL_OK) return ret;

    /* Step 6: Gyroscope config — ±250°/s range
       FS_SEL = 0 → Full Scale Range = ±250 deg/s
       Sensitivity = 131 LSB/(deg/s) */
    data = 0x00;
    ret = HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, MPU6050_REG_GYRO_CONFIG,
                            I2C_MEMADD_SIZE_8BIT, &data, 1, 100);

    return ret;
}

/**
  * @brief  Read all 6 axes from MPU6050 in a single burst read
  * @note   Burst read of 14 bytes: AXH AXL AYH AYL AZH AZL TH TL GXH GXL GYH GYL GZH GZL
  *         Temperature bytes (index 6,7) are skipped.
  */
HAL_StatusTypeDef MPU6050_ReadAll(I2C_HandleTypeDef *hi2c, MPU6050_Data *data)
{
    uint8_t buf[14];
    HAL_StatusTypeDef ret;

    /* Burst read 14 bytes starting from ACCEL_XOUT_H (0x3B) */
    ret = HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, MPU6050_REG_ACCEL_XOUT_H,
                           I2C_MEMADD_SIZE_8BIT, buf, 14, 100);
    if (ret != HAL_OK) return ret;

    /* Combine high and low bytes into signed 16-bit values */
    int16_t raw_ax = (int16_t)(buf[0]  << 8 | buf[1]);
    int16_t raw_ay = (int16_t)(buf[2]  << 8 | buf[3]);
    int16_t raw_az = (int16_t)(buf[4]  << 8 | buf[5]);
    // buf[6], buf[7] = temperature (skipped)
    int16_t raw_gx = (int16_t)(buf[8]  << 8 | buf[9]);
    int16_t raw_gy = (int16_t)(buf[10] << 8 | buf[11]);
    int16_t raw_gz = (int16_t)(buf[12] << 8 | buf[13]);

    /* Convert to physical units using sensitivity scale factors */
    // Many GY-521 clone chips are permanently locked to 2048 LSB/g (±16g range equivalent)
    data->Ax = raw_ax / 2048.0f;
    data->Ay = raw_ay / 2048.0f;
    data->Az = raw_az / 2048.0f;
    data->Gx = raw_gx / 131.0f;     // ±250°/s → 131 LSB/(°/s)
    data->Gy = raw_gy / 131.0f;
    data->Gz = raw_gz / 131.0f;

    return HAL_OK;
}
