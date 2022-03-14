#include "mp3Frame.h"

int numOfPacketSaving = 0;
int buffTcpPacket_wrindex = 0;
int buffTcpPacket_rdindex = 0;
// FrameStruct *headFrame = NULL, *curFrame = NULL;
int newVol = 98;
// int timeFrame;
// int timePacket; // = numOframe * timePerFrame

extern osMutexId_t buffTcpPacket_mtID;

PacketStruct buffTcpPacket[TCP_PACKET_BUFF_SIZE_MAX] = {
    {.bool_isempty = 1, .timestamp = 0},
    {.bool_isempty = 1, .timestamp = 0},
    {.bool_isempty = 1, .timestamp = 0},
    {.bool_isempty = 1, .timestamp = 0},
    {.bool_isempty = 1, .timestamp = 0},
    {.bool_isempty = 1, .timestamp = 0},
    {.bool_isempty = 1, .timestamp = 0},
    {.bool_isempty = 1, .timestamp = 0},
    {.bool_isempty = 1, .timestamp = 0},
    {.bool_isempty = 1, .timestamp = 0},
    {.bool_isempty = 1, .timestamp = 0},
    {.bool_isempty = 1, .timestamp = 0}
};

#define MP3LOG LOG_WRITE

// int64_t curTimeDebug, offsetTimeDebug, curTimeDebugFirst = 0, curTimeDebugSecond = 0;

int getCurrentNumOfPacket()
{
    return numOfPacketSaving;
}

