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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "canhacker.h"
#include "sn65hvd230dr.h"
/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define UART_RX_LINE_BUFFER 80
#define UART_RX_BUFFER 256
#define UART_RX_BUFFERS_COUNT 2
/* USER CODE END PD */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    uint8_t buffer[UART_RX_BUFFER];
    uint8_t readed;
    uint8_t filled;
    int offset;
} UART_DMA_RX_Buffer;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define UART_RX_LINE_BUFFER 80
#define UART_RX_BUFFER 256
#define UART_RX_BUFFERS_COUNT 2
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan1;

TIM_HandleTypeDef htim6;

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_tx;
DMA_HandleTypeDef hdma_usart1_rx;

osThreadId defaultTaskHandle;
osThreadId uartRxDmaTaskHandle;
osMessageQId uartTxQueueHandle;
osMessageQId canRxQueueHandle;
osMessageQId canTxQueueHandle;
/* USER CODE BEGIN PV */

int uartLineIndex = 0;
uint8_t uartLine[UART_RX_LINE_BUFFER];
static CanTxMsgTypeDef CanTxMessage;
uint32_t Mailbox;
CanHacker_HandleTypeDef hcanHacker;
UART_DMA_RX_Buffer uartRxBuffer[UART_RX_BUFFERS_COUNT] = {
        {.readed = 1, .filled = 0, .offset = 0},
        {.readed = 1, .filled = 0, .offset = 0}
};
uint8_t currentUartRxBuffer = 0;
uint8_t currentUartRxBufferToRead = 0;
CAN_FilterTypeDef canFilterConfig;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

static void MX_GPIO_Init(void);

static void MX_DMA_Init(void);

static void MX_CAN_Init(void);

static void MX_USART1_UART_Init(void);

static void MX_TIM6_Init(void);

void StartDefaultTask(void const *argument);

void StartTaskUartRxDmaTask(void const *argument);

/* USER CODE BEGIN PFP */
void throwError(char *msg);

void txUart();

void txCan();

