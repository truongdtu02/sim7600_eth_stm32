/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "lwip.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sim7600_uart_dma.h"
#include "tcp_udp_stack.h"
#include "rsa.h"
#include "AES_128.h"
#include "md5.h"
#include "ethLAN.h"
#include "vs1063a.h"
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
SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim8;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for InitQueue */
osMessageQueueId_t InitQueueHandle;
const osMessageQueueAttr_t InitQueue_attributes = {
  .name = "InitQueue"
};
/* Definitions for InitTimerPeriodic */
osTimerId_t InitTimerPeriodicHandle;
const osTimerAttr_t InitTimerPeriodic_attributes = {
  .name = "InitTimerPeriodic"
};
/* Definitions for InitTimerOnce */
osTimerId_t InitTimerOnceHandle;
const osTimerAttr_t InitTimerOnce_attributes = {
  .name = "InitTimerOnce"
};
/* Definitions for InitBinSem */
osSemaphoreId_t InitBinSemHandle;
const osSemaphoreAttr_t InitBinSem_attributes = {
  .name = "InitBinSem"
};
/* Definitions for InitCountSem */
osSemaphoreId_t InitCountSemHandle;
const osSemaphoreAttr_t InitCountSem_attributes = {
  .name = "InitCountSem"
};
/* Definitions for InitEvent */
osEventFlagsId_t InitEventHandle;
const osEventFlagsAttr_t InitEvent_attributes = {
  .name = "InitEvent"
};
/* USER CODE BEGIN PV */

///// Task  ////
osThreadId_t configSimTaskHandle;
const osThreadAttr_t configSimTask_attributes = {
  .name = "configSimTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh3,
};

osThreadId_t connectSimTaskHandle;
const osThreadAttr_t connectSimTask_attributes = {
  .name = "connectSimTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh2,
};

osThreadId_t sendSimTaskHandle;
const osThreadAttr_t sendSimTask_attributes = {
  .name = "sendSimTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh1,
};

osThreadId_t recvSimTaskHandle;
const osThreadAttr_t recvSimTask_attributes = {
  .name = "recvSimTask",
  .stack_size = 1024 * 4 ,
  .priority = (osPriority_t) osPriorityHigh,
};

osThreadId_t connectEthTaskId;
const osThreadAttr_t connectEthTask_attributes = {
  .name = "connectEthTask",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};

