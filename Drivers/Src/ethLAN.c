#include "ethLAN.h"

extern osEventFlagsId_t ConnectEthEventID;
extern osMessageQueueId_t SendEthQueueID;

//extern struct netif gnetif;

int ethConnectStatus = 0; //0~none, 1~done TCP, begin TLS, 2~done TLS

struct netconn *conn; //tcp
struct udp_pcb *upcb; //udp

extern char serverDomain[];
extern int serverPort;

extern struct netif gnetif;

//udp
void udp_receive_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    LOG_WRITE("recv udp\n");
    UDP_Packet_Analyze(p->payload, p->len);

    /* Free receive pbuf */
    if (pbuf_free(p) != ERR_OK)
    {
        LOG_WRITE("recv udp pcb error\n");
        osEventFlagsSet(ConnectEthEventID, 1 << errorEthEnum);
    }
}

void udp_client_connect(ip_addr_t serverIpAddr, uint16_t sererPort)
{
    static bool bUdpFirstTime = true;
    err_t err;
    bool bUdpConnectErr = true;
    LOG_WRITE("udp connect %d %d\n", bUdpFirstTime, upcb);
    if (bUdpFirstTime) /* First time - Create a new UDP control block  */
    {
        upcb = udp_new();
        bUdpFirstTime = false;
    }
    else 
    {
        if (upcb != NULL)
            udp_disconnect(upcb);
    }

    if (upcb != NULL)
    {
        /* configure destination IP address and port */
        err = udp_connect(upcb, &serverIpAddr, sererPort);

        if (err == ERR_OK)
        {
            /* Set a receive callback for the upcb */
            udp_recv(upcb, udp_receive_callback, NULL);

            bUdpConnectErr = false;
        }
    }

    if (bUdpConnectErr)
    {
        LOG_WRITE("create new udp pcb error\n");
        osEventFlagsSet(ConnectEthEventID, 1 << errorEthEnum);
    }
}

void udp_client_send(uint8_t *data, int len)
{
    LOG_WRITE("udp send %d %p\n", ethConnectStatus, upcb);

    if (ethConnectStatus != 2 || upcb == NULL)
        return;

    bool bUdpSendErr = true;
    struct pbuf *p;

    /* allocate pbuf from pool*/
    p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_POOL);

    if (p != NULL)
    {
        /* copy data to pbuf */
        if (pbuf_take(p, data, len) == ERR_OK)
        {
            /* send udp data */
            if (udp_send(upcb, p) == ERR_OK)
            {
                bUdpSendErr = false;
            }
        }
        /* free pbuf */
        if (pbuf_free(p) != ERR_OK)
        {
            bUdpSendErr = true;
        }
    }

    if (bUdpSendErr)
    {
        LOG_WRITE("send udp pcb error\n");
        osEventFlagsSet(ConnectEthEventID, 1 << errorEthEnum);
    }
}

//tcp
void ethSendTask()
{
    sendEthPack sendMsgObj;
    osStatus_t sendStt;
    err_t err;
    //wait until have packet
    for (;;)
    {
        sendStt = osMessageQueueGet(SendEthQueueID, &sendMsgObj, NULL, osWaitForever);
        LOG_WRITE("eth send task %d %d\n", ethConnectStatus, sendMsgObj.type);
        if (ethConnectStatus == 0 || sendStt != osOK)
            continue;
        if (sendMsgObj.type == 1)
        {
            int nWritten = 0;
            size_t len;
            while (nWritten < sendMsgObj.len)
            {
                err = netconn_write_partly(
                    conn,                                      //connection
                    (const void *)(sendMsgObj.ptr + nWritten), //buffer pointer
                    (sendMsgObj.len - nWritten),               //buffer length
                    NETCONN_DONTBLOCK,                         //no copy
                    (size_t *)&len);                           //written len
                if (err == ERR_OK)
                {
                    nWritten += (int)len;
                }
                else
                {
                    //re-connect
                    osEventFlagsSet(ConnectEthEventID, 1 << reConEthEnum);
                    break;
                }
            }
        }
        else if (sendMsgObj.type == 2)
        {
            udp_client_send(sendMsgObj.ptr, sendMsgObj.len);
        }
    }
}