void transmitErrorMessage(char *message);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void) {
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
    MX_CAN_Init();
    MX_USART1_UART_Init();
    MX_TIM6_Init();
    /* USER CODE BEGIN 2 */
    sn65Hvd230DrStandbyMode();
    CanHacker_Init(&hcanHacker);

    canFilterConfig.FilterBank = 0;
    canFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    canFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    canFilterConfig.FilterIdHigh = 0x0000;
    canFilterConfig.FilterIdLow = 0x0000;
    canFilterConfig.FilterMaskIdHigh = 0x0000;
    canFilterConfig.FilterMaskIdLow = 0x0000;
    canFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    canFilterConfig.FilterActivation = CAN_FILTER_ENABLE;
    canFilterConfig.SlaveStartFilterBank = 1;

    HAL_CAN_ConfigFilter(&hcan1, &canFilterConfig);
    HAL_CAN_Start(&hcan1);
    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
    /* USER CODE END 2 */

    /* USER CODE BEGIN RTOS_MUTEX */
    /* add mutexes, ... */
    /* USER CODE END RTOS_MUTEX */

    /* USER CODE BEGIN RTOS_SEMAPHORES */
    /* add semaphores, ... */
    /* USER CODE END RTOS_SEMAPHORES */

    /* USER CODE BEGIN RTOS_TIMERS */
    /* start timers, add new ones, ... */
    /* USER CODE END RTOS_TIMERS */

    /* Create the queue(s) */
    /* definition and creation of uartTxQueue */
    osMessageQDef(uartTxQueue, 16, int);
    uartTxQueueHandle = osMessageCreate(osMessageQ(uartTxQueue), NULL);

    /* definition and creation of canRxQueue */
    osMessageQDef(canRxQueue, 16, int);
    canRxQueueHandle = osMessageCreate(osMessageQ(canRxQueue), NULL);

    /* definition and creation of canTxQueue */
    osMessageQDef(canTxQueue, 16, int);
    canTxQueueHandle = osMessageCreate(osMessageQ(canTxQueue), NULL);

    /* USER CODE BEGIN RTOS_QUEUES */
    /* add queues, ... */
    /* USER CODE END RTOS_QUEUES */

    /* Create the thread(s) */
    /* definition and creation of defaultTask */
    osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
    defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

    /* definition and creation of uartRxDmaTask */
    osThreadDef(uartRxDmaTask, StartTaskUartRxDmaTask, osPriorityIdle, 0, 128);
    uartRxDmaTaskHandle = osThreadCreate(osThread(uartRxDmaTask), NULL);

    /* USER CODE BEGIN RTOS_THREADS */
    /* add threads, ... */
    /* USER CODE END RTOS_THREADS */

    /* Start scheduler */
    osKernelStart();

    /* We should never get here as control is now taken by the scheduler */
    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /** Initializes the RCC Oscillators according to the specified parameters
    * in the RCC_OscInitTypeDef structure.
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV2;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }
    /** Initializes the CPU, AHB and APB buses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
    PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief CAN Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN_Init(void) {

    /* USER CODE BEGIN CAN_Init 0 */
    HAL_StatusTypeDef canInitResult = HAL_OK;
    /* USER CODE END CAN_Init 0 */

    /* USER CODE BEGIN CAN_Init 1 */

    /* USER CODE END CAN_Init 1 */
    hcan1.Instance = CAN;
    hcan1.Init.Prescaler = 6;
    hcan1.Init.Mode = CAN_MODE_NORMAL;
    hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan1.Init.TimeSeg1 = CAN_BS1_6TQ;
    hcan1.Init.TimeSeg2 = CAN_BS2_5TQ;
    hcan1.Init.TimeTriggeredMode = DISABLE;
    hcan1.Init.AutoBusOff = DISABLE;
    hcan1.Init.AutoWakeUp = DISABLE;
    hcan1.Init.AutoRetransmission = DISABLE;
    hcan1.Init.ReceiveFifoLocked = DISABLE;
    hcan1.Init.TransmitFifoPriority = DISABLE;
    if (HAL_CAN_Init(&hcan1) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN CAN_Init 2 */
    canInitResult = HAL_CAN_Init(&hcan1);
    switch (canInitResult) {
        case HAL_ERROR:
            throwError("MX_CAN_INIT: error");
            break;
        case HAL_BUSY:
            throwError("MX_CAN_INIT: busy");
            break;
        case HAL_TIMEOUT:
            throwError("MX_CAL_INIT: timeout");
            break;
        default:
            break;
    }
    /* USER CODE END CAN_Init 2 */

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void) {

    /* USER CODE BEGIN TIM6_Init 0 */

    /* USER CODE END TIM6_Init 0 */

    TIM_MasterConfigTypeDef sMasterConfig = {0};

    /* USER CODE BEGIN TIM6_Init 1 */

    /* USER CODE END TIM6_Init 1 */
    htim6.Instance = TIM6;
    htim6.Init.Prescaler = 35999;
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim6.Init.Period = 60000;
    htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim6) != HAL_OK) {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN TIM6_Init 2 */

    /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void) {

    /* USER CODE BEGIN USART1_Init 0 */

    /* USER CODE END USART1_Init 0 */

    /* USER CODE BEGIN USART1_Init 1 */

    /* USER CODE END USART1_Init 1 */
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN USART1_Init 2 */

    /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void) {

    /* DMA controller clock enable */
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* DMA interrupt init */
    /* DMA1_Channel4_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
    /* DMA1_Channel5_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(STBY_GPIO_Port, STBY_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pins : PC13 PC14 PC15 */
    GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /*Configure GPIO pins : PA0 PA1 PA2 PA3
                             PA4 PA5 PA6 PA7
                             PA8 PA15 */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3
                          | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7
                          | GPIO_PIN_8 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /*Configure GPIO pins : PB0 PB1 PB2 PB10
                             PB11 PB12 PB13 PB14
                             PB15 PB5 PB6 PB7 */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_10
                          | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14
                          | GPIO_PIN_15 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*Configure GPIO pin : STBY_Pin */
    GPIO_InitStruct.Pin = STBY_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(STBY_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
void throwError(char *msg) {
    transmitErrorMessage(msg);
    while (1) {
        HAL_Delay(1000);
    }
}

void txUart() {
    HAL_UART_StateTypeDef result;
    HAL_UART_StateTypeDef state = HAL_UART_GetState(&huart1);

    if (state != HAL_UART_STATE_BUSY_TX_RX && state != HAL_UART_STATE_BUSY_TX) {
        osEvent event = osMessageGet(uartTxQueueHandle, osWaitForever);
        if (event.status == osEventMessage) {
            char *str = event.value.p;
            unsigned int len = strlen(str);
            if (len > 0) {
                result = HAL_UART_Transmit_DMA(&huart1, (uint8_t *) str, len);
                switch (result) {
                    case HAL_ERROR:
                        throwError("HAL_UART_Transmit_IT: error");
                        break;
                    case HAL_BUSY:
                        throwError("HAL_UART_Transmit_IT: busy");
                        break;
                    case HAL_TIMEOUT:
                        throwError("HAL_UART_Transmit_IT: timeout");
                        break;
                    default:
                        break;
                }
            } else {
                free(str);
            }
        }
    } else {}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->pTxBuffPtr) {
        free(huart->pTxBuffPtr);
    }
    txUart();
}

void startUartDmaReceive(UART_HandleTypeDef *huart, UART_DMA_RX_Buffer *uartDmaRxBuffer) {
    uartDmaRxBuffer->readed = 0;
    HAL_StatusTypeDef result = HAL_UART_Receive_DMA(huart, uartDmaRxBuffer->buffer, UART_RX_BUFFER);
    switch (result) {
        case HAL_ERROR:
            throwError("HAL_UART_Receive_DMA: error");
            break;
        case HAL_BUSY:
            throwError("HAL_UART_Receive_DMA: busy");
            break;
        case HAL_TIMEOUT:
            throwError("HAL_UART_Receive_DMA: timeout");
            break;
        default:
            break;
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        uartRxBuffer[currentUartRxBuffer].filled = 1;
        currentUartRxBuffer++;
        currentUartRxBuffer %= UART_RX_BUFFERS_COUNT;
        if (!uartRxBuffer[currentUartRxBuffer].readed) {
            throwError("UART RX OVERRUN");
        }
        startUartDmaReceive(huart, &uartRxBuffer[currentUartRxBuffer]);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    uint32_t code = huart->ErrorCode;
    if (code & HAL_UART_ERROR_PE) {
        throwError("UART error: Parity error");
    }
    if (code & HAL_UART_ERROR_NE) {
        throwError("UART error: Noise error");
    }
    if (code & HAL_UART_ERROR_FE) {
        throwError("UART error: Frame error");
    }
    if (code & HAL_UART_ERROR_ORE) {
        throwError("UART error: Overrun error");
    }
    if (code & HAL_UART_ERROR_DMA) {
        throwError("UART error: DMA error");
    }
    char str[80];
    sprintf(str, "Uart error code: %X", (unsigned int) code);
    throwError(str);
}

void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan) {
    printf("HAL_CAN_TxMailbox0CompleteCallback\n");
    txCan();
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    HAL_StatusTypeDef result = HAL_OK;
    CanRxMsgTypeDef *rxMsg = malloc(sizeof(CanRxMsgTypeDef));
    result = HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxMsg->header, rxMsg->data);
    switch (result) {
        case HAL_ERROR:
            throwError("HAL_CAN_RxFifo0MsgPendingCallback: error");
            break;
        case HAL_BUSY:
            throwError("HAL_CAN_RxFifo0MsgPendingCallback: busy");
            break;
        case HAL_TIMEOUT:
            throwError("HAL_CAN_RxFifo0MsgPendingCallback: timeout");
            break;
        default:
            break;
    }
    osMessagePut(canRxQueueHandle, (uint32_t) rxMsg, osWaitForever);
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan) {
    throwError("HAL_CAN_ErrorCallback");
}

