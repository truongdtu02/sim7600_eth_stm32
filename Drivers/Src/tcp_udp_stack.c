#include "tcp_udp_stack.h"

#define TIM_NTP TIM2

enum SendTLSPackeTypeEnum { statusPacketEnum, mp3PacketEnum };

struct
{
  int Success, Send_AES_ID, Error; //RSA ~ pubkey, ACK ~ successful, Error: need to re-connect
} TCP_UDP_Flag;

//global var
int TCPConnectStatus = 0; // 0:begineCon, 1: recvedRSA, 2: recvedACK ~ successful
bool IsTLSHanshaked = false;
bool ISError = false;

uint8_t TcpBuff[TCP_BUFF_LEN];
int TcpBuffOffset = 0;
bool bIsPending = false;
int remainData = 0; //reamin data need to collect

uint8_t *salt;
int saltLen;

uint8_t RSA_Packet[RSA_PACKET_LEN];
uint8_t Keepalive_Packet[] = {1, 0, 0};
const int Keepalive_Packet_LEN = 3;
uint8_t Status_Packet[] = {4, 0, 0, 0, 0, 0};
const int Status_Packet_LEN = 6;
uint8_t NTP_Packet[32];
const int NTP_PACKET_LEN = 32;

#define AUDIO_PACKET_LEN 1500
uint8_t Audio_Packet[AUDIO_PACKET_LEN] = {1};
extern char *DeviceID;

const char *ntpAESkey = "dayLaAESKeyNtp!!";
const int ntpAESkeyLen = 16;
int tcpTimerCount = 1, udpTimerCount = 0; //tcpTimerCount need start from 1 since 0 % n == 0
//but need to update udp quickly, so udpTimerCount = 0;

extern osTimerId_t TCPTimerOnceID;

enum
{
  connectTypeETH,
  connectTypeSim7600
} connectType;

osEventFlagsId_t TCP_UDP_StackEventID;
//notify to ethernet or sim7600 stack
void TCP_UDP_Notify(int flagsEnum)
{
  LOG_WRITE("tcpUdpNotify %d\n", flagsEnum);
  osEventFlagsSet(TCP_UDP_StackEventID, 1 << flagsEnum);
}

//payload : 1-tcp, 2-udp
bool TCP_UDP_Send(int type, uint8_t *data, int len)
{
  LOG_WRITE("tcpUdpSend %d\n", type);
  if (connectType == connectTypeSim7600)
  {
    return sim7600_IP(type, data, len);
  }
  else
  {
    return ethSendIP(type, data, len);
  }
}

MP3Struct *mp3Packet;
bool AudioPacketHandle(uint8_t *data, int len)
{
  mp3Packet = (MP3Struct*)data;

  //decrypt 16B next with old aes key to get new key
  if(AES_Decrypt_Packet(data + 16, 16) > 0)
  {
    //decrypt with mp3 key, from volume
    int lenPacketDecrypt = len - MP3_PACKET_HEADER_LEN_BEFORE_VOLUME;
    lenPacketDecrypt -=  (lenPacketDecrypt % 16); //make sure it is multiple of 16 aes block len
    if(AES_Decrypt_Packet_Key(&(mp3Packet->volume), lenPacketDecrypt, mp3Packet->aesMP3key) > 0)
    {
      uint8_t vol = mp3Packet->volume;
      if(vol == 100)
        return true;
    }
  }
  //wrong mp3 packet
  LOG_WRITE("mp3 packet error\n");
}

