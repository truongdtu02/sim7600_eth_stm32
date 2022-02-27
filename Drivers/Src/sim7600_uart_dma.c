/*
 * sim7600_uart_dma.c
 *
 *  Created on: May 2, 2021
 *      Author: Bom
 */

#include "sim7600_uart_dma.h"
#include "debugAPI.h"

// uint8_t *sim_dma_buff; //circle buffer
// uint8_t *sim_buff;

uint8_t sim_dma_buff[15000]; //circle buffer
uint8_t sim_buff[15001]; //circle buffer

int sim_buff_length;
int sim_dma_buff_size;

//don't want res1, res2 content be changed
const char* res1;
const char* res2;

//global var
// char serverDomain[100] = "iothtnhust20201.xyz";
char serverDomain[100] = "45.118.145.137";
// char serverDomain[100] = "4.tcp.ngrok.io"; //"45.118.145.137";
int serverPort = 5000; //1308;
int connectToHostCount = 0;
int restartSimstatus = 0;

extern osEventFlagsId_t ConfigSimEventID;
extern osEventFlagsId_t ConnectSimEventID;
extern osThreadId_t connectSimTaskHandle;

int cmdSendStatus = 0; // 1: sending, 0: none, *** 2:beggin send ip packet (wait '>')
extern osEventFlagsId_t SendSimEventID;
extern osMessageQueueId_t SendSimQueueID;

int send_ip_error_count = 0;

int sim7600ConnectStatus = 0; // 0:no-TCP-connect, 1: successful TCP handshake, 2: successful open TLS-TCP and UDP socket

int sim7600ConnectTime = 0;
int sim7600DisconnectTime = 0;

bool bSim7600IsRunning = false;

void sim7600_powerON()
{
  LOG_WRITE("powerOn\n");  
  Sim_PWR(0);
  osDelay(500);
  Sim_PWR(1);
  osDelay(500);
  Sim_PWR(0);
  osDelay(1000); //margin

  //wait until sim_status == 1;, time-out 60s
  int try = 6000;
  int count = 0;
  while (try--)
  {
    if (Sim_STT > 0)
      count++;
    else
      count = 0;
    if(count > 10)
      break;
    osDelay(10);
  }
  osDelay(10000); //max time

  bSim7600IsRunning = true;
}

void sim7600_powerOFF()
{
  bSim7600IsRunning = false;

  LOG_WRITE("powerOff\n");  
  Sim_PWR(0);
  osDelay(500);
  Sim_PWR(1);
  osDelay(4000);
  Sim_PWR(0);
  osDelay(1000); //margin

  //wait until sim_status == 0;, time-out 60s
  int try = 6000;
  int count = 0;
  while (try--)
  {
    if (Sim_STT == 0)
      count++;
    else
      count = 0;
    if(count > 10) //remove noise
      break;
    osDelay(10);
  }
  osDelay(30000); //max time
}

void sim7600_reset()
{

  LOG_WRITE("reset\n");  

  Sim_RST(0);
  osDelay(500);
  Sim_RST(1);
  osDelay(400);
  Sim_RST(0);
  osDelay(30000);
}

void sim7600_gpio_init()
{
  LOG_WRITE("sim7600 gpioInit\n");  

  //gpio init
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOE);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);

  //set default state at begin (high or low depend on hardware / circuit)
  LL_GPIO_ResetOutputPin(pwrSIM_GPIO_Port, pwrSIM_Pin | rstSIM_Pin);

  GPIO_InitStruct.Pin = pwrSIM_Pin | rstSIM_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  LL_GPIO_Init(pwrSIM_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = sttSIM_Pin; //status pin
  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN; //need pull down to read exactly value from (sim_status -> TXS0108EPWR)
  LL_GPIO_Init(sttSIM_GPIO_Port, &GPIO_InitStruct);

  LL_GPIO_ResetOutputPin(CTS_SIM_GPIO_Port, CTS_SIM_Pin);
  GPIO_InitStruct.Pin = CTS_SIM_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(CTS_SIM_GPIO_Port, &GPIO_InitStruct);
}

//initialize UART, DMA, GPIO for stm32-sim7600 to connect Internet + gps (full) or just use jps (mini)
void sim7600_init(bool isMini)
{
  LOG_WRITE("simInit\n");
  sim_dma_buff_size = 15000;

  // if (isMini)
  // {
  //   //init memory - buffer mini
  //   sim_dma_buff_size = SIM_BUFF_SIZE_MINI;
  //   sim_dma_buff = (uint8_t *)malloc(SIM_BUFF_SIZE_MINI); //circle buffer
  //   sim_buff = (uint8_t *)malloc(SIM_BUFF_SIZE_MINI + 1); //real data received (+1 bytes for '\0' end of string)
  // }
  // else
  // {
  //   //init memory - buffer full
  //   sim_dma_buff_size = SIM_BUFF_SIZE_FULL;
  //   sim_dma_buff = (uint8_t *)malloc(SIM_BUFF_SIZE_FULL); //circle buffer
  //   sim_buff = (uint8_t *)malloc(SIM_BUFF_SIZE_FULL + 1); //real data received (+1 bytes for '\0' end of string)
  // }

  sim7600_gpio_init();

  LL_USART_Disable(USART1);
  while (LL_USART_IsEnabled(USART1)); //waite until En bit == 0
  
  //disable DMA
  LL_DMA_DisableStream(DMA2, LL_DMA_STREAM_2);
  while (LL_DMA_IsEnabledStream(DMA2, LL_DMA_STREAM_2))
    ; //wait until En bit == 0

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  //uart1 + DMA2 stream 2, channel 2 init
  LL_USART_InitTypeDef USART_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2);
  /*
       * USART1 GPIO Configuration
       *
       * PA9    ------> USART1_TX
       * PA10   ------> USART1_RX
       * PA11   ------> USART1_CTS
       * PA12   ------> USART1_RTS
       */
  // GPIO_InitStruct.Pin = LL_GPIO_PIN_9 | LL_GPIO_PIN_10 |LL_GPIO_PIN_11|LL_GPIO_PIN_12;
  GPIO_InitStruct.Pin = LL_GPIO_PIN_9 | LL_GPIO_PIN_10 |LL_GPIO_PIN_11; //debug
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_7; // AF_7 ~ USART1..3 see at datasheet (Figure.. Selecting an alternate function...)
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USART1 RX DMA 2 stream 2 channel 4 Init */

  LL_DMA_SetChannelSelection(DMA2, LL_DMA_STREAM_2, LL_DMA_CHANNEL_4);
  LL_DMA_SetDataTransferDirection(DMA2, LL_DMA_STREAM_2, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
  LL_DMA_SetStreamPriorityLevel(DMA2, LL_DMA_STREAM_2, LL_DMA_PRIORITY_HIGH);
  LL_DMA_SetMode(DMA2, LL_DMA_STREAM_2, LL_DMA_MODE_CIRCULAR);
  LL_DMA_SetPeriphIncMode(DMA2, LL_DMA_STREAM_2, LL_DMA_PERIPH_NOINCREMENT);
  LL_DMA_SetMemoryIncMode(DMA2, LL_DMA_STREAM_2, LL_DMA_MEMORY_INCREMENT);
  LL_DMA_SetPeriphSize(DMA2, LL_DMA_STREAM_2, LL_DMA_PDATAALIGN_BYTE);
  LL_DMA_SetMemorySize(DMA2, LL_DMA_STREAM_2, LL_DMA_MDATAALIGN_BYTE);
  LL_DMA_DisableFifoMode(DMA2, LL_DMA_STREAM_2);
  LL_DMA_SetPeriphAddress(DMA2, LL_DMA_STREAM_2, (uint32_t)&USART1->DR);

  //set uart rx buffer receive
  LL_DMA_SetMemoryAddress(DMA2, LL_DMA_STREAM_2, (uint32_t)sim_dma_buff);
  LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_2, sim_dma_buff_size);

  /* USART configuration */
  USART_InitStruct.BaudRate = SIM7600_BAUDRATE_DEFAULT;
  USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
  USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
  USART_InitStruct.Parity = LL_USART_PARITY_NONE;
  USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_RTS_CTS; //LL_USART_HWCONTROL_NONE;
  USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
  LL_USART_Init(USART1, &USART_InitStruct);
  LL_USART_ConfigAsyncMode(USART1);
  LL_USART_EnableDMAReq_RX(USART1);

  /* Enable USART and DMA */
  LL_DMA_EnableStream(DMA2, LL_DMA_STREAM_2);
  LL_USART_Enable(USART1);

  //wait until USART DMA is ready
  while (!LL_USART_IsEnabled(USART1) || !LL_DMA_IsEnabledStream(DMA2, LL_DMA_STREAM_2))
    ;

  //power off to debug (don't need to plug out sim7600)
