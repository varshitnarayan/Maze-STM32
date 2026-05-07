/**
  ******************************************************************************
  * @file    main.c
  * @author  Varshith Narayan (IITJ Engineering)
  * @brief   Ball Maze Game using MPU6050 and LCD (ILI9341)
  ******************************************************************************
  */

#include "main.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "stm32f4xx_hal_rng.h"
#include "mpu6050.h"

/* Function Prototypes */
static void SystemClock_Config(void);
static void I2C1_Init(void);
static void USART1_Init(void);
void BoardInit(void);
void DisplayUI(void);
void Toggle_Leds(void);
void DisplayWinningScreen(void);
void DisplayFailureScreen(void);
bool BallHitTheLosingWall(int array[], int x_position, int y_position, int radius);
bool BallHitTheWinningWall(int x_position, int y_position);
bool CalculateIfCollisionOccurred(int array[], int x, int y);

/* Global Variables */
static int failures = 0;            // Score tracking
static int victories = 0;           // Score tracking
static RNG_HandleTypeDef rng_inst;  // Required for random maze generation
I2C_HandleTypeDef hi2c1;            // I2C Handle for MPU6050
UART_HandleTypeDef huart1;          // UART Handle for Putty
char lcdStringBuffer[100];          // Buffer for printing scores to LCD

int main(void)
{ 
    // Initialize Hardware (LCD, I2C, RNG, Clocks)
    BoardInit();

    // --- ONE-TIME CALIBRATION AT STARTUP ---
    float Ax_offset = 0.0f;
    float Ay_offset = 0.0f;
    int calib_count = 0;
    MPU6050_Data mpu_data;

    // Initialize LCD Layer for text display
    BSP_LCD_LayerDefaultInit(1, LCD_FRAME_BUFFER);
    BSP_LCD_SelectLayer(1);
    BSP_LCD_SetFont(&Font12);
    BSP_LCD_Clear(LCD_COLOR_LIGHTBLUE);
    BSP_LCD_SetBackColor(LCD_COLOR_LIGHTBLUE);

    // Give the user 2 seconds to place the board perfectly flat
    BSP_LCD_SetTextColor(LCD_COLOR_RED);
    BSP_LCD_DisplayStringAt(0, 100, (uint8_t *)"PLACE FLAT", CENTER_MODE);
    BSP_LCD_DisplayStringAt(0, 130, (uint8_t *)"ON TABLE!", CENTER_MODE);
    HAL_Delay(2000);

    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_DisplayStringAt(0, 160, (uint8_t *)"Calibrating...", CENTER_MODE);

    while (calib_count < 100)
    {
        if (MPU6050_ReadAll(&hi2c1, &mpu_data) == HAL_OK)
        {
            Ax_offset += mpu_data.Ax;
            Ay_offset += mpu_data.Ay;
            calib_count++;
            HAL_Delay(5);
        }
    }
    Ax_offset /= 100.0f;
    Ay_offset /= 100.0f;
    // ----------------------------------------

    // The 'start' label allows the game to loop back after win/loss
    start:

    // Clear screen and show score area
    DisplayUI();

    // Generate and Display Maze
    // 15x17 grid fits the 240x320 portrait screen well
    static int maze[255];
    Maze_Generate(maze, 15, 17, &rng_inst);
    Maze_Display(maze, 15, 17, 16, LCD_COLOR_DARKBLUE);

    // Initial Ball Position
    int x_position = 25;
    int y_position = 23;
    int ball_radius = 3;
    BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
    BSP_LCD_FillCircle(x_position, y_position, ball_radius);

    // Movement Parameters
    float Xval, Yval;
    // Sensitivity threshold (Deadzone) to ignore sensor noise.
    // 0.10g ignores values between -10 and 10 in PuTTY.
    float sensitivity = 0.10f;
    int distance = 1;          // Pixel step size per move
    uint32_t last_print_tick = 0; // Timer to control print refresh rate

    while (1)
    {
        // 1. Read MPU6050 Data
        if (MPU6050_ReadAll(&hi2c1, &mpu_data) == HAL_OK)
        {
            // We use Accelerometer data to detect board tilt and subtract the flat offset
            Xval = mpu_data.Ax - Ax_offset;
            Yval = mpu_data.Ay - Ay_offset;

            // Update LCD and UART only every 500ms (to slow down refresh rate without affecting game speed)
            if (HAL_GetTick() - last_print_tick >= 500)
            {
                // Display MPU6050 values on the screen
                sprintf(lcdStringBuffer, "Ax:%d Ay:%d   ", (int)(Xval * 100), (int)(Yval * 100));
                BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
                BSP_LCD_SetBackColor(LCD_COLOR_LIGHTBLUE);
                BSP_LCD_DisplayStringAt(5, 5, (uint8_t *)lcdStringBuffer, LEFT_MODE);

                // Transmit Accel values over UART to Putty instead of Gyro
                char uartBuf[100];
                sprintf(uartBuf, "Normalized Tilt -> Ax: %d, Ay: %d\r\n",
                        (int)(Xval * 100),
                        (int)(Yval * 100));
                HAL_UART_Transmit(&huart1, (uint8_t*)uartBuf, strlen(uartBuf), HAL_MAX_DELAY);

                last_print_tick = HAL_GetTick(); // Reset the timer
            }

            // 2. Check if board is tilted beyond sensitivity threshold
            if (fabs(Xval) > sensitivity || fabs(Yval) > sensitivity)
            {
                // Erase old ball
                BSP_LCD_SetTextColor(LCD_COLOR_LIGHTBLUE);
                BSP_LCD_FillCircle(x_position, y_position, ball_radius);

                // Update positions based on tilt
                // Depending on the board orientation, you may need to swap X and Y or change signs
                if (Xval > sensitivity)  x_position += distance;
                if (Xval < -sensitivity) x_position -= distance;
                if (Yval > sensitivity)  y_position += distance;
                if (Yval < -sensitivity) y_position -= distance;

                // Draw new ball
                BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
                BSP_LCD_FillCircle(x_position, y_position, ball_radius);

                // 3. Collision Logic
                if (BallHitTheWinningWall(x_position, y_position))
                {
                    victories++;
                    DisplayWinningScreen();
                    goto start; // Restart game
                }

                if (BallHitTheLosingWall(maze, x_position, y_position, ball_radius))
                {
                    failures++;
                    DisplayFailureScreen();
                    goto start; // Restart game
                }
            }
        }

        // Small delay to control ball speed and prevent flickering
        HAL_Delay(15);
    }
}