osThreadId_t sendEthTaskId;
const osThreadAttr_t sendEthTask_attributes = {
  .name = "sendEthTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};

osThreadId_t recvEthTaskId;
const osThreadAttr_t recvEthTask_attributes = {
  .name = "recvEthTask",
  .stack_size = 1024 * 4 ,
  .priority = (osPriority_t) osPriorityHigh,
};

osThreadId_t vs1063TaskId;
const osThreadAttr_t vs1063Task_attributes = {
  .name = "vs1063Task",
  .stack_size = 256 * 4 ,
  .priority = (osPriority_t) osPriorityHigh, //==recvETH recvSim task
};
////////////////////////////////////////


/// Event flags  ///

/* bit order  4 3 2 1 0 , priority MSB->LSB
              firstConfig NetError  Calling sendErrorOrIPclose Message
*/ 
// enum configSimFlagEnum {msg, calling, netError, firstConfig}; //in sim7600 .h
osEventFlagsId_t ConfigSimEventID;
const osEventFlagsAttr_t ConfigSimEvent_attributes = {
  .name = "ConfigSimEvent"
};

//recvACK recvRSA reconnect
//2 1 0
// enum connectSimFlagEnum {reConEnum, recvRSAEnum, recvACKEnum}; //in sim7600 .h

osEventFlagsId_t ConnectSimEventID;
const osEventFlagsAttr_t ConnectSimEvent_attributes = {
  .name = "ConnectSimEvent"
};

/* bit order  0
              sendSuccess
*/ 
osEventFlagsId_t SendSimEventID;
const osEventFlagsAttr_t SendSimEvent_attributes = {
  .name = "SendSimEvent"
};

osEventFlagsId_t ConnectEthEventID;
const osEventFlagsAttr_t ConnectEthEvent_attributes = {
  .name = "ConnectEthEvent"
};

////////////////////////


/// Semaphore ///
osSemaphoreId_t SimATBinSem;
const osSemaphoreAttr_t SimATBinSem_attributes = {
  .name = "SimATBinSem"
};
////////////////////////


//// Message queue //
osMessageQueueId_t SendSimQueueID;
const osMessageQueueAttr_t SendSimQueue_attributes = {
  .name = "SendSimQueue"
};

osMessageQueueId_t SendEthQueueID;
const osMessageQueueAttr_t SendEthQueue_attributes = {
  .name = "SendEthQueue"
};
//////////////////////////

///// timer ///////////
osTimerId_t TCPTimerOnceID; //timer for time-out handshake and KeepAlive

//debug
osTimerId_t SttCheckTimerID; //timer for check sim status
#define STT_CHECK_TIMER_INTERVAL 10 //ms
int oldSimStt = -1;
void SimStt_Timer_Callback(void *argument)
{
  if(Sim_STT != oldSimStt)
  {
    oldSimStt = Sim_STT;
    LOG_WRITE("sim status %d\n", oldSimStt);
  }
}

///////////////////////

////////////////////////


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM8_Init(void);
static void MX_SPI2_Init(void);
void StartDefaultTask(void *argument);
void CallbackTimerPeriodic(void *argument);
void CallbackTimerOnce(void *argument);

/* USER CODE BEGIN PFP */
char* DeviceID = "123456781234567812345678";

// #if DEBUG_LOG
int _write(int32_t file, uint8_t *ptr, int32_t len)
{
    for (int i = 0; i < len; i++)
    {
        ITM_SendChar(*ptr++);
    }
    return len;
}
// #endif

volatile unsigned long ulHighFrequencyTimerTicks;

void configureTimerForRunTimeStats(void) {
  ulHighFrequencyTimerTicks = 0;
  HAL_TIM_Base_Start_IT(&htim8);
}

unsigned long getRunTimeCounterValue(void) {
  return ulHighFrequencyTimerTicks;
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
extern struct netif gnetif;

  //sim
void StartConfigSimTask(void *argument);
void StartConnectSimTask(void *argument);
void StartSendSimTask(void *argument);
void StartRecvSimTask(void *argument);

  //eth
void StartConnectEthTask(void *argument);
void StartSendEthTask(void *argument);
void StartRecvEthTask(void *argument);

void StartVs1063Task(void *argument);
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
  HAL_Delay(30000); //wait for debug
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_TIM8_Init();
  MX_SPI2_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start(&htim2);
  configureTimerForRunTimeStats();

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of InitBinSem */
  InitBinSemHandle = osSemaphoreNew(1, 1, &InitBinSem_attributes);

  /* creation of InitCountSem */
  InitCountSemHandle = osSemaphoreNew(2, 2, &InitCountSem_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  SimATBinSem = osSemaphoreNew(1, 1, &SimATBinSem_attributes);
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* creation of InitTimerPeriodic */
  InitTimerPeriodicHandle = osTimerNew(CallbackTimerPeriodic, osTimerPeriodic, NULL, &InitTimerPeriodic_attributes);

  /* creation of InitTimerOnce */
  InitTimerOnceHandle = osTimerNew(CallbackTimerOnce, osTimerOnce, NULL, &InitTimerOnce_attributes);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  TCPTimerOnceID = osTimerNew(TCP_Timer_Callback, osTimerOnce, NULL, NULL);
  SttCheckTimerID = osTimerNew(SimStt_Timer_Callback, osTimerPeriodic, NULL, NULL);
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of InitQueue */
  InitQueueHandle = osMessageQueueNew (16, sizeof(uint16_t), &InitQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  
  /* USER CODE END RTOS_THREADS */

  /* Create the event(s) */
  /* creation of InitEvent */
  InitEventHandle = osEventFlagsNew(&InitEvent_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  
  // SendEthEventID = osEventFlagsNew(&SendEthEvent_attributes);


  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
//  VS1063_Init();
//  VS1063_SoftReset();
  while (1)
  {
//	  VS1063_PlayBeep();
//	  HAL_Delay(500);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_5);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_5)
  {
  }
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
  LL_RCC_HSE_Enable();

   /* Wait till HSE is ready */
  while(LL_RCC_HSE_IsReady() != 1)
  {

  }
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_4, 168, LL_RCC_PLLP_DIV_2);
  LL_RCC_PLL_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL_IsReady() != 1)
  {

  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_4);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {

  }
  LL_SetSystemCoreClock(168000000);

   /* Update the time base */
  if (HAL_InitTick (TICK_INT_PRIORITY) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi2.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 41999;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM8 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM8_Init(void)
{

  /* USER CODE BEGIN TIM8_Init 0 */

  /* USER CODE END TIM8_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM8_Init 1 */

  /* USER CODE END TIM8_Init 1 */
  htim8.Instance = TIM8;
  htim8.Init.Prescaler = 0;
  htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim8.Init.Period = 1679;
  htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim8.Init.RepetitionCounter = 0;
  htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim8) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim8, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim8, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM8_Init 2 */

  /* USER CODE END TIM8_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  LL_USART_InitTypeDef USART_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);

  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
  /**USART1 GPIO Configuration
  PA9   ------> USART1_TX
  PA10   ------> USART1_RX
  PA11   ------> USART1_CTS
  PA12   ------> USART1_RTS
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_9|LL_GPIO_PIN_10|LL_GPIO_PIN_11|LL_GPIO_PIN_12;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USART1 DMA Init */

  /* USART1_RX Init */
  LL_DMA_SetChannelSelection(DMA2, LL_DMA_STREAM_2, LL_DMA_CHANNEL_4);

  LL_DMA_SetDataTransferDirection(DMA2, LL_DMA_STREAM_2, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);

  LL_DMA_SetStreamPriorityLevel(DMA2, LL_DMA_STREAM_2, LL_DMA_PRIORITY_VERYHIGH);

  LL_DMA_SetMode(DMA2, LL_DMA_STREAM_2, LL_DMA_MODE_CIRCULAR);

  LL_DMA_SetPeriphIncMode(DMA2, LL_DMA_STREAM_2, LL_DMA_PERIPH_NOINCREMENT);

  LL_DMA_SetMemoryIncMode(DMA2, LL_DMA_STREAM_2, LL_DMA_MEMORY_INCREMENT);

  LL_DMA_SetPeriphSize(DMA2, LL_DMA_STREAM_2, LL_DMA_PDATAALIGN_BYTE);

  LL_DMA_SetMemorySize(DMA2, LL_DMA_STREAM_2, LL_DMA_MDATAALIGN_BYTE);

  LL_DMA_DisableFifoMode(DMA2, LL_DMA_STREAM_2);

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  USART_InitStruct.BaudRate = 115200;
  USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
  USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
  USART_InitStruct.Parity = LL_USART_PARITY_EVEN;
  USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_RTS_CTS;
  USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
  LL_USART_Init(USART1, &USART_InitStruct);
  LL_USART_ConfigAsyncMode(USART1);
  LL_USART_Enable(USART1);
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* Init with LL driver */
  /* DMA controller clock enable */
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2);

  /* DMA interrupt init */
  /* DMA2_Stream2_IRQn interrupt configuration */
  NVIC_SetPriority(DMA2_Stream2_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),5, 0));
  NVIC_EnableIRQ(DMA2_Stream2_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOE);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOH);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);

  /**/
  LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_6);

  /**/
  LL_GPIO_SetOutputPin(GPIOE, LL_GPIO_PIN_7|LL_GPIO_PIN_11|LL_GPIO_PIN_13);

  /**/
  GPIO_InitStruct.Pin = LL_GPIO_PIN_2|LL_GPIO_PIN_9;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LL_GPIO_PIN_6;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LL_GPIO_PIN_7|LL_GPIO_PIN_11|LL_GPIO_PIN_13;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOE, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