//  sim7600_powerOFF();

  //power on sim7600
  sim7600_powerON();
}

void sim7600_update_response(const char *_res1, const char *_res2)
{
  LOG_WRITE("UpdateRes\n");
  if (_res1 != NULL && strlen(_res1) > 0)
    res1 = _res1;
  else
    res1 = NULL;

  if (_res2 != NULL && strlen(_res2) > 0)
    res2 = _res2;
  else
    res2 = NULL;
}

void sim7600_usart_send_string(const char *str)
{
  LOG_WRITE("sendStr\n");
  if(str != NULL)
  {
    int tmpLen = strlen(str);
    if(tmpLen > 0)
      sim7600_usart_send_byte(str, tmpLen);
  }
}

// __STATIC_INLINE void LL_USART_TransmitData8(USART_TypeDef *USARTx, uint8_t Value)
// {
//   USARTx->DR = Value;
// }
// __STATIC_INLINE uint32_t LL_USART_IsActiveFlag_TXE(USART_TypeDef *USARTx)
// {
//   return (READ_BIT(USARTx->SR, USART_SR_TXE) == (USART_SR_TXE));
// }
// __STATIC_INLINE uint32_t LL_USART_IsActiveFlag_TC(USART_TypeDef *USARTx)
// {
//   return (READ_BIT(USARTx->SR, USART_SR_TC) == (USART_SR_TC));
// }
void sim7600_usart_send_byte(const void *data, int len)
{
  LOG_WRITE("sendBytes\n");
  const uint8_t *d = (uint8_t*)data;

  for (; len > 0; --len, ++d)
  {
    LL_USART_TransmitData8(USART1, *d);
    while (!LL_USART_IsActiveFlag_TXE(USART1))
      ;
  }
  while (!LL_USART_IsActiveFlag_TC(USART1))
    ;
}

//send direct AT cmd, return 0: fail, 1,2: response 1,2
int sim7600_send_cmd(const char *cmd, const char *response1, const char *response2, int timeout)
{
  LOG_WRITE("sendCMD\n");
  uint32_t sendSimFlag = 0;
  sim7600_update_response(response1, response2);
  sim7600_usart_send_string(cmd);
  cmdSendStatus = 1;
  sendSimFlag = osEventFlagsWait(SendSimEventID, 3, osFlagsWaitAny, timeout); //500ms receive max 100 bytes with baud = 115200
  cmdSendStatus = 0;
  if(sendSimFlag == 1U || sendSimFlag == 2U)
  {
    return sendSimFlag;
  }
  else if(sendSimFlag != osFlagsErrorTimeout)
  {
    LOG_WRITE("sim send cmd eror\n");
    osEventFlagsSet(ConfigSimEventID, 1 << simErrorEnum); //restart + reset
  }
  return 0;
}

//push AT cmd to queue, return 0: fail, 1,2: response 1,2
int sim7600_AT(const char *cmd, const char *response1, const char *response2, int timeout, int try)
{
  LOG_WRITE("simAT\n");
  sendSimPack sendMsgObj;
  osStatus_t sendMsgStt;
  uint32_t sendMsgFlag = 0;

  osEventFlagsId_t SimATEventID = osEventFlagsNew(NULL);
  if (SimATEventID == NULL)
  {
    LOG_WRITE("SimATEventID eror\n");
    osEventFlagsSet(ConfigSimEventID, 1 << simErrorEnum); //restart + reset
    return 0;
  }

  //put data
  sendMsgObj.EventID = SimATEventID;
  sendMsgObj.ptr = cmd;
  sendMsgObj.res1 = response1;
  sendMsgObj.res2 = response2;
  sendMsgObj.timeout = timeout;
  sendMsgObj.type = 0;
  while (try-- > 0)
  {
    sendMsgStt = osMessageQueuePut(SendSimQueueID, &sendMsgObj, 0U, 0U);
    if (sendMsgStt == osOK)
    {
      sendMsgFlag = osEventFlagsWait(SimATEventID, 3, osFlagsWaitAny, timeout + 100); //+100 margin
      if (sendMsgFlag == 1 || sendMsgFlag == 2) 
      {
        osEventFlagsDelete(SimATEventID);
        return sendMsgFlag;
      }
      else if(sendMsgFlag != osFlagsErrorTimeout) //flag error
      {
        LOG_WRITE("SendSimQueueID eror1\n");
        osEventFlagsSet(ConfigSimEventID, 1 << simErrorEnum); //restart + reset
        break;
      }
    }
    else if(sendMsgStt == osErrorTimeout || sendMsgStt == osErrorResource)
    {
      if(try > 0)
        osDelay(timeout);
    }
    else //msg queue error
    {
      LOG_WRITE("SendSimQueueID error2\n");
      osEventFlagsSet(ConfigSimEventID, 1 << simErrorEnum); //restart + reset
      break;
    }
  }
  osEventFlagsDelete(SimATEventID);
  return 0; //fail
}

//send AT and set simError flag if error, return false if send error
int sim7600_AT_notify_error(const char *cmd, const char *response1, const char *response2, int timeout, int try)
{
  LOG_WRITE("simATnotify\n");
  int tmpFlag = sim7600_AT(cmd, response1, response2, timeout, try);
  if (tmpFlag == 0) //error
  {
    //set bit simError in ConfigSimEventID to restart and re config
    LOG_WRITE("sim7600_AT_notify error\n");
    osEventFlagsSet(ConfigSimEventID, (1 << simErrorEnum));
  }
  return tmpFlag;
}