//get adu and frame from tcp packet and save to memory. this version, get each frame is come in time
// void mp3GetFrame_old(MP3Struct *mp3Packet, int len)
// {
//     MP3LOG("mp3GetFrame\n");
// 
//     //check property packet, len of adu and mp3 frame
//     uint8_t *tmpPtr = mp3Packet->data + mp3Packet->sizeOfFirstFrame + (mp3Packet->numOfFrame - 1) * mp3Packet->frameSize;
//     if (tmpPtr != (uint8_t *)mp3Packet + len)
//         return;
// 
//     //get volume, timeperframe
//     newVol = mp3Packet->volume;
//     timeFrame = mp3Packet->timePerFrame;
//     timePacket = mp3Packet->timePerFrame * mp3Packet->numOfFrame;
// 
//     //check time, before save
//     int64_t curTime = TCP_UDP_GetNtpTime();
//     curTimeDebug = curTime;
//     if(curTime == 0) return; //don't have ntp time
//     int64_t offsetTime = mp3Packet->timestamp - curTime;
//     offsetTimeDebug = offsetTime;
// 
//     //debug
//     if(curTimeDebugFirst == 0) curTimeDebugFirst = curTime;
// //    LOG_WRITE(">>> offset recv time %d", (int)offsetTime);
// 
// //   return;
// 
//     int numOfFrame;     //copy from tcp packet to memory
// 
//     MP3LOG("mp3 packet offsetTime %lld\n", offsetTime);
// 
//     if (offsetTime < 0) //packet come slow, need remove some frames
//     {
// 
//         int64_t numOfFrameWillRemove = (-offsetTime) / mp3Packet->timePerFrame + 1; // + 1 to margin
//         numOfFrame = (int)mp3Packet->numOfFrame - (int)numOfFrameWillRemove;
// 
//         //debug     /////////////////////////
//         if (numOfFrame < 0) //sth wrong ?
//         {
//             LOG_WRITE("numOfFrame < 0 \n");
//             return;
//         }
//         /////////////////////////////////////
// 
//         numOfFrame = min(numOfFrame, (int)mp3Packet->numOfFrame);
//     }
//     else
//     {
//         numOfFrame = (int)mp3Packet->numOfFrame;
//     }
// 
//     //remain slot to save frame
//     int remainSlotSaveFrame = MP3_NUM_OF_FRAME_MAX - numOfMp3FrameSaving;
// 
//     //debug     //////////////////////
//     if (remainSlotSaveFrame < 0) //sth wrong ?
//     {
//         LOG_WRITE("remainSlotSaveFrame < 0 \n");
//         return;
//     }
//     //////////////////////////////////
// 
//     numOfFrame = min(numOfFrame, remainSlotSaveFrame);
// 
//     
// 
//     int numOfFrameRemoved = mp3Packet->numOfFrame - numOfFrame;               //use this to update timestamp
//     mp3Packet->timestamp += (int)mp3Packet->timePerFrame * numOfFrameRemoved; //update new timestamp
// 
//     int realFrameSize = mp3Packet->frameSize + 4; //4B for header
//     uint8_t *framePtr = mp3Packet->data;
//     //copy numOfFrame frame
//     for (int i = 0; i < (int)numOfFrame; i++)
//     {
//         FrameStruct *newFrame = (FrameStruct *)malloc(sizeof(FrameStruct));
//         if (newFrame != NULL) //success allocate
//         {
//             newFrame->nextFrame = NULL;
//             newFrame->prevFrame = NULL;
// 
//             if (i == 0) //adu frame
//                 // newFrame->data = (uint8_t *)malloc(ADU_FRAME_SIZE);
//                 newFrame->data = (uint8_t *)malloc(mp3Packet->sizeOfFirstFrame);
//             else //mp3 frame
//                 newFrame->data = (uint8_t *)malloc(realFrameSize);
//             if (newFrame->data != NULL)
//             {
//                 if (i == 0)
//                 {
//                     //copy data
//                     memcpy(newFrame->data, framePtr, mp3Packet->sizeOfFirstFrame);
//                     newFrame->len = mp3Packet->sizeOfFirstFrame;
//                     // newFrame->len = ADU_FRAME_SIZE;
//                     framePtr += mp3Packet->sizeOfFirstFrame;
// 
//                     // //change bitrate to 144kbps
//                     // newFrame->data[2] &= 0x0F;
//                     // newFrame->data[2] |= 0xD0; //0b1101 0000
//                     // //clear backpoint of playbuff
//                     // newFrame->data[4] = 0;
// 
//                     //fill 0x00 byte
//                     // int i;
//                     // for(i = mp3Packet->sizeOfFirstFrame; i < ADU_FRAME_SIZE; i++)
//                     // {
//                     //     newFrame->data[i] = 0;
//                     // }
//                 }
//                 else
//                 {
//                     //copy data, first 4B header, then ....
//                     memcpy(newFrame->data, mp3Packet->data, 4);
//                     memcpy(newFrame->data + 4, framePtr, mp3Packet->frameSize);
//                     newFrame->len = realFrameSize;
//                     framePtr += mp3Packet->frameSize;
//                 }
// 
//                 //copy value id, session, timestamp
//                 if (i == (int)numOfFrame - 1) //last frame
//                     newFrame->isTail = true;
//                 else
//                     newFrame->isTail = false;
//                 newFrame->session = mp3Packet->session;
//                 newFrame->id = mp3Packet->frameID + i;
//                 newFrame->timestamp = mp3Packet->timestamp + i * mp3Packet->timePerFrame;
// 
//                 if (curFrame != NULL)
//                 {
//                     curFrame->nextFrame = newFrame;
//                     newFrame->prevFrame = curFrame;
//                 }
//                 curFrame = newFrame;
//                 if (headFrame == NULL) //first time
//                     headFrame = newFrame;
// 
//                 numOfMp3FrameSaving++;
//             }
//             else
//             {
//                 //free newFrame
//                 free(newFrame);
//                 LOG_WRITE("fail allocate new frame data\n");
//                 break; //not enough data
//             }
//         }
//         else
//         {
//             LOG_WRITE("fail allocate new frame node\n");
//         }
//     }
// 
//     //debug
//     if(numOfMp3FrameSaving >= 10 && curTimeDebugSecond == 0) curTimeDebugSecond = TCP_UDP_GetNtpTime();
// }

//this version ignore all packet mp3(include many frames) if this is not in time

uint16_t caculateChecksum(uint8_t *data, int offset, int length)
{
    uint32_t checkSum = 0;
    int index = offset;
    while (length > 1)
    {
        checkSum += ((uint32_t)data[index] << 8) | ((uint32_t)data[index + 1]); // little edian
        length -= 2;
        index += 2;
    }
    if (length == 1) // still have 1 byte
    {
        checkSum += ((uint32_t)data[index] << 8);
    }
    while ((checkSum >> 16) > 0) // checkSum > 0xFFFF
    {
        checkSum = (checkSum & 0xFFFF) + (checkSum >> 16);
    }
    // inverse
    checkSum = ~checkSum;
    return (uint16_t)checkSum;
}