void txCan() {
    if (hcan1.State == HAL_CAN_STATE_READY) {
        osEvent event = osMessageGet(canTxQueueHandle, osWaitForever);
        if (event.status == osEventMessage) {
            CanTxMessage = *(CanTxMsgTypeDef *) event.value.p;
            HAL_StatusTypeDef result = HAL_CAN_AddTxMessage(&hcan1, &CanTxMessage.header, CanTxMessage.data, &Mailbox);
            free(event.value.p);
            switch (result) {
                case HAL_ERROR:
                    throwError("HAL_CAN_AddTxMessage: error");
                    break;
                case HAL_BUSY:
                    throwError("HAL_CAN_AddTxMessage: busy");
                    break;
                case HAL_TIMEOUT:
                    throwError("HAL_CAN_AddTxMessage: timeout");
                    break;
                default:
                    break;
            }
            switch (hcan1.ErrorCode) {
                case HAL_CAN_ERROR_EWG:
                case HAL_CAN_ERROR_EPV:
                case HAL_CAN_ERROR_BOF:
                case HAL_CAN_ERROR_STF:
                case HAL_CAN_ERROR_FOR:
                case HAL_CAN_ERROR_ACK:
                case HAL_CAN_ERROR_BR:
                case HAL_CAN_ERROR_BD:
                case HAL_CAN_ERROR_CRC:
                    throwError("TXCAN advanced error");
                default:
                    break;
            }
        }
    }
}

void transmitErrorMessage(char *message) {
    unsigned int len = strlen((char *) message);
    char *str = malloc(len + 3);
    memcpy(str, message, len);
    str[len] = '\r';
    str[len + 1] = '\n';
    str[len + 2] = '\0';
    osMessagePut(uartTxQueueHandle, (uint32_t) str, osWaitForever);
    txUart();
}

void gotoNextBufferToRead() {
    if (currentUartRxBufferToRead != currentUartRxBuffer) {
        currentUartRxBufferToRead++;
        currentUartRxBufferToRead %= UART_RX_BUFFERS_COUNT;
    }
}