//debug
uint32_t waitTimecmdSendStatus2;

//send packet, type:1 - TCP , type:2 - UDP; return true: send successful, false: fail
bool sim7600_send_packet_ip(int type, const uint8_t *data, int data_length)
{
  LOG_WRITE("sendIP %d\n", type);
  //can send udp when sim7600ConnectStatus  == 2 and tcp when sim7600ConnectStatus >= 1;
  char *sim7600_cmd_buff; //default null
  if (type == 2 && sim7600ConnectStatus == 2)
  { 
    // udp
    if (serverDomain != NULL)
    {
      int tmpLen = strlen(serverDomain);
      if (tmpLen > 0)
      {
        sim7600_cmd_buff = (char *)malloc(tmpLen + 35);
        if (sim7600_cmd_buff != NULL)
          sprintf(sim7600_cmd_buff, "AT+CIPSEND=1,%d,\"%s\",%d\r", data_length, serverDomain, serverPort);
      }
    }
  }
  else if (type == 1 && sim7600ConnectStatus >= 1)
  { 
    //tcp
    sim7600_cmd_buff = (char*)malloc(25);
    if(sim7600_cmd_buff != NULL)
      sprintf(sim7600_cmd_buff, "AT+CIPSEND=0,%d\r", data_length);
  }
  else
  {
    return false;
  }

  if (sim7600_cmd_buff == NULL) //can't allocate memory
  {
    LOG_WRITE("sim7600_send_packet_ip allocate error\n");
    osEventFlagsSet(ConfigSimEventID, (1 << simErrorEnum));
    return false;
  }

  sim7600_usart_send_string(sim7600_cmd_buff);
  free(sim7600_cmd_buff);
  // sim7600_update_response(">",""); //not necessaary, see sim7600_handle_received to know detail, at line cmdSendStatus == 2
  cmdSendStatus = 2; // -> ipSendStatus

  //debug
  uint32_t tmpCNT = TIM2->CNT;

  uint32_t sendSimFlag = osEventFlagsWait(SendSimEventID, 1U, osFlagsWaitAll, 500); //500ms receive max 100 bytes with baud = 115200

  //debug
  waitTimecmdSendStatus2 = (TIM2->CNT - tmpCNT) >> 1;

  if (sendSimFlag == 1U)
  { //recevied >
    sim7600_update_response("+CIPSEND", "+CIPERROR");
    sim7600_usart_send_byte(data, data_length);
    cmdSendStatus = 1; // -> ipSendStatus
    sendSimFlag = osEventFlagsWait(SendSimEventID, 3U, osFlagsWaitAny, 5000);
  }
  cmdSendStatus = 0; // reset
  if(sendSimFlag == 1U) //send success full
  {
    send_ip_error_count = 0;
    return true;
  }
  else if(sendSimFlag == 2U)
  {
    send_ip_error_count++;
    if(send_ip_error_count >= SEND_IP_ERROR_COUNT_MAX)
    {
      send_ip_error_count = 0;
      osEventFlagsSet(ConnectSimEventID, 1 << ipCloseEnum); //re-CONNECT
    }
  }
  else //don't recv response -> something wrong
  {
    LOG_WRITE("sendSimFlag error\n");
    osEventFlagsSet(ConfigSimEventID, 1 << simErrorEnum); //restart, reset
  }
  return false;
}

//send IP packet by put to send sim queue. type: 2-udp, 1-tcp
bool sim7600_IP(int type, uint8_t *data, int len)
{
  LOG_WRITE("simIP %d\n", type);;
  if(type != 1 && type != 2)
  {
    return false;
  }
  sendSimPack sendMsgObj;
  sendMsgObj.ptr = data;
  sendMsgObj.len = len;
  sendMsgObj.type = type;
  osStatus_t sendMsgStt = osMessageQueuePut(SendSimQueueID, &sendMsgObj, 0U, 0U);

  return (sendMsgStt == osOK);
}

//mini config for just GPS
bool sim7600_miniConfig()
{
  //reverse

  return true;
}
int baudrate_arr[] = {300, 600, 1200, 2400, 4800, 9600, 19200,
38400, 57600, 115200, 230400, 460800, 921600, 3000000};
//full config for internet
bool sim7600_fullConfig()
{
  LOG_WRITE("fullConfig\n");
  char sim7600_cmd_buff[25];
  //change to main baudrate permantly
  // sprintf(sim7600_cmd_buff, "AT+IPR=%d\r\n", SIM7600_BAUDRATE_MAIN); //temporarily
  sprintf(sim7600_cmd_buff, "AT+IPREX=%d\r\n", SIM7600_BAUDRATE_MAIN); //permantly, prevent receive "...SMS DONE..." after reboot/power-up
  sim7600_AT(sim7600_cmd_buff, "OK", NULL, 500, 2);              //don't check for the first config
  
  // int j;
  // for(j = 0; j < sizeof(baudrate_arr) / 4; j++) {
  //   sim7600_change_baud(baudrate_arr[j]);
  //   osDelay(10);
  //   if (sim7600_AT("ATE0\r\n", "OK", NULL, 500, 2)) {
  //     printf("oke\n");
  //   }
  // }
  // j = 0;
  // for(j = 0; j < sizeof(baudrate_arr) / 4; j++) {
  //   sim7600_change_baud(baudrate_arr[j]);
  //   osDelay(10);
  //   if (sim7600_AT("ATE0\r\n", "OK", NULL, 500, 2)) {
  //     printf("oke\n");
  //   }
  // }
  if (!sim7600_AT(sim7600_cmd_buff, "OK", NULL, 500, 2))
    return false;
  //echo cmd off
  if (!sim7600_AT("ATE0\r\n", "OK", NULL, 500, 2))
    return false;
  restartSimstatus = 0; //reset

  //flow control AT+IFC=2,2 (sim_RTS, sim_CTS respectively)
  if (!sim7600_AT("AT+IFC=2,2\r\n", "OK", NULL, 500, 2))
    return false;

  //check sim
  if (!sim7600_AT("at+ciccid\r\n", "OK", NULL, 500, 2))
    return false;
  if (!sim7600_AT("at+csq\r\n", "OK", NULL, 500, 2))
    return false;

  // set timeout value for AT+NETOPEN/AT+CIPOPEN/AT+CIPSEND
  //AT+CIPTIMEOUT=10000,10000,5000 ~ 10s, 10s, 5s
  if (!sim7600_AT("AT+CIPTIMEOUT=10000,10000,5000\r\n", "OK", NULL, 500, 2)) {
	  printf("f1\n");
	  //return false;
  }

  //config parameters of socket
  //10 times retranmission IP packet, no(0) delay to output data received
  //ack=0, 1:error result code with string values
  //1:add data header, the format is â€œ+RECEIVE,<link num>,<data length>â€�
  //< AsyncMode > = 0
  //minimum retransmission timeout value for TCP connection in ms : 500
  if (!sim7600_AT("AT+CIPCCFG=10,0,0,1,1,0,500\r\n", "OK", NULL, 500, 2)){
	  printf("f2\n");
	  //return false;
  }

  //display header when receive â€œ+RECEIVE,<link num>,<data length>
  //AT+CIPHEAD=1 : \r\nOK\r\n
  if (!sim7600_AT("AT+CIPHEAD=1\r\n", "OK", NULL, 500, 2))
    return false;

  //don't display remote IP (server ip)
  //AT+CIPSRIP=0 : \r\nOK\r\n
  if (!sim7600_AT("AT+CIPSRIP=0\r\n", "OK", NULL, 500, 2))
    return false;

  //gps AT+CGPS=1,1 AT+CGPSINFO
//  if (!sim7600_AT("AT+CGPS=1,3\r\n", "OK", NULL, 500, 2))
//    return false;
//
//  if (!sim7600_AT("AT+CGPSINFO\r\n", "OK", NULL, 500, 2))
//    return false;

  return true;
}