int miss_save_frame = 0, miss_time_frame = 0, cant_save_frame1 = 0, cant_save_frame2 = 0, cant_save_frame3 = 0;
int64_t offsetPacket, offsetPacket_lowest = 1000, packet_timeout = 0;
void mp3SaveFrame(MP3Struct *mp3Packet, int len)
{
    static int oldSession = -1, oldID = -1;

    MP3LOG("mp3SaFr\n");

    //check property packet, len of adu and mp3 frame
    uint8_t *tmpPtr = mp3Packet->data + mp3Packet->sizeOfFirstFrame + (mp3Packet->numOfFrame - 1) * mp3Packet->frameSize;
    if (tmpPtr != (uint8_t *)mp3Packet + len || mp3Packet->sizeOfFirstFrame > ADU_FRAME_SIZE \
        || mp3Packet->frameSize + 4 != MP3_FRAME_SIZE || mp3Packet->numOfFrame != NUM_OF_FRAME_IN_TCP_PACKET) {
        MP3LOG("err0\n");
        cant_save_frame1++;
        return;
    }

    if(oldSession == -1 || oldSession != mp3Packet->session) {
        oldSession = mp3Packet->session;
        oldID = mp3Packet->frameID;
    } else {
        if(oldID + 5 < mp3Packet->frameID) {
            miss_save_frame += mp3Packet->frameID - oldID - 5;
        }
        oldID = mp3Packet->frameID;
    }
    {
        int64_t t1 = TCP_UDP_GetNtpTime();
        if(t1 != 0) {
            offsetPacket = mp3Packet->timestamp - t1;
            if(offsetPacket < offsetPacket_lowest)
                offsetPacket_lowest = offsetPacket;
            if(offsetPacket < 0)
                miss_time_frame++;
        }
    }

    // goto exit;

    //get volume, timeperframe
    newVol = mp3Packet->volume;
    // timeFrame = mp3Packet->timePerFrame;
    // timePacket = mp3Packet->timePerFrame * mp3Packet->numOfFrame;

    //check time, before save
    int64_t curTime = TCP_UDP_GetNtpTime();
    if(curTime == 0) { //don't have ntp time
        cant_save_frame2++;
        return;
    }

    int64_t offsetTime = mp3Packet->timestamp - curTime;

    // MP3LOG("mp3 packet offsetTime %ld\n", offsetTime);

    if (offsetTime <= 24) //packet come slow, need remove some frames
    {
        // MP3LOG("mp3 packet timeout\n");
        packet_timeout++;
        // return;
    }

    osStatus_t status = osMutexAcquire(buffTcpPacket_mtID, WAIT_TCP_PACKET_BUFF_MUTEX);
    if(status == osOK) { //acquire success
        if(buffTcpPacket[buffTcpPacket_wrindex].bool_isempty) {
            buffTcpPacket[buffTcpPacket_wrindex].frameID = mp3Packet->frameID;
            buffTcpPacket[buffTcpPacket_wrindex].session = mp3Packet->session;

            //get timestampe
            buffTcpPacket[buffTcpPacket_wrindex].timestamp = mp3Packet->timestamp;
            
            //save packet
            //adu first
            //copy data
            uint8_t *mp3Ptr = buffTcpPacket[buffTcpPacket_wrindex].mp3Frame;
            uint8_t *framePtr = mp3Packet->data;
            //change bitrate to 144kbps
            // framePtr[2] &= 0x0F;
            // framePtr[2] |= 0xD0; //0b1101 0000
            // //clear backpoint of playbuff
            // framePtr[4] = 0;

            memcpy(mp3Ptr, framePtr, mp3Packet->sizeOfFirstFrame);
            framePtr += mp3Packet->sizeOfFirstFrame;

            // fill 0x00 byte
            int j;
            // for(j = mp3Packet->sizeOfFirstFrame; j < ADU_FRAME_SIZE; j++) {
            //     *(mp3Ptr + j) = 0;
            // }
            mp3Ptr += ADU_FRAME_SIZE;

            //copy another mp3 packet
            for (j = 0; j < (NUM_OF_FRAME_IN_TCP_PACKET - 1); j++) {
                //copy data, first 4B header, then ....
                memcpy(mp3Ptr, mp3Packet->data, 4);
                memcpy(mp3Ptr + 4, framePtr, mp3Packet->frameSize);
                framePtr += mp3Packet->frameSize;
                mp3Ptr += MP3_FRAME_SIZE;
            }
            buffTcpPacket[buffTcpPacket_wrindex].bool_isempty = 0;

            //debug
            // uint16_t chsum = caculateChecksum(buffTcpPacket[buffTcpPacket_wrindex].mp3Frame, 0, MP3_BLOCK_SIZE);
            // printf("**cksum2 %d 0x%04X\n", buffTcpPacket[buffTcpPacket_wrindex].frameID, chsum);

            buffTcpPacket_wrindex++;
            if(buffTcpPacket_wrindex >= TCP_PACKET_BUFF_SIZE_MAX)
                buffTcpPacket_wrindex = 0;
        } else
        {
            cant_save_frame3++;
        }
        osMutexRelease(buffTcpPacket_mtID);
    }
}

