/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>  // For va_list, vsnprintf

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t dma_rx_buffer[MODEM_BUF_SIZE]; // DMA raw data pool // UART1 modem RX
char modem_response[MODEM_BUF_SIZE];  // Clean string for processing // UART1 responses
volatile uint8_t modem_data_ready = 0;
uint8_t debug_tx_buf[DEBUG_BUF_SIZE];	// UART2 debug TX

typedef enum {
    MODEM_AT_CHECK,
    MODEM_ECHO_OFF,
	MODEM_NETWORK_CHECK,
    MODEM_SIM_STATUS,
    MODEM_SEND_SMS_CMD,
    MODEM_SMS_BODY,
	MODEM_CALLING,
    MODEM_IDLE
} ModemState_t;

ModemState_t currentState = MODEM_AT_CHECK;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void send_uart (char *string)
{
	uint8_t len = strlen (string);
	HAL_UART_Transmit(&huart2, (uint8_t *) string, len, HAL_MAX_DELAY); // Transmitting in Blocking Mode
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_5;  // PA5 = PWRKEY
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // POWER SEQUENCE - CORRECT TIMING
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);   // HIGH = OFF
  HAL_Delay(500);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET); // LOW pulse ON
  HAL_Delay(1200);  // 1.2s pulse
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);   // HIGH = running

  send_uart("PWRKEY pulsed - Modem booting...\r\n");
  HAL_Delay(5000);  // Wait RDY URCs

  // === DMA + STATE MACHINE (NEW) ===
  send_uart("Starting DMA SMS Demo...\r\n");

  // Enable UART1 IDLE + Start DMA
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
  HAL_UART_Receive_DMA(&huart1, dma_rx_buffer, MODEM_BUF_SIZE);

  send_uart("Sending first AT Command...\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t*)"AT\r\n", 4, 100);

//  /* NOW RETURN TO YOUR ORIGINAL MAIN LOOP */
//  goto original_main_loop;  // Skip polling while(1)
//
//  uint8_t test_rx[64];
//  uint32_t last_at = 0;
//  while(1) {
//      // Echo modem RX → PuTTY
//      if (HAL_UART_Receive(&huart1, test_rx, 1, 10) == HAL_OK) {
//          HAL_UART_Transmit(&huart2, test_rx, 1, 10);
//      }
//
//      // Auto AT every 3s
//      if (HAL_GetTick() - last_at > 3000) {
//          send_uart("TX> AT\r\n");
//          HAL_UART_Transmit(&huart1, (uint8_t*)"AT\r\n", 4, 100);
//          last_at = HAL_GetTick();
//      }
//  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  //original_main_loop:
  while (1)
  {
	  if (modem_data_ready) {
	          modem_data_ready = 0;

	          // Log response to PuTTY (USART2)
	          send_uart("[MODEM]: ");
	          send_uart(modem_response);
	          send_uart("\r\n");

	          // State Machine Logic
	          if (strstr(modem_response, "OK") || strstr(modem_response, ">")) {
	              switch (currentState) {
	                  case MODEM_AT_CHECK:
	                      currentState = MODEM_ECHO_OFF;
	                      HAL_UART_Transmit(&huart1, (uint8_t*)"ATE0\r\n", 6, 100);
	                      break;

	                  case MODEM_ECHO_OFF:
	                      currentState = MODEM_SIM_STATUS;  // Skip SIM_DETECT - go direct to CPIN
	                      HAL_UART_Transmit(&huart1, (uint8_t*)"AT+CPIN?\r\n", 10, 100);
	                      break;

//	                  case MODEM_SIM_DETECT:
//	                      send_uart("Checking SIM detect...\r\n");
//	                      HAL_UART_Transmit(&huart1, (uint8_t*)"AT+QSIMDET\r\n", 12, 100);  // SIM hotplug detect
//	                      break;

	                  case MODEM_SIM_STATUS:
	                      if (strstr(modem_response, "+CPIN: READY") || strstr(modem_response, "READY")) {
	                          send_uart("SIM OK! Checking network...\r\n");
	                          currentState = MODEM_NETWORK_CHECK;
	                          HAL_UART_Transmit(&huart1, (uint8_t*)"AT+CREG?\r\n", 10, 100);  // Circuit switched
	                          HAL_UART_Transmit(&huart1, (uint8_t*)"AT+CEREG?\r\n", 10, 100); // Packet switched
	                      }
	                      break;

	                  case MODEM_NETWORK_CHECK:
	                      if (strstr(modem_response, "+CREG: 0,1") || strstr(modem_response, "+CREG: 0,5") ||
	                          strstr(modem_response, "+CEREG: 0,1") || strstr(modem_response, "+CEREG: 0,5")) {
	                          send_uart("NETWORK REGISTERED! SMS ready...\r\n");
	                          currentState = MODEM_SEND_SMS_CMD;
	                          HAL_UART_Transmit(&huart1, (uint8_t*)"AT+CMGF=1\r\n", 11, 100);
	                      } else {
	                          send_uart("Waiting network registration...\r\n");
	                          HAL_Delay(5000);
	                          HAL_UART_Transmit(&huart1, (uint8_t*)"AT+CREG?\r\n", 10, 100);
	                      }
	                      break;

	                  case MODEM_SEND_SMS_CMD:  // ← ADD THIS MISSING CASE
	                      send_uart("Setting SMS text mode...\r\n");
	                      currentState = MODEM_SMS_BODY;
	                      HAL_UART_Transmit(&huart1, (uint8_t*)"AT+CMGS=\"+91...\"\r\n", 25, 100);
	                      break;

	                  case MODEM_SMS_BODY:
	                          HAL_Delay(100);
	                          HAL_UART_Transmit(&huart1, (uint8_t*)"Hello from STM32 DMA Mode!", 26, 100);
	                          uint8_t ctrlZ = 0x1A;
	                          HAL_UART_Transmit(&huart1, &ctrlZ, 1, 100);
	                          send_uart("SMS sent, starting VOICE CALL...\r\n");
	                          currentState = MODEM_CALLING;  // → CALL NEXT
	                          break;

	                  case MODEM_CALLING:  // ← NEW VOICE CALL
	                          send_uart("DIALING +91...\r\n");
	                          HAL_UART_Transmit(&huart1, (uint8_t*)"ATD+91...;\r\n", 20, 100);  // Voice call
	                          HAL_Delay(5000);  // Ring time
	                          HAL_UART_Transmit(&huart1, (uint8_t*)"ATH\r\n", 5, 100);  // Hang up
	                          send_uart("Call ended - System ready\r\n");
	                          currentState = MODEM_IDLE;
	                          break;

	                  case MODEM_IDLE:
	                      HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET); // Success LED
	                      break;
	              }
	          }
	  }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