void sim7600_fullConfigTask()
{
  // char testt[] = "+fa+af+as\r\nRDY\r\n\r\n+CPIN: READY\r\n\r\nSMS DONE\r\n\r\nPB DONE\r\n";
  // testt[2] = 0;
  // testt[5] = 0;
  // testt[8] = 0;
  // memcpy(sim_buff, testt, sizeof(testt));

  // int tmp0;
  // for (tmp0 = 0; tmp0 < sizeof(testt); tmp0++)
  // {
  //   if (sim_buff[tmp0] == '+')
  //   {
  //     if (strstr(sim_buff + tmp0, "+CPIN: READY\r\n") != NULL)
  //     {
  //       if ((strstr(sim_buff + tmp0, "SMS DONE\r\n") != NULL) && (strstr(sim_buff + tmp0, "PB DONE\r\n") != NULL))
  //       {
  //         LOG_WRITE("sim is rebooted\n");
  //         osEventFlagsSet(ConfigSimEventID, 1 << rebootEnum);
  //         return sim_buff_length;
  //       }
  //     }
  //   }
  // }


  LOG_WRITE("fullConfigTask\n");
  //first config
  sim7600_init(false);
  if(sim_buff == NULL || sim_dma_buff == NULL) //can't allocate memory
  {
    LOG_WRITE("sim7600_fullConfigTask error0\n");
    osEventFlagsSet(ConfigSimEventID, (1 << simErrorEnum)); //restart sim7600 and reset stm32
  }
  else
  {
    if (sim7600_fullConfig())
    {
      //set event to invoke connect task open net and open socket tcp, udp
      osEventFlagsSet(ConnectSimEventID, 1 << openNetEnum);
      osEventFlagsSet(ConnectSimEventID, 1 << openConEnum);
      // osThreadFlagsSet(connectSimTaskHandle, 1 << reConEnum);
    }
    else
    { //error
      LOG_WRITE("sim7600_fullConfigTask error1\n");
      //set bit simError in ConfigSimEventID to restart and re-config
      osEventFlagsSet(ConfigSimEventID, (1 << simErrorEnum));
    }
  }
  ////
  int configSimFlag;
  for (;;)
  {
    configSimFlag = osEventFlagsWait(ConfigSimEventID, 0xFF, osFlagsNoClear | osFlagsWaitAny, osWaitForever);
    LOG_WRITE("configSimFlag %d\n", configSimFlag);
    if(configSimFlag < 0) //error 0xFFFFFFF...U
    {
      LOG_WRITE("configSimFlag errorFlag\n");
      LOG_WRITE("sim7600_fullConfigTask error2\n");
      osEventFlagsSet(ConfigSimEventID, (1 << simErrorEnum)); //restart + reset
    }
    //analyze follow priority
    else if (configSimFlag & (1 << simErrorEnum))
    {
      LOG_WRITE("configSimFlag simError\n");
      osEventFlagsClear(ConfigSimEventID, 1 << simErrorEnum);
      //restart then clear all flag, var
      sim7600_restart(); //restart + reset
      if (sim7600_fullConfig())
      {
        //set event to invoke connect task open net and open socket tcp, udp
        osEventFlagsSet(ConnectSimEventID, 1 << openNetEnum);
        osEventFlagsSet(ConnectSimEventID, 1 << openConEnum);
        // osThreadFlagsSet(connectSimTaskHandle, 1 << reConEnum);
      }
      else
      { //error
        LOG_WRITE("sim7600_fullConfigTask error3\n");
        //set bit simError in ConfigSimEventID to restart and re config
        osEventFlagsSet(ConfigSimEventID, (1 << simErrorEnum));
      }
    }
    else if(configSimFlag & (1 << rebootEnum))
    {
      LOG_WRITE("configSimFlag rebootEnum\n");
      osEventFlagsClear(ConfigSimEventID, 1 << rebootEnum);
      if (sim7600_fullConfig())
      {
        //set event to invoke connect task open net and open socket tcp, udp
        osEventFlagsSet(ConnectSimEventID, 1 << openNetEnum);
        osEventFlagsSet(ConnectSimEventID, 1 << openConEnum);
        // osThreadFlagsSet(connectSimTaskHandle, 1 << reConEnum);
      }
      else
      { //error
        LOG_WRITE("sim7600_fullConfigTask error4\n");
        //set bit simError in ConfigSimEventID to restart and re config
        osEventFlagsSet(ConfigSimEventID, (1 << simErrorEnum));
      }
    }
    else if (configSimFlag & (1 << callingEnum))
    {
      LOG_WRITE("configSimFlag calling\n");
      osEventFlagsClear(ConfigSimEventID, 1 << callingEnum);
      //send AT+CHUP to end call
      sim7600_AT_notify_error("AT+CHUP", "OK", NULL, 500, 2);
    }
    // else if (configSimFlag & (1 << IPerrorEnum))
    // {
    //   //reconnect TCP, UDP
    //   osEventFlagsClear(ConfigSimEventID, 1 << IPerrorEnum);
    //   // osEventFlagsSet(ConnectSimEventID, 1U);
    //   osThreadFlagsSet(connectSimTaskHandle, 1 << reConEnum);
    // }
    else if (configSimFlag & (1 << smsEnum))
    {
      LOG_WRITE("configSimFlag sms\n");
      osEventFlagsClear(ConfigSimEventID, 1 << smsEnum);
      //delete all msg
      // if(sim7600_delete_all_msg()) {
      // } else { //error
      //   osEventFlagsSet(ConfigSimEventID, (1 << netErrorEnum));
      // }
    }
    else
    { //something wrong
      LOG_WRITE("configSimFlag sthElse\n");
      LOG_WRITE("sim7600_fullConfigTask error5\n");
      //set bit simError in ConfigSimEventID to restart and re config
      osEventFlagsSet(ConfigSimEventID, (1 << simErrorEnum));
    }
  }
}