int mp3GetVol()
{
    // MP3LOG("mp3GetVol\n");
    return newVol;
}

//mp3 get frame, if frame is out_of_time remove(empty=1), if 
int miss_get_frame = 0, offset_get_frame = 0, offset_get_frame_lowest = 1000;
int mp3GetFrame(uint8_t *buf, int buf_size) {
    static int oldSession = -1, oldID = -1;

    // MP3LOG("mp3SaFr\n");
    if(buf_size != (MP3_BLOCK_SIZE)) {
        MP3LOG("err0\n");
        return -1;
    }
    
    int64_t curTime = 0, offset = 0;
    int error = 0;
    //check timestamp of current 
    //check time, before save

    osStatus_t status = osMutexAcquire(buffTcpPacket_mtID, WAIT_TCP_PACKET_BUFF_MUTEX);
    if(status != osOK) {
        MP3LOG("err1\n");
        return -1;
    }

    for(;;) {
        curTime = TCP_UDP_GetNtpTime();
        if(curTime == 0) { //don't have ntp time
            error = 1;
            break; 
        }
        if(buffTcpPacket[buffTcpPacket_rdindex].bool_isempty) {
            error = 1;
            break;
        }
        //else
        offset = buffTcpPacket[buffTcpPacket_rdindex].timestamp - curTime;
        offset_get_frame = offset;
        if(offset_get_frame < offset_get_frame_lowest)
            offset_get_frame_lowest = offset_get_frame;
        if(offset > (5*24)) { //two soon to get this frame
            error = 1;
            break;
        }
        else if(offset < 0) {
            //this frame is out_of_time need to remove
            buffTcpPacket[buffTcpPacket_rdindex].bool_isempty = 1;
            buffTcpPacket[buffTcpPacket_rdindex].timestamp = 0;
            buffTcpPacket_rdindex++;
            if(buffTcpPacket_rdindex >= TCP_PACKET_BUFF_SIZE_MAX)
                buffTcpPacket_rdindex = 0;
            continue;
        } else {
            //this frames is siutable
            memcpy(buf, buffTcpPacket[buffTcpPacket_rdindex].mp3Frame, buf_size);
            // memcpy(buf2, buffTcpPacket[buffTcpPacket_rdindex].mp3Frame, buf_size);
            //this frame is used need to remove
            buffTcpPacket[buffTcpPacket_rdindex].bool_isempty = 1;
            buffTcpPacket[buffTcpPacket_rdindex].timestamp = 0;

            //debug
            // uint16_t chsum = caculateChecksum(buf, 0, MP3_BLOCK_SIZE);
            // printf("*cksum %d 0x%04X\n", buffTcpPacket[buffTcpPacket_rdindex].frameID, chsum);
            if(oldSession == -1 || oldSession != buffTcpPacket[buffTcpPacket_rdindex].session) {
                oldSession = buffTcpPacket[buffTcpPacket_rdindex].session;
                oldID = buffTcpPacket[buffTcpPacket_rdindex].frameID;
            } else {
                if(oldID + 5 < buffTcpPacket[buffTcpPacket_rdindex].frameID) {
                    miss_get_frame += buffTcpPacket[buffTcpPacket_rdindex].frameID - oldID - 5;
                }
                oldID = buffTcpPacket[buffTcpPacket_rdindex].frameID;
            }

            buffTcpPacket_rdindex++;
            if(buffTcpPacket_rdindex >= TCP_PACKET_BUFF_SIZE_MAX)
                buffTcpPacket_rdindex = 0;
            break;
        }
    }

    osMutexRelease(buffTcpPacket_mtID);
    // MP3LOG("reerr%d\n", -error);
    return (-error);
}