int realPacketLen; //len payload , not including md5
int totalTCPBytes = 0;
void TCP_Packet_Handle()
{
  LOG_WRITE("tcpPacketHdl\n");
  ISError = true;
  PacketTCPStruct *packetTcpHeader = (PacketTCPStruct *)TcpBuff;
  totalTCPBytes += packetTcpHeader->len;
  realPacketLen = packetTcpHeader->len - SIZE_OF_MD5;
  if (!IsTLSHanshaked)
  {
    if (TCPConnectStatus == 0 && realPacketLen == SIZE_OF_PUBKEY)
    {
      //check MD5
      if (CheckMD5(packetTcpHeader))
      {
        PacketTCPStruct *rsaPacketHeader = (PacketTCPStruct *)RSA_Packet;

        //copy salt first
        srand(time(0));
        int j;
        for (j = 0; j < saltLen; j++)
        {
          rsaPacketHeader->payload[j] = (uint8_t)rand();
          salt[j] = rsaPacketHeader->payload[j] & 0x7F;
        }
        //copy next ID
        memcpy(rsaPacketHeader->payload + saltLen, DeviceID, saltLen);
        //convert text with salt
        ConvertTextWithSalt(rsaPacketHeader->payload, saltLen, saltLen, Add);

        //then AES key
        AES_Generate_Rand_Key();
        memcpy(rsaPacketHeader->payload + saltLen + saltLen, AES_Get_Key(), AES128_KEY_LEN);

        //caculate md5
        uint8_t *md5sum = md5hash(rsaPacketHeader->payload, saltLen + saltLen + AES128_KEY_LEN);
        memcpy(rsaPacketHeader->md5, md5sum, MD5_LEN);

        //next encrypt from md5
        if (RSA2048_Pubkey_Encrypt(packetTcpHeader->payload, RSA_MAX_MODULUS_LEN, rsaPacketHeader->md5, SIZE_OF_MD5 + saltLen + saltLen + AES128_KEY_LEN, rsaPacketHeader->md5) > 0)
        {
          rsaPacketHeader->len = RSA_BLOCK_LEN;
          TCPConnectStatus = 1;
          TCP_UDP_Send(1, RSA_Packet, RSA_PACKET_LEN);
          ISError = false;
        }
      }
    }
    else if (TCPConnectStatus == 1 && realPacketLen >= AES128_BLOCK_LEN && (realPacketLen % AES128_BLOCK_LEN) == 0)
    {
      if (CheckMD5(packetTcpHeader))
      {
        if (AES_Decrypt_Packet(packetTcpHeader->payload, realPacketLen) == realPacketLen)
        {
          //check salt ACK
          if (CheckSaltACK(packetTcpHeader->payload, realPacketLen))
          {
            TCP_UDP_Notify(TCP_UDP_Flag.Success); //notify connect task success, then to open UDP connect
            TCPConnectStatus = 2;
            IsTLSHanshaked = true;
            //successful, delete timer osTimerStop, reset connectToHostCount, change Domain2 <-> Domain1 if neccessary
            osTimerStart(TCPTimerOnceID, TIMER_INTERVAL);
            ISError = false;
          }
        }
      }
    }
  }
  else //handle TLS packet
  {
    ISError = false;

    //debug
    int *order = (int*)(TcpBuff + 4);
    int llen = packetTcpHeader->len + 4; // + 4B len
    LOG_WRITE("mp3 len:%d\n", llen - 4);

    // if(packetTcpHeader->len < 1000)
    // {
    //   LOG_WRITE("error tcp packet len:%d\n", packetTcpHeader->len);
    // }

    if(CheckMD5(packetTcpHeader))
    {
      //check payload has at least 16B (1 AES block)
      if(realPacketLen >= AES128_BLOCK_LEN)
      {
        //decrype first 16B to check type of packet
        if(AES_Decrypt_Packet(packetTcpHeader->payload, AES128_BLOCK_LEN) > 0)
        {
          uint8_t typeOfPacket = packetTcpHeader->payload[0];
          if(typeOfPacket == mp3PacketEnum && realPacketLen > MP3_PACKET_HEADER_LEN)
          {
        	  AudioPacketHandle(packetTcpHeader->payload, realPacketLen);
          }
          else if (typeOfPacket == statusPacketEnum)
          {
            
          }
        }
      }
    }
  }
  
  if (ISError)
  {
    LOG_WRITE("TCP_Packet_Handle recv error\n");
    TCP_UDP_Notify(TCP_UDP_Flag.Error);
    ISError = false;
  }
}

int old_packet_len;
// void TCP_Packet_Analyze(uint8_t *tcpPacket, int length)
// {
//   LOG_WRITE("tcpPacketAnalyze\n");
//   PacketTCPStruct *packetTcpHeader = (PacketTCPStruct *)TcpBuff;
//   while (length > 0)
//   {
//     //beggin get length of packet
//     if (!bIsPending)
//     {
//       //barely occur, not enough data tp detect lenght of TCP packet
//       if ((TcpBuffOffset + length) < SIZE_OF_LEN)
//       {
//         memcpy(TcpBuff + TcpBuffOffset, tcpPacket, length);
//         length = 0;
//         TcpBuffOffset += length;
//         LOG_WRITE("(TcpBuffOffset + length) < SIZE_OF_LEN\n");
//       }
//       //else enough data to detect
//       else
//       {
//         //copy just enough
//         int tmpOffset = SIZE_OF_LEN - TcpBuffOffset;
//         memcpy(TcpBuff + TcpBuffOffset, tcpPacket, tmpOffset);
//         TcpBuffOffset = SIZE_OF_LEN;
//         length -= tmpOffset;
//         tcpPacket += tmpOffset;
//         bIsPending = true;
//
//         remainData = packetTcpHeader->len;
//         LOG_WRITE("tcpPacketLen %d\n", remainData);
//         //data is so big, sth wrong
//         if (remainData > TCP_BUFF_LEN || remainData < 0)
//         {
//           LOG_WRITE("TCP packet len error\n");
//           TCP_UDP_Notify(TCP_UDP_Flag.Error);
//           return;
//         }
//         old_packet_len = remainData;
//       }
//     }
//     //got length, continue collect data
//     else
//     {
//       //save to buff
//       if (length < remainData) //not enough data to get
//       {
//         memcpy(TcpBuff + TcpBuffOffset, tcpPacket, length);
//         TcpBuffOffset += length;
//         remainData -= length;
//         length = 0;
//       }
//       else
//       {
//         //done packet
//         memcpy(TcpBuff + TcpBuffOffset, tcpPacket, remainData);
//         length -= remainData;
//         tcpPacket += remainData;
//         TcpBuffOffset = 0; //reset
//         bIsPending = false;
//         TCP_Packet_Handle();
//       }
//     }
//   }
// }