void StartConfigSimTask(void *argument)
{
	(void*) argument;
  //debug
  osTimerStart(SttCheckTimerID, STT_CHECK_TIMER_INTERVAL);

  sim7600_fullConfigTask();
//	AES_Generate_Rand_Key();
//  for(;;) {
//    RSA2048_Pubkey_Encrypt(NULL, 0, input, strlen(input), output);
//    AES_Encrypt_Packet(output, 250);
//    AES_Decrypt_Packet(output, 256);
//    osDelay(1000);
//  }
}

void StartConnectSimTask(void *argument)
{
	(void*) argument;
  sim7600_connectTask();
//	for(;;) {
//		osDelay(100);
//	}
}

void StartSendSimTask(void *argument)
{
  sim7600_sendTask();
//	for(;;) {
//			osDelay(100);
//		}
}
void StartRecvSimTask(void *argument)
{
  sim7600_recvTask();
//	for(;;) {
//			osDelay(100);
//		}
}

void StartConnectEthTask(void *argument)
{
  (void*) argument;
  ethConnectTask();
}

void StartSendEthTask(void *argument)
{
  (void*) argument;
  ethSendTask();
}
void StartRecvEthTask(void *argument)
{
  (void*) argument;
  ethRecvTask();
}

void StartVs1063Task(void *argument)
{
	(void*) argument;
	VS1063_PlayMP3_Task();
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* init code for LWIP */
  MX_LWIP_Init();
  /* USER CODE BEGIN 5 */
//	VS1063_Init();
//	while(true)
//	{
//		VS1063_PlayBeep();
//		osDelay(500);
//	}
  if (netif_is_link_up(&gnetif) == 0)
  {
    //start with sim7600
    LOG_WRITE("sim7600 mode\n");

    ConfigSimEventID = osEventFlagsNew(&ConfigSimEvent_attributes);
    ConnectSimEventID = osEventFlagsNew(&ConnectSimEvent_attributes);
    SendSimEventID = osEventFlagsNew(&SendSimEvent_attributes);

    SendSimQueueID = osMessageQueueNew(5, sizeof(sendSimPack), &SendSimQueue_attributes);

    configSimTaskHandle = osThreadNew(StartConfigSimTask, NULL, &configSimTask_attributes);
    connectSimTaskHandle = osThreadNew(StartConnectSimTask, NULL, &connectSimTask_attributes);
    sendSimTaskHandle = osThreadNew(StartSendSimTask, NULL, &sendSimTask_attributes);
    recvSimTaskHandle = osThreadNew(StartRecvSimTask, NULL, &recvSimTask_attributes);
    vs1063TaskId = osThreadNew(StartVs1063Task, NULL, &vs1063Task_attributes);
  }
  else
  {
    //start with ETH
    LOG_WRITE("eth mode\n");

//    while(MX_LWIP_checkIsystem_ip_addr() == 0)
//        {
//        	osDelay(1000);
//        }

    ConnectEthEventID = osEventFlagsNew(&ConnectEthEvent_attributes);

    SendEthQueueID = osMessageQueueNew(5, sizeof(sendEthPack), &SendEthQueue_attributes);

    connectEthTaskId = osThreadNew(StartConnectEthTask, NULL, &connectEthTask_attributes);
    sendEthTaskId = osThreadNew(StartSendEthTask, NULL, &sendEthTask_attributes);
    // recvEthTaskId = osThreadNew(StartRecvEthTask, NULL, &recvEthTask_attributes);
    vs1063TaskId = osThreadNew(StartVs1063Task, NULL, &vs1063Task_attributes);
  }

  /* Infinite loop */
  for (;;)
  {
    osDelay(1000);
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
  }
  /* USER CODE END 5 */
}

/* CallbackTimerPeriodic function */
void CallbackTimerPeriodic(void *argument)
{
  /* USER CODE BEGIN CallbackTimerPeriodic */

  /* USER CODE END CallbackTimerPeriodic */
}

/* CallbackTimerOnce function */
void CallbackTimerOnce(void *argument)
{
  /* USER CODE BEGIN CallbackTimerOnce */

  /* USER CODE END CallbackTimerOnce */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
	  //blink led in special period, like on 0.5s, off 0.5s, on 0.5s, off 0.5s, on 0.5s, off 2s

  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