void sim7600_connectTask()
{
  LOG_WRITE("connectTask\n");
  int connectSimFlag;
  for (;;)
  {
    // connectSimFlag = osThreadFlagsWait(0xFF, osFlagsWaitAny | osFlagsNoClear, osWaitForever);
    connectSimFlag = osEventFlagsWait(ConnectSimEventID, 0xFF, osFlagsNoClear | osFlagsWaitAny, osWaitForever);
    LOG_WRITE("connectSimFlag %d\n", connectSimFlag);
    if (connectSimFlag < 0) //error 0xFFFFFFF...U
    {
      LOG_WRITE("connectSimFlag errorFlag\n");
      LOG_WRITE("sim7600_connectTask error0\n");
      osEventFlagsSet(ConfigSimEventID, (1 << simErrorEnum)); //restart + reset
    }
    else if (connectSimFlag & (1 << netErrorEnum))
    {
      LOG_WRITE("connectSimFlag netError\n");
      osEventFlagsClear(ConnectSimEventID, 1 << netErrorEnum);

      //release tcp/udp stack
      sim7600ConnectStatus = 0;
      TCP_UDP_Stack_Release();

      //close IP socket (tcp, udp)
      if(!sim7600_AT_notify_error("AT+CIPCLOSE=0\r\n", "OK", "+CIPCLOSE: 0,2", 10000, 2)) continue; //tcp //+CIPCLOSE: 0,0
      if(!sim7600_AT_notify_error("AT+CIPCLOSE=1\r\n", "OK", "+CIPCLOSE: 1,2", 10000, 2)) continue; //udp //+CIPCLOSE: 1,0
      //close net
      if (!sim7600_AT_notify_error("AT+NETCLOSE\r\n", "OK", "ERROR", 10000, 2)) continue; //+NETCLOSE: 0

      //set flag to open net and tcp/udp again
      osEventFlagsSet(ConnectSimEventID, 1 << openNetEnum);
      osEventFlagsSet(ConnectSimEventID, 1 << openConEnum);
    }
    else if (connectSimFlag & (1 << openNetEnum))
    {
      LOG_WRITE("connectSimFlag openNet\n");
      osEventFlagsClear(ConnectSimEventID, 1 << openNetEnum);

      //open network
      if (!sim7600_AT_notify_error("AT+NETOPEN\r\n", "OK", NULL, 10000, 2)) continue; //+NETOPEN: 0
    }
    else if (connectSimFlag & (1 << ipCloseEnum))
    {
      LOG_WRITE("connectSimFlag ipClose\n");
      sim7600DisconnectTime++;

      osEventFlagsClear(ConnectSimEventID, 1 << ipCloseEnum);

      //release tcp/udp stack
      sim7600ConnectStatus = 0;
      TCP_UDP_Stack_Release();

      //maybe tcp/udp, one of them is closed, so to make sure before open tcp/udp connect, release all
      //close IP socket (tcp, udp)
      if(!sim7600_AT_notify_error("AT+CIPCLOSE=0\r\n", "OK", "+CIPCLOSE: 0,2", 10000, 2)) continue;
      if(!sim7600_AT_notify_error("AT+CIPCLOSE=1\r\n", "OK", "+CIPCLOSE: 1,2", 10000, 2)) continue;

      //then set openCon flag to re-open tcp/udp connect
      osEventFlagsSet(ConnectSimEventID, 1 << openConEnum);
    }
    else if ((connectSimFlag & (1 << openConEnum)) && sim7600ConnectStatus == 0)
    {
      LOG_WRITE("connectSimFlag openCon\n");
      sim7600ConnectTime++;

      osEventFlagsClear(ConnectSimEventID, 1 << openConEnum);

      //check connectToHostCount, if > connectToHostCountMax then change serverDomain, serverport
      //open tcp connect
      char *sim7600_cmd_buff;
      if(serverDomain != NULL)
      {
        int tmpLen = strlen(serverDomain);
        if(tmpLen > 0)
          sim7600_cmd_buff = (char*)malloc(tmpLen + 35);
      }
      if(sim7600_cmd_buff == NULL) //can't allocate mem in heap
      {
        LOG_WRITE("sim7600_connectTask error1\n");
        osEventFlagsSet(ConfigSimEventID, 1 << simErrorEnum); //restart
        continue;
      }
      sprintf(sim7600_cmd_buff, "AT+CIPOPEN=0,\"TCP\",\"%s\",%d\r\n", serverDomain, serverPort);
      int tmpResTCP = sim7600_AT_notify_error(sim7600_cmd_buff, "+CIPOPEN: 0,0", "+CIPOPEN: 0", 10000, 2);
      free(sim7600_cmd_buff);
      if (tmpResTCP == 1) //successful open
      {
        //open tcp success
        sim7600ConnectStatus = 1;
        //init tcp udp stack to start TLS handshake
        TCP_UDP_Stack_Init(ConnectSimEventID, doneTLSEnum, ipCloseEnum, false);
        TCP_Request(); //request rsa pub key
      }
      else if(tmpResTCP == 2) //open fail, re-connect
      {
        osDelay(RECONNECT_INTERVAL);
        osEventFlagsSet(ConnectSimEventID, 1 << openConEnum);
      }
    }
    else if ((connectSimFlag & (1 << doneTLSEnum)) && sim7600ConnectStatus == 1)
    {
      LOG_WRITE("connectSimFlag doneTLS\n");
      osEventFlagsClear(ConnectSimEventID, 1 << doneTLSEnum);

      //open UDP connect
      int tmpResUDP = sim7600_AT_notify_error("AT+CIPOPEN=1,\"UDP\",,,8080\r\n", "+CIPOPEN: 1,0", "+CIPOPEN: 1", 10000, 2);
      if(tmpResUDP == 1) //successful
      {
        sim7600ConnectStatus = 2;
        //successful, reset connectToHostCount, change Domain2 <-> Domain1 if neccessary
      }
      else if(tmpResUDP == 2) //open fail
      {
        osEventFlagsSet(ConnectSimEventID, 1 << ipCloseEnum);
      }
    }
    // else if ((connectSimFlag & (1 << sendIpErrorEnum)) && sim7600ConnectStatus >= 1)
    // {
    //   osEventFlagsClear(ConnectSimEventID, 1 << sendIpErrorEnum);
//
    //   send_ip_error_count++;
    //   if(send_ip_error_count >= SEND_IP_ERROR_COUNT_MAX)
    //   {
    //     send_ip_error_count = 0;
    //     osEventFlagsSet(ConnectSimEventID, 1 << ipCloseEnum); //re-CONNECT
    //   }
    // }
    else
    { //something wrong
      LOG_WRITE("connectSimFlag sthElse\n");
      LOG_WRITE("sim7600_connectTask error2\n");
      osEventFlagsSet(ConfigSimEventID, 1 << simErrorEnum); //restart
    }
  }
}

//delete when receive message to avoid overflow memory
bool sim7600_delete_all_msg()
{
	return true;
}