void ethConnectRelease()
{
    //release tcp/udp stack
    ethConnectStatus = 0;
    TCP_UDP_Stack_Release();

    netconn_close(conn);  //close session
    netconn_delete(conn); //free memory
    osDelay(RECONNECT_INTERVAR_ETH);
}

//return 0 ~ success, -1 ~ fail
int ethConnectTcpUdp()
{
    ip_addr_t server_addr; //server address
    err_t err;
 
    int tryDhcp = 5;
    while (MX_LWIP_checkIsystem_ip_addr() == 0)
    {
        osDelay(1000);
        if(tryDhcp-- < 0)
        {
            //error connect
            return -1;
        }
    }

    //debug
    uint8_t* ipLWIP = MX_LWIP_getIP();
    if(ipLWIP != NULL)
    {
        LOG_WRITE(">>> ip: %d %d %d %d\n", ipLWIP[0], ipLWIP[1], ipLWIP[2], ipLWIP[3]);
    }

    conn = netconn_new(NETCONN_TCP); //new tcp netconn

    if (conn != NULL)
    {
        int tryGetIp = 5;
        while (tryGetIp--)
        {
            err = netconn_gethostbyname(serverDomain, &server_addr);
            if (err == ERR_OK)
            {
                break;
            }
        }

        if (err == ERR_OK)
            err = netconn_connect(conn, &server_addr, serverPort); //connect to the server

        if (err == ERR_OK)
        {
            ethConnectStatus = 1; //done TCP
            udp_client_connect(server_addr, serverPort);
            TCP_UDP_Stack_Init(ConnectEthEventID, doneTLSEthEnum, reConEthEnum, true);
            TCP_Request(); //request rsa pub key

            return 0;
        }
    }
    else
    {
        LOG_WRITE("create new tcp conn error\n");
    }
    return -1;

}

//receive data, put in loop
void ethRecv()
{
    struct netbuf *buf;
    void *data;
    u16_t len; //buffer length

    err_t err = netconn_recv(conn, &buf);

    LOG_WRITE("net conn recv %d\n", err);

    if (err == ERR_OK)
    {
        do
        {
            netbuf_data(buf, &data, &len); //receive data pointer & length
            TCP_Packet_Analyze(data, len);
        } while (netbuf_next(buf) >= 0); //check buffer empty
        netbuf_delete(buf);              //clear buffer
    }
    else
    {
        osEventFlagsSet(ConnectEthEventID, 1 << reConEthEnum);
    }
}
int countethConnectTask = 0;
void ethConnectTask()
{
    //first time
    if(ethConnectTcpUdp() != 0)
    {
        osEventFlagsSet(ConnectEthEventID, 1 << reConEthEnum); //to re connect
    }

    LOG_WRITE("connectEthTask\n");
    int connectEthFlag;
    for (;;)
    {
        connectEthFlag = osEventFlagsWait(ConnectEthEventID, 0xFF, osFlagsWaitAny, 0);
        countethConnectTask++;
        // LOG_WRITE("connectEthFlag %d\n", connectEthFlag);
        if (connectEthFlag & (1 << errorEthEnum))
        {
            LOG_WRITE("conEthFlag err\n");
            //reboot stm32 by watch-dog
        }
        else if (connectEthFlag & (1 << reConEthEnum))
        {
            ethConnectRelease();
            if(ethConnectTcpUdp() != 0)
            {
                osEventFlagsSet(ConnectEthEventID, 1 << errConEthEnum); //to re connect
                continue;
            }
        }
        else if (connectEthFlag & (1 << doneTLSEthEnum))
        {
            ethConnectStatus = 2;
        }
        
        //recv data
        ethRecv();
    }
}

bool ethSendIP(int type, uint8_t *data, int len)
{
    if (type != 1 && type != 2)
    {
        return false;
    }
    LOG_WRITE("eth send %d\n", type);
    sendEthPack sendMsgObj;
    sendMsgObj.ptr = data;
    sendMsgObj.len = len;
    sendMsgObj.type = type;
    osStatus_t sendMsgStt = osMessageQueuePut(SendEthQueueID, &sendMsgObj, 0U, 0U);

    return (sendMsgStt == osOK);
}