/**
  * @brief  Initializes all necessary discovery board peripherals
  */
void BoardInit(void)
{
    HAL_Init();
    SystemClock_Config();

    // Enable Peripheral Clocks
    __HAL_RCC_RNG_CLK_ENABLE();   // For Random Maze
    // I2C GPIO Clocks should be enabled in HAL_I2C_MspInit

    // Initialize Peripherals via BSP
    BSP_LCD_Init();

    // Initialize I2C and MPU6050
    I2C1_Init();
    if (MPU6050_Init(&hi2c1) != HAL_OK) {
        // Initialization Error: You could light a red LED here
    }

    // Initialize UART for Putty
    USART1_Init();

    // Configure RNG
    rng_inst.Instance = RNG;
    if (HAL_RNG_Init(&rng_inst) != HAL_OK) {
        // Handle Error
    }
}

/**
  * @brief I2C1 Initialization Function
  */
static void I2C1_Init(void)
{
    // 1. Enable Clocks for I2C1 and GPIOB
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    // 2. Configure GPIO Pins for I2C1 (PB8 = SCL, PB9 = SDA)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // 3. Initialize I2C1
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        // Error Handler
    }
}

/**
  * @brief USART1 Initialization Function (For Putty)
  */
static void USART1_Init(void)
{
    // 1. Enable Clocks for USART1 and GPIOA
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    // 2. Configure GPIO Pins for USART1 (PA9 = TX, PA10 = RX)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 3. Initialize USART1
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        // Error Handler
    }
}

/**
  * @brief  Sets up the game UI and score area
  */
void DisplayUI(void)
{
    BSP_LCD_LayerDefaultInit(1, LCD_FRAME_BUFFER);
    BSP_LCD_SelectLayer(1);
    BSP_LCD_SetFont(&Font12);
    BSP_LCD_Clear(LCD_COLOR_LIGHTBLUE);
    BSP_LCD_SetBackColor(LCD_COLOR_LIGHTBLUE);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);

    // Display Scores at the bottom of the screen
    sprintf(lcdStringBuffer, "Failures: %d", failures);
    BSP_LCD_DisplayStringAt(5, 280, (uint8_t *)lcdStringBuffer, LEFT_MODE);

    sprintf(lcdStringBuffer, "Victories: %d", victories);
    BSP_LCD_DisplayStringAt(5, 300, (uint8_t *)lcdStringBuffer, LEFT_MODE);
}

/**
  * @brief  Checks for winning area (Exit of the maze)
  */
bool BallHitTheWinningWall(int x_position, int y_position)
{
    // Define your winning coordinates (Target Exit)
    if (x_position >= 208 && x_position <= 230 && y_position >= 256 && y_position <= 272)
    {
        return true;
    }
    return false;
}

/**
  * @brief  Collision detection against maze walls
  */
bool BallHitTheLosingWall(int array[], int x_position, int y_position, int radius)
{
    // Check points around the circle circumference for walls
    // Using a simplified 8-point check for performance
    if (CalculateIfCollisionOccurred(array, x_position + radius, y_position) ||
        CalculateIfCollisionOccurred(array, x_position - radius, y_position) ||
        CalculateIfCollisionOccurred(array, x_position, y_position + radius) ||
        CalculateIfCollisionOccurred(array, x_position, y_position - radius))
    {
        return true;
    }
    return false;
}

/**
  * @brief  Maps LCD coordinates to maze array index
  */
bool CalculateIfCollisionOccurred(int array[], int x, int y)
{
    // Scale factor 16 based on Maze_Display block size
    int row = x / 16;
    int col = y / 16;

    // Check if the current grid cell is a wall (value == 1)
    if (array[row + 15 * col] == 1) return true;

    return false;
}

void DisplayWinningScreen()
{
    BSP_LCD_Clear(LCD_COLOR_LIGHTGREEN);
    BSP_LCD_SetFont(&Font20);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_DisplayStringAt(0, 140, (uint8_t*)"YOU WIN!", CENTER_MODE);
    HAL_Delay(2000);
}

void DisplayFailureScreen()
{
    BSP_LCD_Clear(LCD_COLOR_LIGHTRED);
    BSP_LCD_SetFont(&Font20);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_DisplayStringAt(0, 140, (uint8_t*)"TRY AGAIN!", CENTER_MODE);
    HAL_Delay(2000);
}

/**
  * @brief  Standard System Clock Config for 180MHz
  */
static void SystemClock_Config(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 360;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    HAL_PWREx_EnableOverDrive();

    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}

/**
  * @brief  Toggle LEDs for SysTick heartbeat
  */
void Toggle_Leds(void)
{
  static uint8_t ticks = 0;

  if(ticks++ > 100)
  {
    // BSP_LED_Toggle(LED3); // Removed if using custom board
    // BSP_LED_Toggle(LED4);
    ticks = 0;
  }
}
