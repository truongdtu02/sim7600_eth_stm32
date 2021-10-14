/*
 * sim7600_uart_dma.h
 *
 *  Created on: May 2, 2021
 *      Author: Bom
 */

//dma need to define with circle gmode end trans single byte

//baudrate

#ifndef INC_SIM7600_UART_DMA_H_
#define INC_SIM7600_UART_DMA_H_
#include <mp3Frame.h>
#include "main.h"
#include "stm32f407xx.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "stdbool.h"
#include "cmsis_os.h"
#include "stdio.h"
#include "tcp_udp_stack.h"

#define RECV_SIM_TASK_INTERVAL 5 //50ms
// #define sim_dma_buff_size 10000

#define RECONNECT_INTERVAL 10000 // 2s

#define SIM_BUFF_SIZE_FULL  10000 //
#define SIM_BUFF_SIZE_MINI  300 //

#define SIM7600_BAUDRATE_DEFAULT 115200//115200
#define SIM7600_BAUDRATE_MAIN 3000000//3000000 //3 Mbps
//LL_USART_SetBaudRate (USART_TypeDef * USARTx, uint32_t PeriphClk,uint32_t OverSampling, uint32_t BaudRate)
#define PCLK2_FREQ 84000000 //84MHz
//#define sim7600Baudrate(x) UART_BRR_SAMPLING16(HAL_RCC_GetPCLK2Freq(), x)
#define sim7600SetBaudrate(x) LL_USART_SetBaudRate(USART1, PCLK2_FREQ, LL_USART_OVERSAMPLING_16, x)

#define pwrSIM_Pin LL_GPIO_PIN_0
#define sttSIM_Pin LL_GPIO_PIN_2
#define rstSIM_Pin LL_GPIO_PIN_1

#define pwrSIM_GPIO_Port GPIOE
#define rstSIM_GPIO_Port GPIOE
#define sttSIM_GPIO_Port GPIOE

//CTS sim <-> RTS uart stm32
#define CTS_SIM_Pin LL_GPIO_PIN_12
#define RTS_SIM_Pin LL_GPIO_PIN_11
#define CTS_SIM_GPIO_Port GPIOA

//after MAX_NUM_RESTART_SIM7600 + MAX_NUM_RESTART_SIM7600 , but can't success
//sim7600 will sleep in ... minutes  (SLEEP_MINUTES_SIM7600)
#define MAX_NUM_RESET_SIM7600 2
#define MAX_NUM_RESTART_SIM7600 20
#define SLEEP_MINUTES_SIM7600 12*60 //~12h //(max 1193 * 60)

//max data length of response : \r\n ...respose...\r\n
#define MAX_DATA_LENGTH_OF_RESPONSE_R_N 100

//send ip error count max, if send_error_time >= this number, we will close tcp/udp socket and reconnect
#define SEND_IP_ERROR_COUNT_MAX 5

//#define Sim_PWR(x) x ? HAL_GPIO_WritePin(pwrSIM_GPIO_Port, pwrSIM_Pin, GPIO_PIN_SET) : HAL_GPIO_WritePin(pwrSIM_GPIO_Port, pwrSIM_Pin, GPIO_PIN_RESET)
#define Sim_PWR(x) x ? LL_GPIO_SetOutputPin(pwrSIM_GPIO_Port, pwrSIM_Pin) : LL_GPIO_ResetOutputPin(pwrSIM_GPIO_Port, pwrSIM_Pin)

//#define Sim_RST(x) x ? HAL_GPIO_WritePin(rstSIM_GPIO_Port, rstSIM_Pin, GPIO_PIN_SET) : HAL_GPIO_WritePin(rstSIM_GPIO_Port, rstSIM_Pin, GPIO_PIN_RESET)
#define Sim_RST(x) x ? LL_GPIO_SetOutputPin(rstSIM_GPIO_Port, rstSIM_Pin) : LL_GPIO_ResetOutputPin(rstSIM_GPIO_Port, rstSIM_Pin)
#define Sim_CTS(x) x ? LL_GPIO_SetOutputPin(CTS_SIM_GPIO_Port, CTS_SIM_Pin) : LL_GPIO_ResetOutputPin(CTS_SIM_GPIO_Port, CTS_SIM_Pin)

#define SIM7600_PAUSE_RX()  Sim_CTS(1)
// #define SIM7600_PAUSE_RX()   LL_USART_Disable(USART1); while (LL_USART_IsEnabled(USART1)); //waite until En bit == 0

#define SIM7600_RESUME_RX() Sim_CTS(0)
// #define SIM7600_RESUME_RX()  LL_USART_Enable(USART1); while (!LL_USART_IsEnabled(USART1)); //waite until En bit == 1
// #define SIM7600_RESUME_RX()  LL_USART_Enable(USART1); while (!LL_USART_IsEnabled(USART1)); //waite until En bit == 1

//read status
#define Sim_STT  (LL_GPIO_ReadInputPort(sttSIM_GPIO_Port) & sttSIM_Pin)

//led status TCP connect, turn on when success, off when not
#define sim7600_tcp_led_status(x) x ? LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_6) : LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_6)
//led status toggle when tcp is connecting
#define sim7600_tcp_led_connecting_toggle LL_GPIO_TogglePin(GPIOA, LL_GPIO_PIN_6)
#define sim7600_tcp_led_connecting_off LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_7)
// Calculate length of statically allocated array

// Buffer for USART DMA
// Contains RAW unprocessed data received by UART and transfered by DMA

typedef struct  {
  osEventFlagsId_t EventID;
  uint8_t type; // 0: normal cmd, 1: send TCP packet, 2 send UDP packet
  const uint8_t *ptr;
  int len;
  const char *res1, *res2;
  int timeout;
} sendSimPack;

enum configSimFlagEnum {smsEnum, callingEnum, simErrorEnum, rebootEnum} ;
enum connectSimFlagEnum {openConEnum, openNetEnum, doneTLSEnum, netErrorEnum, ipCloseEnum, sendIpErrorEnum}; //in sim7600 .h

__STATIC_INLINE void sim7600_delay_ms(int _ms)
{
	osDelay(_ms);
}

//check string str contain substr ?
__STATIC_INLINE bool stringContain(const char* str, const char* substr)
{
 if(strstr(str, substr) != NULL) return true;
 return false;
}

void sim7600_init(bool isMini);

bool sim7600_fullConfig();
bool sim7600_miniConfig();

void sim7600_usart_rx_check();
void sim7600_usart_send_byte(const void* data, int len);
void sim7600_usart_send_string(const char* str);
void sim7600_change_baud(uint32_t baudrate);
void sim7600_restart();



int sim7600_handle_received_data();
int sim7600_send_cmd(const char *cmd, const char *response1, const char *response2, int timeout);
void sim7600_update_response(const char* _res1, const char* _res2);
bool sim7600_send_packet_ip(int type, const uint8_t* data, int data_length);

void sim7600_fullConfigTask();
void sim7600_connectTask();
void sim7600_sendTask();
void sim7600_recvTask();
int sim7600_AT(const char* cmd, const char* response1, const char* response2, int timeout, int try);
bool sim7600_IP(int type, uint8_t *data, int len);

typedef struct
{
    uint16_t checkSumHeader; //encoded (reserved)
    int IDframe; //ID of start frame
    int songID;
	uint8_t frame[];
//} packetMP3HeaderStruct;
} __attribute__ ((packed)) packetMP3HeaderStruct;

#endif /* INC_SIM7600_UART_DMA_H_ */