// FrameStruct *mp3GetHeadFrame()
// {
//     return headFrame;
// }

//get frame has timestamp(ts), ntp-ts > 0, < 48

// FrameStruct *mp3GetNewFrame()
// {
//     // MP3LOG("mp3GetNewFrame\n");
//     //check time
//     // int64_t curTime = TCP_UDP_GetNtpTime();
//     int64_t offsetTime = TCP_UDP_GetNtpTime() - headFrame->timestamp;
//     if(offsetTime < 0) 
//         return NULL;
//     else if(offsetTime < 40) 
//         return headFrame;
//     else // >= 40
//     {
//         //remove all frame to tail
//         mp3RemoveTcpFrame(headFrame);
//     }
//     
//     return headFrame;
// }

// void mp3RemoveFrame(FrameStruct *frame)
// {
//     MP3LOG("mp3RemoveFrame\n");
//     if(frame != NULL)
//     {
//         if(frame->data != NULL)
//             free(frame->data);
// 
//         headFrame = frame->nextFrame; //point to next frame
// 
//         free(frame);
//         numOfMp3FrameSaving--;
//     }
// }

//remove from current frame to tail frame

// void mp3RemoveTcpFrame(FrameStruct *frame)
// {
//     MP3LOG("mp3RemoveTcpFrame\n");
//     for(;;)
//     {
//         if(frame == NULL) return; //has nothing to delete
//         bool endProcess = frame->isTail;
// 
//         mp3RemoveFrame(frame);
//         frame = headFrame;
// 
//         if(endProcess) return; //reach tail
//     }
// }

//return true ~ valid frame; false ~ fail, need read next frame
// int64_t curTimeDebug3 = 0;

// bool mp3CheckFrame(FrameStruct *frame)
// {
//     MP3LOG("mp3CheckFrame\n");
//     
//     static bool IsRemoveTail = false;
// 
//     if(frame == NULL) return false;
//     if(frame->data == NULL)
//     {
//         headFrame = frame->nextFrame;
//         free(frame);
//     }
// 
//     if(IsRemoveTail && frame->isTail)
//     {
//         //remove tail
//         mp3RemoveFrame(frame);
//         IsRemoveTail = false;
//         return false;
//     }
// 
//     //check time
//     int64_t curTime = TCP_UDP_GetNtpTime();
//     int64_t offsetTime = TCP_UDP_GetNtpTime() - frame->timestamp;
// 
//     if(offsetTime > timePacket) //remove all frame to tail
//     {
//     	if(curTimeDebug3 == 0) curTimeDebug3 = TCP_UDP_GetNtpTime();
//         mp3RemoveTcpFrame(frame);
//         MP3LOG("mp3RemoveTcpFrame\n");
//         return false;
//     }
//     else if(offsetTime > 2 * timeFrame) //remove just tail frame
//     {
//         MP3LOG("mp3 remove tail\n");
//     	if(curTimeDebug3 == 0) curTimeDebug3 = TCP_UDP_GetNtpTime();
//         if(frame->isTail)
//         {
//             mp3RemoveFrame(frame);
//             IsRemoveTail = false;
//             return false;
//         }
//         else
//         { 
//             IsRemoveTail = true;
//             return true;
//         }
//     } 
//     else if(offsetTime < -timeFrame)
//     {
//         return false; //still early to play
//     }
//     else // -24 -> 48 ms
//     {
//     	if(curTimeDebug3 == 0) curTimeDebug3 = TCP_UDP_GetNtpTime();
//         IsRemoveTail = false;
//         MP3LOG("mp3 vaild frame %d\n", frame->id);
//         return true;
//     }
// }