//convert hex string to byte array, auto override. return len of byte array if success, else return -1
uint8_t hexStringMap[255];
bool bHexStringMapFrist = true;
int HexStringToByteArray()
{
  //initialize
  if(bHexStringMapFrist)
  {
    bHexStringMapFrist = false;
    for(int i = 0; i < 255; i++)
    {
      if(i >= '0' && i <= '9')
      {
        hexStringMap[i] = i - '0';
      }
      else if(i >= 'A' && i <= 'F')
      {
        hexStringMap[i] = i - 'A' + 10;
      }
      else if(i >= 'a' && i <= 'f')
      {
        hexStringMap[i] = i - 'a' + 10;
      }
      else
      {
        hexStringMap[i] = 0;
      }
    }
  }

  int i, j; //i for hex string, j for byte array
  for(i = 0, j = 0; i < TcpBuffOffset; i+=2, j++)
  {
    uint8_t tmpByte = (hexStringMap[TcpBuff[i]] << 4) | hexStringMap[TcpBuff[i+1]];
    TcpBuff[j] = tmpByte;
  }
  return j;
}

//packet string hex
void TCP_Packet_Analyze(uint8_t *recvData, int length)
{
  int upper = length;
  int offset = 0;
  while (length > 0)
  {
    int eofPackIndx = -1;

    for (int i = offset; i < upper; i++)
    {
      if (recvData[i] == '#')
      {
        eofPackIndx = i;
        break;
      }
    }
    if (eofPackIndx == -1) //not find "#"
    {
      if (TcpBuffOffset + length < TCP_BUFF_LEN)
      {
        // System.Buffer.BlockCopy(recvData, offset, Tcpbuff, TcpbuffOffset, length);
        memcpy(TcpBuff + TcpBuffOffset, recvData + offset, length);
        TcpBuffOffset += length;
        length = 0;
      }
    }
    else
    {
      int lenTmp = eofPackIndx - offset; // 0 1 2 3 4
      if (lenTmp > 0 && TcpBuffOffset + lenTmp < TCP_BUFF_LEN)
      {
        // System.Buffer.BlockCopy(recvData, offset, Tcpbuff, TcpbuffOffset, lenTmp);
        memcpy(TcpBuff + TcpBuffOffset, recvData + offset, lenTmp);
        TcpBuffOffset += lenTmp;
      }
      offset = eofPackIndx + 1; //+1 for "#"
      length -= (lenTmp + 1);     // +1 for "#"

      //handle tcp packet
      if (TcpBuffOffset % 2 == 0) //byte to hex string -> double length of packet
      {
        //TcpbuffOffset ~ length of Tcp packet
        //convert hex string to byte array
        TcpBuffOffset = HexStringToByteArray();

        //check length field
        int lenOfPacket = (int)(*(uint16_t *)TcpBuff);
        if (lenOfPacket == TcpBuffOffset - 2) //2 byte of length field
        {
//          curPacketSize = lenOfPacket;s
          TCP_Packet_Handle();
        }
      }
      TcpBuffOffset = 0;
    }
  }
}