void sim7600_sendTask()
{
  LOG_WRITE("sendTask\n");
  sendSimPack sendMsgObj;
  osStatus_t sendMsgStt;
  for (;;)
  {
    sendMsgStt = osMessageQueueGet(SendSimQueueID, &sendMsgObj, NULL, osWaitForever); //wait until has msg
    LOG_WRITE("sendMsgStt %d\n", sendMsgStt);
    if (sendMsgStt == osOK && bSim7600IsRunning)
    {
      LOG_WRITE("sendMsgType %d\n", sendMsgObj.type);
      //check send msg type
      if (sendMsgObj.type == 0)
      { //normal cmd
        int tmpRes = sim7600_send_cmd(sendMsgObj.ptr, sendMsgObj.res1, sendMsgObj.res2, sendMsgObj.timeout);
        if (tmpRes == 1 || tmpRes == 2)
        {
          if (sendMsgObj.EventID != NULL) //can't make hard error
            osEventFlagsSet(sendMsgObj.EventID, tmpRes);
        }
      }
      else
      { //send TCP packet
        sim7600_send_packet_ip(sendMsgObj.type, sendMsgObj.ptr, sendMsgObj.len);
      }
    }
  }
}

void sim7600_recvTask()
{
  LOG_WRITE("recvTask\n");
  for (;;)
  {
    osDelay(RECV_SIM_TASK_INTERVAL);
    if(bSim7600IsRunning)
      sim7600_usart_rx_check();
    // TCP_UDP_Notify(1);
  }
}

//global var
int old_pos_dma = 0;
int new_pos_dma = 0;

void sim7600_usart_rx_check()
{
  SIM7600_PAUSE_RX();
  
  /* Calculate current position in buffer */
  new_pos_dma = sim_dma_buff_size - (int)(LL_DMA_GetDataLength(DMA2, LL_DMA_STREAM_2) & 0xFFFF);
  if(new_pos_dma < 0)
  {
    LOG_WRITE("new_pos_dma < 0\n");
    //something wrong
    SIM7600_RESUME_RX();
    osEventFlagsSet(ConfigSimEventID, (1 << simErrorEnum)); //restart + reset
    return;
  }
  //0 1 2 3 4 5
  if (new_pos_dma != old_pos_dma)
  { /* Check change in received data */
    LOG_WRITE("new_pos_dma %d, oldPos %d\n", new_pos_dma, old_pos_dma);
    if (new_pos_dma > old_pos_dma)
    { /* Current position is over previous one */
      /* We are in "linear" mode */
      /* Process data directly by subtracting "pointers" */
      //usart_process_data(&usart_rx_dma_buffer[old_pos_dma], new_pos_dma - old_pos_dma);
      sim_buff_length = new_pos_dma - old_pos_dma;
      memcpy(sim_buff, sim_dma_buff + old_pos_dma, sim_buff_length);
    }
    else
    {
      /* We are in "overflow" mode */
      /* First process data to the end of buffer */
      sim_buff_length = sim_dma_buff_size - old_pos_dma; //0 1 2 3 4
      memcpy(sim_buff, sim_dma_buff + old_pos_dma, sim_buff_length);
      /* Check and continue with beginning of buffer */
      if (new_pos_dma > 0)
      {
        //usart_process_data(&usart_rx_dma_buffer[0], new_pos_dma);
        memcpy(sim_buff + sim_buff_length, sim_dma_buff, new_pos_dma);
        sim_buff_length += new_pos_dma;
      }
    }

    //debug /////////////////////////////////////
    uint8_t tmp = sim_buff[10]; sim_buff[10] = 0;
    LOG_WRITE("simbuff %s\n", sim_buff);
    sim_buff[10] = tmp;
    //////////////////////////////////////////////

    int returnTmp = sim7600_handle_received_data();
    SIM7600_RESUME_RX();
    LOG_WRITE("returnTmp %d\n", returnTmp);
    if (returnTmp < 0) //something wrong
    {
      LOG_WRITE("returnTmp < 0\n");
      //something wrong
      osEventFlagsSet(ConfigSimEventID, (1 << simErrorEnum)); //restart + reset
      return;
    }

    //debug //////////////////////////////////////
    savePosDma(old_pos_dma, new_pos_dma, returnTmp);
    //////////////////////////////////////////////

    old_pos_dma += returnTmp;
    old_pos_dma %= sim_dma_buff_size; // ~ if(old_pos_dma >= sim_dma_buff_size) old_pos_dma -= sim_dma_buff_size;
  }

  SIM7600_RESUME_RX();
}

//return 0 ~ success + continue, 1~ no success,2 ~ return sim_buff_index
//__STATIC_INLINE  ~ macro
int check_normal_response(uint8_t *posOfSubStr, const char *response, int *_sim_buff_index) //macro
{
  LOG_WRITE("chkRes\n");
  //simple because all data received is string
  *_sim_buff_index = posOfSubStr + strlen(response) - sim_buff; // + 2 for "\r\n"
  return 0;

  //check whether have \r\n at the end of this response
  posOfSubStr += strlen(response); //point to position right after the last character of response on sim_buff
  //in the worst case the last character of response is last character of sim_buff
  // ->  posOfSubStr point to sim_buff[sim_buff_length] = '\0' (initialize above) -> pointerTo_r_n == NULL
  if(posOfSubStr >= (sim_buff + sim_buff_length)) 
  {
    LOG_WRITE("chkRes pos >= (sim_+ sim_b)\n");
    return 2;
  }
  uint8_t *pointerTo_r_n = strstr(posOfSubStr, "\r");
  if (pointerTo_r_n != NULL)
  {
    //perfect sastified
    //change _sim_buff_index
    *_sim_buff_index = pointerTo_r_n - sim_buff + 2; // + 2 for "\r\n"
    return 0;
  }
  else if (posOfSubStr + MAX_DATA_LENGTH_OF_RESPONSE_R_N < sim_buff + sim_buff_length) //the worst case, have enough bytes but can't sastified
  {                                                                                    \
    //data may be error-bit
    LOG_WRITE("chkIp err\n");
    return 1;
  }
  else
    //return _sim_buff_index; don't have enough data
  {
    LOG_WRITE("chkIp 2\n");
    return 2;
  }
}