int processUartDMABuffer(UART_HandleTypeDef *huart, UART_DMA_RX_Buffer *uartDmaRxBuffer) {
    int itemsProcessed = 0;
    if (uartDmaRxBuffer->readed) {
        return itemsProcessed;
    }
    int64_t dmaOffset;
    if (uartDmaRxBuffer->filled) {
        dmaOffset = UART_RX_BUFFER;
    } else {
        dmaOffset = huart->RxXferSize - huart->hdmarx->Instance->CNDTR;
    }
    int64_t bytesReady = dmaOffset - uartDmaRxBuffer->offset;
    if (bytesReady > 0) {
        while (uartDmaRxBuffer->offset < dmaOffset) {
            uint8_t c = uartDmaRxBuffer->buffer[uartDmaRxBuffer->offset];
            switch (c) {
                case '\0':
                case '\r':
                case '\n':
                    if (uartLineIndex > 0) {
                        uartLine[uartLineIndex++] = '\0';
                        //uint8_t *str = malloc(uartLineIndex);
                        //memcpy(str, uartLine, uartLineIndex);
                        CanHacker_Receive_Cmd(&hcanHacker, uartLine);
                        uartLineIndex = 0;
                        /*osStatus status = osMessagePut(uartRxQueueHandle, (uint32_t)str, osWaitForever);
                        switch (status) {
                            case osOK:
                                break;
                            default:
                                throwError("Error put to queue");
                                break;
                        }*/
                    }
                    break;
                default:
                    uartLine[uartLineIndex++] = c;
                    break;
            }
            uartDmaRxBuffer->offset++;
        }
        itemsProcessed++;
    }
    if (uartDmaRxBuffer->offset >= UART_RX_BUFFER) {
        uartDmaRxBuffer->readed = 1;
        uartDmaRxBuffer->offset = 0;
        uartDmaRxBuffer->filled = 0;
        gotoNextBufferToRead();
    }
    return itemsProcessed;
}

void CanHacker_ErrorCallback(CanHacker_HandleTypeDef *canHacker, char *message) {
    throwError(message);
}

void CanHacker_CanTxMsgReadyCallback(CanHacker_HandleTypeDef *canHacker, CanTxMsgTypeDef *txMsg) {
    CanTxMsgTypeDef *txMsgQ = malloc(sizeof(CanTxMsgTypeDef));
    memcpy(txMsgQ, txMsg, sizeof(CanTxMsgTypeDef));
    osMessagePut(canTxQueueHandle, (uint32_t) txMsgQ, osWaitForever);
    txCan();
}

void CanHacker_UartMsgReadyCallback(CanHacker_HandleTypeDef *canHacker, uint8_t *line) {
    unsigned int len = strlen((char *) line);
    char *str = malloc(len + 1);
    memcpy(str, line, len + 1);
    osMessagePut(uartTxQueueHandle, (uint32_t) str, osWaitForever);
    txUart();
}

uint32_t CanHacker_GetTimestampCallback(CanHacker_HandleTypeDef *canHacker) {
    return htim6.Instance->CNT;
}

void CanHacker_StartTimerCallback(CanHacker_HandleTypeDef *canHacker) {
    HAL_TIM_Base_Start(&htim6);
}

void CanHacker_StopTimerCallback(CanHacker_HandleTypeDef *canHacker) {
    HAL_TIM_Base_Stop(&htim6);
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const *argument) {
    /* USER CODE BEGIN 5 */
    /* Infinite loop */
    for (;;) {
        int itemsProcessed = 0;
        osEvent canEvent = osMessageGet(canRxQueueHandle, osWaitForever);
        if (canEvent.status = osEventMessage) {
            CanHacker_Receive_CanMsg(&hcanHacker, (CanRxMsgTypeDef *) canEvent.value.p);
            free(canEvent.value.p);
            itemsProcessed++;
        }
        if (!itemsProcessed) {
            osDelay(1);
        }
    }
    /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartTaskUartRxDmaTask */
/**
* @brief Function implementing the uartRxDmaTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTaskUartRxDmaTask */
void StartTaskUartRxDmaTask(void const *argument) {
    /* USER CODE BEGIN StartTaskUartRxDmaTask */
    currentUartRxBuffer = 0;
    currentUartRxBufferToRead = 0;
    startUartDmaReceive(&huart1, &uartRxBuffer[currentUartRxBuffer]);
    /* Infinite loop */
    for (;;) {
        int itemsProcessed = processUartDMABuffer(&huart1, &uartRxBuffer[currentUartRxBufferToRead]);
        if (!itemsProcessed) {
            osDelay(1);
        }
    }
    /* USER CODE END StartTaskUartRxDmaTask */
}

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM1 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
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
void Error_Handler(void) {
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
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