uint32_t rtt;
int64_t ntpTime = 0, ntpStart; //ntpstart = timer of stm32 when receive ntpTime
void UDP_Packet_Analyze(uint8_t *data, int len)
{
  LOG_WRITE("udpPacketAnalyze\n");
  //ntp packet
  if (len == 12)
  {
    NTPStruct2 *ntpPack = (NTPStruct2 *)data;
    rtt = (TIM_NTP->CNT - ntpPack->clientTime) >> 1; // >> 1 ~ / 2 (since TIM_NTP tick 0.5ms)

    //the first time, get as soon as posible
    if (ntpTime == 0)
    {
      ntpTime = ntpPack->serverTime + rtt / 2;
      ntpStart = TIM_NTP->CNT >> 1; // /2 since TIMER tick 0.5ms
    }

    //get until enough percious
    if (rtt < RTT_NTP_MAX)
    {
      udpTimerCount++;
      ntpTime = ntpPack->serverTime + rtt / 2;
      ntpStart = TIM_NTP->CNT >> 1; // >> 1 ~ /2 since TIMER tick 0.5ms
    }
  }
}

//get UTC NTP time
int64_t TCP_UDP_GetNtpTime()
{
  return ntpTime + (TIM_NTP->CNT >> 1) - ntpStart; //  >> 1 ~ /2 since TIMER tick 0.5ms
}

// void UDP_Packet_Analyze(uint8_t *data, int len)
// {
//   LOG_WRITE("udpPacketAnalyze\n");
//   //ntp packet
//   if (len == 12)
//   {
//     NTPStruct2 *ntpPack = (NTPStruct2 *)data;
//     rtt = (TIM_NTP->CNT - ntpPack->clientTime) >> 1; // >> 1 ~ / 2 (since TIM_NTP ~ TIM5 tick 0.5ms)
//     serverTime = ntpPack->serverTime;
//     if (rtt < RTT_NTP_MAX)
//     {
//       udpTimerCount++;
//       //test;
//       if (serverTimeSave == 0)
//       {
//         serverTimeSave = serverTime + rtt / 2;
//         time_ntp_cnt_save = TIM_NTP->CNT;
//       }
//       else
//       {
//         //check different time_client - time_server
//         diffTime = serverTimeSave + ((TIM_NTP->CNT - time_ntp_cnt_save) >> 1) - (serverTime + rtt / 2);
//       }
//     }
//   }
//   else if (len == 32)
//   {
//     NTPStruct *ntpPacket = (NTPStruct *)data;
//
//     //decrypt md5 first
//     AES_Decrypt_Packet_Key(data, MD5_LEN, (uint8_t *)ntpAESkey);
//     //caculate md5 and check
//     uint8_t *md5Sum = md5hash(data + MD5_LEN, 16);
//     int i;
//     for (i = 0; i < MD5_LEN; i++)
//     {
//       // if(*(int*)(data + i) != *(int*)(md5Sum + i)) return;
//       if (md5Sum[i] != data[i])
//         return;
//     }
//
//     //decrypt data
//     AES_Decrypt_Packet_Key(data + MD5_LEN, 16, (uint8_t *)ntpAESkey);
//
//     rtt = (TIM_NTP->CNT - ntpPacket->clientTime) >> 1; // >> 1 ~ / 2 (since TIM_NTP ~ TIM5 tick 0.5ms)
//     serverTime = ntpPacket->serverTime;
//     if (rtt < RTT_NTP_MAX)
//     {
//       udpTimerCount++;
//       //test;
//       if (serverTimeSave == 0)
//       {
//         serverTimeSave = serverTime + rtt / 2;
//         time_ntp_cnt_save = TIM_NTP->CNT;
//       }
//       else
//       {
//         //check different time_client - time_server
//         diffTime = serverTimeSave + ((TIM_NTP->CNT - time_ntp_cnt_save) >> 1) - (serverTime + rtt / 2);
//       }
//     }
//   }
// }

//init after TCP three-way handshake, but before RSA-AES handshake
void TCP_UDP_Stack_Init(osEventFlagsId_t eventID, int successFlag, int errorFlag, bool IsETH)
{
  LOG_WRITE("tcpUdpInit\n");
  //only one time after reboot
  if (salt == NULL)
  {
    saltLen = strlen(DeviceID);
    if (saltLen > DEVICE_LEN_MAX)
      saltLen = DEVICE_LEN_MAX;
    salt = (uint8_t *)malloc(saltLen);
  }

  TCPConnectStatus = 0;
  IsTLSHanshaked = false;
  ISError = false;

  TcpBuffOffset = 0;
  bIsPending = false;
  remainData = 0;

  udpTimerCount = 0;
  tcpTimerCount = 1;
  ntpTime = 0;

  if (IsETH)
  {
    connectType = connectTypeETH;
  }
  else
  {
    connectType = connectTypeSim7600;
  }

  TCP_UDP_StackEventID = eventID;
  TCP_UDP_Flag.Success = successFlag; //successful TLS handshake
  TCP_UDP_Flag.Error = errorFlag;

  //send request rsa-pubkey
  //  TCP_UDP_Send(1, Keepalive_Packet, Keepalive_Packet_LEN);

  //init timer time-out tcp connect (max time before recv "ACK"))
  osTimerStart(TCPTimerOnceID, TLS_HANDSHAKE_TIMEOUT);
}