//return 0 ~ success + continue, 1~ no success (error),2 ~ return sim_buff_index (don't have enough data)
int check_normal_ip_packet(uint8_t *posOfSubStrSave, const char *response, int *_sim_buff_index, uint8_t **outputData, int *outputLen)
{
  LOG_WRITE("chkIpRes, %d, %x, resLen %d\n", *_sim_buff_index, posOfSubStrSave, strlen(response));
  //check whether have \r\n at buffer
  posOfSubStrSave += strlen(response);                      //point to length of IP packet (right after ",1," or ",0,")
  if(posOfSubStrSave >= (sim_buff + sim_buff_length))
  {
    LOG_WRITE("chkIpRes pos >= (sim_+ sim_b)\n");
    return 2;
  }
  uint8_t *pointerTo_r_n = strstr(posOfSubStrSave, "\r"); //point to '\r'
  //in the worst case ',' after  "+RECEIVE,1" is last character of sim_buff
  // ->  posOfSubStrSave point to sim_buff[sim_buff_length] = '\0' (initialize above) -> pointerTo_r_n == NULL
  if (pointerTo_r_n != NULL && (pointerTo_r_n - posOfSubStrSave) <= 4) //,0,1500\r\n
  {
    int lengthOfIPPacket = 0;
    while (posOfSubStrSave < pointerTo_r_n)
    { //make sure break when meet '\r'
      char numberTmp = (char)(*posOfSubStrSave);
      if (numberTmp >= '0' && numberTmp <= '9')
      {
        lengthOfIPPacket = lengthOfIPPacket * 10 + numberTmp - '0';
      }
      else
      { //data maybe bit-error -> restart
        LOG_WRITE("chkIpRes Err1\n");
        return 1;
      }
      posOfSubStrSave++;
    }
    posOfSubStrSave += 2;                                                //point to data
    if (posOfSubStrSave + lengthOfIPPacket > sim_buff + sim_buff_length) // don't have enough data
    {
      return 2;
    }
    //else data is sastified
    *outputData = posOfSubStrSave;
    *outputLen = lengthOfIPPacket;
    *_sim_buff_index = posOfSubStrSave - sim_buff + lengthOfIPPacket;
    LOG_WRITE("ipPacket len %d\n", lengthOfIPPacket);
    return 0;
  }
  else if (posOfSubStrSave + 6 < sim_buff + sim_buff_length) //the worst case: +RECEIVE,1,1500\r\n
  {
    //data may be error-bit
    LOG_WRITE("chkIpRes Err2\n");
    return 1;
  }
  else
    return 2; //dont have enough data
}

//list response
#define LIST_RESPONSE_SIZE 6
char listResponse[LIST_RESPONSE_SIZE][20] =
{
        //result check
        "RECEIVE,1,", //0 udp received
        "CIPEVENT",   //1 net error
        "+IPCLOSE",    //2 udp or tcp closed unexpectedly
        "RECEIVE,0,", //3 tcp received
        "RING",        //4 call
        "CMTI",       //5 sms
                       //.. cmd check
};

// "+CIPERROR",   //6 send IP packet error

 //handle received data, return num of bytes handled
int sim7600_handle_received_data()
{
  LOG_WRITE("simHdlRecv\n");
  //make sure sim_buff is string
  sim_buff[sim_buff_length] = '\0'; //can do this since real size of sim_buff = sim_buff_size + 1, so even sim_buff_length (max) = sim_buff_size, it is still oke
  uint8_t *posOfSubStr;
  uint8_t *posOfSubStrSave;
  int sim_buff_index = 0;
  LOG_WRITE("sim_buff_length %d\n", sim_buff_length);

  //check whether sim7600 is rebooted, this message can insert anytime, anywhere in simbuff
  //  so this case can have in sim_buff can have 0x00 before +CPIN: READY (this case happen when receive ip packet)
  // int tmp0;
  // for (tmp0 = 0; tmp0 < sim_buff_length; tmp0++)
  // {
  //   if (sim_buff[tmp0] == '+')
  //   {
  //     if ((strstr(sim_buff + tmp0, "+CPIN: READY\r\n") != NULL) &&
  //         (strstr(sim_buff + tmp0, "SMS DONE\r\n") != NULL) && (strstr(sim_buff + tmp0, "PB DONE\r\n") != NULL))
  //     {
  //       LOG_WRITE("sim is rebooted\n");
  //       osEventFlagsSet(ConfigSimEventID, 1 << rebootEnum);
  //       return sim_buff_length;
  //     }
  //   }
  // }

  //can do this, since all data is string
  // if ((strstr(sim_buff, "CPIN: READY") != NULL) &&
  //         (strstr(sim_buff, "SMS DONE") != NULL) && 
  if(strstr(sim_buff, "PB DONE") != NULL)
  {
    LOG_WRITE("sim is rebooted\n");
    osEventFlagsSet(ConfigSimEventID, 1 << rebootEnum);
    return sim_buff_length;
  }

  while (true)
  {
    LOG_WRITE("sim_buff_index %d\n", sim_buff_index);
    posOfSubStrSave = NULL;
    if (sim_buff_index == sim_buff_length)
      return sim_buff_length; //reach end of buff
    else if(sim_buff_index > sim_buff_length) //out of index
    {
      //something wrong
      LOG_WRITE("return sim_buff_index error\n");
      return sim_buff_length;
    }

    int resultCheck = -1;
    //first check cmd response
    const char *checkCmdResPtr;
    if (cmdSendStatus == 1)
    {
      if (res1 != NULL)
      {
        posOfSubStr = strstr(sim_buff + sim_buff_index, res1);
        if (posOfSubStr != NULL)
        {
          posOfSubStrSave = posOfSubStr;
          checkCmdResPtr = res1;
          resultCheck = LIST_RESPONSE_SIZE;
        }
      }
      if (res2 != NULL)
      {
        posOfSubStr = strstr(sim_buff + sim_buff_index, res2);
        if (posOfSubStr != NULL && (posOfSubStr < posOfSubStrSave || posOfSubStrSave == NULL))
        {
          posOfSubStrSave = posOfSubStr;
          checkCmdResPtr = res2;
          resultCheck = LIST_RESPONSE_SIZE;
        }
      }
    }
    else if (cmdSendStatus == 2)
    {
      posOfSubStr = strstr(sim_buff + sim_buff_index, ">");
      if (posOfSubStr != NULL)
      {
        posOfSubStrSave = posOfSubStr;
        resultCheck = LIST_RESPONSE_SIZE;
      }
    }

    //check in list
    for (int i = 0; i < LIST_RESPONSE_SIZE; i++)
    {
      posOfSubStr = strstr(sim_buff + sim_buff_index, listResponse[i]);
      if (posOfSubStr != NULL && (posOfSubStr < posOfSubStrSave || posOfSubStrSave == NULL))
      {
        posOfSubStrSave = posOfSubStr;
        resultCheck = i;
      }
    }

    LOG_WRITE("resCheck %d, cmdStt %d\n", resultCheck, cmdSendStatus);
    int resultTmp = -1;
    if (resultCheck < 0) // nothing can find
    {
      if((sim_buff_length - sim_buff_index) > 100)
      {
        resultTmp = -1;
      }
    }                          

    else if (resultCheck == 0) //udp received
    {
      uint8_t *udpData;
      int udpDataLen;
      resultTmp = check_normal_ip_packet(posOfSubStrSave, listResponse[resultCheck], &sim_buff_index, &udpData, &udpDataLen);
      if (resultTmp == 0)
      {
        UDP_Packet_Analyze(udpData, udpDataLen);
        continue;
      }
    }
    else if (resultCheck == 1) //1 net error
    {
      resultTmp = check_normal_response(posOfSubStrSave, listResponse[resultCheck], &sim_buff_index);
      if (resultTmp == 0) // success find
      {
        //sim7600 error, handle in a task sim7600 config task
        osEventFlagsSet(ConnectSimEventID, 1 << netErrorEnum);
        continue;
      }
    }
    else if (resultCheck == 2) //2 udp/tcp closed , reconnect
    {
      resultTmp = check_normal_response(posOfSubStrSave, listResponse[resultCheck], &sim_buff_index);
      if (resultTmp == 0) // success find
      {
        osEventFlagsSet(ConnectSimEventID, 1 << ipCloseEnum); //notify connectTask that need to reconnect
        continue;
      }
    }
    else if (resultCheck == 3) //3 received TCP packet
    {
      uint8_t *tcpData;
      int tcpDataLen;
      resultTmp = check_normal_ip_packet(posOfSubStrSave, listResponse[resultCheck], &sim_buff_index, &tcpData, &tcpDataLen);
      if (resultTmp == 0)
      {
        TCP_Packet_Analyze(tcpData, tcpDataLen);
        continue;
      }
    }
    else if (resultCheck == 4) //4 call
    {
      resultTmp = check_normal_response(posOfSubStrSave, listResponse[resultCheck], &sim_buff_index);
      if (resultTmp == 0) // success find
      {
        osEventFlagsSet(ConfigSimEventID, 1 << callingEnum); //notify configTask to hang-up
        continue;
      }
    }
    else if (resultCheck == 5) //sms
    {
      resultTmp = check_normal_response(posOfSubStrSave, listResponse[resultCheck], &sim_buff_index);
      if (resultTmp == 0) // success find
      {
        osEventFlagsSet(ConfigSimEventID, 1 << smsEnum); //notify configTask to hang up
        continue;
      }
    }
    else //cmd check
    {
      LOG_WRITE("cmdSendStt %d\n", cmdSendStatus);
      if (cmdSendStatus == 2)
      {                                      //just need '>'
        osEventFlagsSet(SendSimEventID, 1U); //notify to send task
        // osThreadYield();
        //change sim_buff_index
        sim_buff_index = posOfSubStrSave - sim_buff + 1; // + 1 for ">"
        continue;
      }
      else if (cmdSendStatus == 1)
      {
        //check whether have \r\n at the end of this response
        resultTmp = check_normal_response(posOfSubStrSave, checkCmdResPtr, &sim_buff_index);
        if (resultTmp == 0)
        {
          //notify to send task
          if (checkCmdResPtr == res1)
          {
            osEventFlagsSet(SendSimEventID, 1U);
          }
          else
          {
            osEventFlagsSet(SendSimEventID, 2U);
          }
          continue;
        }
      }
    }

    LOG_WRITE("resultTmp %d\n", resultTmp);
    //data or net error, need to start
    if (resultTmp == 1)
    {
      //error need to start
      osEventFlagsSet(ConfigSimEventID, 1 << simErrorEnum); //notify configTask to restart and re config
    }
    //dont have enough data
    //else if(resultTmp == 2) return sim_buff_index;
    return sim_buff_index;
  }

  //discard garbage \r\n .. \r\n
  // while (true)
  // {
  //   if (sim_buff_index >= sim_buff_length)
  //     return sim_buff_length;

  //   posOfSubStr = strstr(sim_buff + sim_buff_index, "\r\n"); //first \r\n
  //   if (posOfSubStr != NULL)
  //   {
  //     posOfSubStr = strstr(posOfSubStr + 2, "\r\n"); //second \r\n
  //     if (posOfSubStr != NULL)                       //detect \r\n...\r\n
  //     {
  //       LOG_WRITE("collect_rn\n");
  //       sim_buff_index = posOfSubStr + 2 - sim_buff;
  //       continue;
  //     }
  //   }
  //   break; //if can't find \r\n .. \r\n sastified
  // }
  // return sim_buff_index;
}