//stop timer before re-connect
void TCP_UDP_Stack_Release()
{
  LOG_WRITE("tcpUdpRelease\n");
  osTimerStop(TCPTimerOnceID);
  IsTLSHanshaked = false;
}

// osTimerStop(TCPTimerOnceID);
//time-out -> recconnect TCP/UDP
void TCP_Timer_Callback(void *argument)
{
  LOG_WRITE("tcpTimerCallb\n");
  (void)argument;
  if (!IsTLSHanshaked)
  {
    TCP_UDP_Notify(TCP_UDP_Flag.Error);
    return;
  }

  if (tcpTimerCount % SEND_STAUS_INTERVAL == 0)
  {
    int diffStatus = 0;
    memcpy(Status_Packet + 2, (uint8_t *)(&diffStatus), 4);
    if (TCP_UDP_Send(1, Status_Packet, Status_Packet_LEN))
    {
      tcpTimerCount++;
    }
  }
  else if (tcpTimerCount % KEEP_ALIVE_INTERVAL == 0)
  {
    if (TCP_UDP_Send(1, Keepalive_Packet, Keepalive_Packet_LEN))
    {
      tcpTimerCount++;
    }
  }
  else
    tcpTimerCount++;

  if (udpTimerCount % NTP_INTERVAL == 0)
  {
    //random number
    // srand(time(0));

    // NTPStruct *ntpPacket = (NTPStruct*)NTP_Packet;
    // ntpPacket->clientTime = TIM_NTP->CNT;
    // ntpPacket->serverTime = ((long)rand() << 4) | rand();
    // ntpPacket->tmp = rand();

    // //encrypt
    // AES_Encrypt_Packet_Key(NTP_Packet + 16, 16, (uint8_t*)ntpAESkey);
    // //md5 check sum
    // uint8_t *md5Sum = md5hash(NTP_Packet + 16, 16);
    // //copy md5 sum to ntp packet
    // memcpy(NTP_Packet, md5Sum, MD5_LEN);
    // //encrypt md5 sum
    // AES_Encrypt_Packet_Key(NTP_Packet, 16, (uint8_t*)ntpAESkey);

    // TCP_UDP_Send(2, NTP_Packet, NTP_PACKET_LEN);
    uint32_t curT = TIM_NTP->CNT;
    memcpy(NTP_Packet, (uint8_t *)(&curT), 4);
    TCP_UDP_Send(2, NTP_Packet, 4);
  }
  else
  {
    udpTimerCount++;
  }
  osTimerStart(TCPTimerOnceID, TIMER_INTERVAL);
}

//request rsa pub key
void TCP_Request()
{
  TCP_UDP_Send(1, Keepalive_Packet, Keepalive_Packet_LEN);
}

bool Is_TCP_LTSHanshake_Connected() { return IsTLSHanshaked; }

//with pubkey packet (first packet), not necessary to decrypt md5
bool CheckMD5(PacketTCPStruct *packet)
{
  LOG_WRITE("chkMD5\n");
  if (TCPConnectStatus != 0) // >=1
  {
    int res = AES_Decrypt_Packet(packet->md5, SIZE_OF_MD5);
    if (res == -1)
      return false;
  }
  uint8_t *md5Sum = md5hash(packet->payload, realPacketLen);
  int i;
  for (i = 0; i < SIZE_OF_MD5; i++)
  {
    if (packet->md5[i] != md5Sum[i])
      return false;
  }
  return true;
}

void ConvertTextWithSalt(uint8_t *data, int offset, int len, enum SaltEnum saltType)
{
  LOG_WRITE("convertTextSalt\n");
  if (salt == NULL)
    return; //something wrong ???

  int i = 0, j = 0;
  if (saltType == Add)
  {
    while (j < len)
    {
      data[j + offset] += salt[i];
      j++;
      i++;
      if (i == saltLen)
        i = 0;
    }
  }
  else //sub
  {
    while (j < len)
    {
      data[j + offset] -= salt[i];
      j++;
      i++;
      if (i == saltLen)
        i = 0;
    }
  }
}

//check data is the same with salt
bool CheckSaltACK(uint8_t *data, int len)
{
  LOG_WRITE("chkSaltACK\n");
  if (len >= saltLen)
  {
    int i;
    for (i = 0; i < saltLen; i++)
    {
      if ((data[i] & 0x7F) != salt[i])
        return false;
    }
    return true;
  }
  return false;
}