// uint16_t sim7600_check_sum_data(uint8_t *ptr, int length)
// {
// 	uint32_t checksum = 0;
// 	while (length > 1) //cong het cac byte16 lai
// 	{
// 		checksum += ((uint32_t)*ptr << 8) | (uint32_t) *(ptr + 1);
// 		ptr += 2;
// 		length -= 2;
// 	}
// 	if (length)
// 	{
// 		checksum += ((uint32_t)*ptr) << 8; //neu con le 1 byte
// 	}
// 	while (checksum >> 16)
// 	{
// 		checksum = (checksum & 0xFFFF) + (checksum >> 16);
// 	}
// 	//nghich dao bit
// 	checksum = ~checksum;
// 	//hoan vi byte thap byte cao
// 	return (uint16_t)checksum;
// }

// bool CheckSumCMD(uint8_t* tcpPacket, int length)
// {
//   uint16_t* checksump = (uint16_t*)(tcpPacket + length - 2);
//   uint16_t checksum = *checksump;
//   if(checksum == sim7600_check_sum_data(tcpPacket, length - 2)) return true;
//   return false;
// }

void sim7600_change_baud(uint32_t baudrate)
{
  LOG_WRITE("changeBaud %lu\n", baudrate);
  LL_USART_Disable(USART1);
  sim7600SetBaudrate(baudrate);
  LL_USART_Enable(USART1);
}

//restart/reset module
//1st use pwr pin
//2nd use rst pin

void sim7600_reset_all_state()
{
  connectToHostCount = 0;

  sim7600ConnectStatus = 0;

  //clear event
  osEventFlagsClear(ConfigSimEventID, 0xFF);
  osEventFlagsClear(ConnectSimEventID, 0xFF);

  //clear message queue
  osMessageQueueReset(SendSimQueueID);

  cmdSendStatus = 0;
  osEventFlagsClear(SendSimEventID, 0xFF);

  old_pos_dma = 0;
  new_pos_dma = 0;
}

void sim7600_restart()
{
  LOG_WRITE("restart\n");
  //suspend all task ??? necessary
  // while (true) //debug
  // {
  //   /* code */
  // }
  

  LL_USART_Disable(USART1);
  while (LL_USART_IsEnabled(USART1)); //waite until En bit == 0
  
  //disable DMA
  LL_DMA_DisableStream(DMA2, LL_DMA_STREAM_2);
  while (LL_DMA_IsEnabledStream(DMA2, LL_DMA_STREAM_2))
    ; //wait until En bit == 0

  //reset global var
  sim7600_reset_all_state();

  if (restartSimstatus < MAX_NUM_RESTART_SIM7600) //0-19
  {
    //use pwr pin to power off
    sim7600_powerOFF();
    restartSimstatus++;
  }
  else if (restartSimstatus < (MAX_NUM_RESTART_SIM7600 + MAX_NUM_RESET_SIM7600)) // 20-21
  {
    sim7600_reset();
    sim7600_powerOFF();
    restartSimstatus++;
  }
  else // >=22
  {
    //delay to wait
    osDelay(SLEEP_MINUTES_SIM7600 * 60 * 1000); // SLEEP_MINUTES_SIM7600 minutes
    restartSimstatus = 0;
  }

  //reset global var
  sim7600_reset_all_state();

  LL_DMA_EnableStream(DMA2, LL_DMA_STREAM_2);
  while (!LL_DMA_IsEnabledStream(DMA2, LL_DMA_STREAM_2))
    ; //wait until En bit == 1

  //change baud rate to default
  sim7600_change_baud(SIM7600_BAUDRATE_DEFAULT); // in this function usart is enable again
  LL_USART_Enable(USART1);

  //power on again
  sim7600_powerON();

  //resume all task
}
