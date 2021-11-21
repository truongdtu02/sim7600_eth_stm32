#include "mp3Frame.h"

int numOfMp3FrameSaving = 0;
FrameStruct *headFrame = NULL, *curFrame = NULL;
int newVol = 98;
int timeFrame;
int timePacket; // = numOframe * timePerFrame

#define MP3LOG

int64_t curTimeDebug, offsetTimeDebug, curTimeDebugFirst = 0, curTimeDebugSecond = 0;

int getCurrentNumOfFrame()
{
    return numOfMp3FrameSaving;
}

//get adu and frame from tcp packet and save to memory
void mp3GetFrame(MP3Struct *mp3Packet, int len)
{
    MP3LOG("mp3GetFrame\n");

    //check property packet, len of adu and mp3 frame
    uint8_t *tmpPtr = mp3Packet->data + mp3Packet->sizeOfFirstFrame + (mp3Packet->numOfFrame - 1) * mp3Packet->frameSize;
    if (tmpPtr != (uint8_t *)mp3Packet + len)
        return;

    //get volume, timeperframe
    newVol = mp3Packet->volume;
    timeFrame = mp3Packet->timePerFrame;
    timePacket = mp3Packet->timePerFrame * mp3Packet->numOfFrame;

    //check time, before save
    int64_t curTime = TCP_UDP_GetNtpTime();
    curTimeDebug = curTime;
    if(curTime == 0) return; //don't have ntp time
    int64_t offsetTime = mp3Packet->timestamp - curTime;
    offsetTimeDebug = offsetTime;

    //debug
    if(curTimeDebugFirst == 0) curTimeDebugFirst = curTime;
//    LOG_WRITE(">>> offset recv time %d", (int)offsetTime);

//   return;

    int numOfFrame;     //copy from tcp packet to memory
    if (offsetTime < 0) //packet come slow, need remove some frames
    {
        int64_t numOfFrameWillRemove = (-offsetTime) / mp3Packet->timePerFrame + 1; // + 1 to margin
        numOfFrame = (int)mp3Packet->numOfFrame - (int)numOfFrameWillRemove;

        //debug     /////////////////////////
        if (numOfFrame < 0) //sth wrong ?
        {
            LOG_WRITE("numOfFrame < 0 \n");
            return;
        }
        /////////////////////////////////////

        numOfFrame = min(numOfFrame, (int)mp3Packet->numOfFrame);
    }
    else
    {
        numOfFrame = (int)mp3Packet->numOfFrame;
    }

    //remain slot to save frame
    int remainSlotSaveFrame = MP3_NUM_OF_FRAME_MAX - numOfMp3FrameSaving;

    //debug     //////////////////////
    if (remainSlotSaveFrame < 0) //sth wrong ?
    {
        LOG_WRITE("remainSlotSaveFrame < 0 \n");
        return;
    }
    //////////////////////////////////

    numOfFrame = min(numOfFrame, remainSlotSaveFrame);

    

    int numOfFrameRemoved = mp3Packet->numOfFrame - numOfFrame;               //use this to update timestamp
    mp3Packet->timestamp += (int)mp3Packet->timePerFrame * numOfFrameRemoved; //update new timestamp

    int realFrameSize = mp3Packet->frameSize + 4; //4B for header
    uint8_t *framePtr = mp3Packet->data;
    //copy numOfFrame frame
    for (int i = 0; i < (int)numOfFrame; i++)
    {
        FrameStruct *newFrame = (FrameStruct *)malloc(sizeof(FrameStruct));
        if (newFrame != NULL) //success allocate
        {
            newFrame->nextFrame = NULL;
            newFrame->prevFrame = NULL;

            if (i == 0) //adu frame
                // newFrame->data = (uint8_t *)malloc(ADU_FRAME_SIZE);
                newFrame->data = (uint8_t *)malloc(mp3Packet->sizeOfFirstFrame);
            else //mp3 frame
                newFrame->data = (uint8_t *)malloc(realFrameSize);
            if (newFrame->data != NULL)
            {
                if (i == 0)
                {
                    //copy data
                    memcpy(newFrame->data, framePtr, mp3Packet->sizeOfFirstFrame);
                    newFrame->len = mp3Packet->sizeOfFirstFrame;
                    // newFrame->len = ADU_FRAME_SIZE;
                    framePtr += mp3Packet->sizeOfFirstFrame;

                    // //change bitrate to 144kbps
                    // newFrame->data[2] &= 0x0F;
                    // newFrame->data[2] |= 0xD0; //0b1101 0000
                    // //clear backpoint of playbuff
                    // newFrame->data[4] = 0;

                    //fill 0x00 byte
                    // int i;
                    // for(i = mp3Packet->sizeOfFirstFrame; i < ADU_FRAME_SIZE; i++)
                    // {
                    //     newFrame->data[i] = 0;
                    // }
                }
                else
                {
                    //copy data, first 4B header, then ....
                    memcpy(newFrame->data, mp3Packet->data, 4);
                    memcpy(newFrame->data + 4, framePtr, mp3Packet->frameSize);
                    newFrame->len = realFrameSize;
                    framePtr += mp3Packet->frameSize;
                }

                //copy value id, session, timestamp
                if (i == (int)numOfFrame - 1) //last frame
                    newFrame->isTail = true;
                else
                    newFrame->isTail = false;
                newFrame->session = mp3Packet->session;
                newFrame->id = mp3Packet->frameID + i;
                newFrame->timestamp = mp3Packet->timestamp + i * mp3Packet->timePerFrame;

                if (curFrame != NULL)
                {
                    curFrame->nextFrame = newFrame;
                    newFrame->prevFrame = curFrame;
                }
                curFrame = newFrame;
                if (headFrame == NULL) //first time
                    headFrame = newFrame;

                numOfMp3FrameSaving++;
            }
            else
            {
                //free newFrame
                free(newFrame);
                LOG_WRITE("fail allocate new frame data\n");
                break; //not enough data
            }
        }
        else
        {
            LOG_WRITE("fail allocate new frame node\n");
        }
    }

    //debug
    if(numOfMp3FrameSaving >= 10 && curTimeDebugSecond == 0) curTimeDebugSecond = TCP_UDP_GetNtpTime();
}

int mp3GetVol()
{
    MP3LOG("mp3GetVol\n");
    return newVol;
}

FrameStruct *mp3GetHeadFrame()
{
    return headFrame;
}


//get frame has timestamp(ts), ntp-ts > 0, < 48
FrameStruct *mp3GetNewFrame()
{
    // MP3LOG("mp3GetNewFrame\n");
    //check time
    // int64_t curTime = TCP_UDP_GetNtpTime();
    int64_t offsetTime = TCP_UDP_GetNtpTime() - headFrame->timestamp;
    if(offsetTime < 0) 
        return NULL;
    else if(offsetTime < 40) 
        return headFrame;
    else // >= 40
    {
        //remove all frame to tail
        mp3RemoveTcpFrame(headFrame);
    }
    
    return headFrame;
}

void mp3RemoveFrame(FrameStruct *frame)
{
    MP3LOG("mp3RemoveFrame\n");
    if(frame != NULL)
    {
        if(frame->data != NULL)
            free(frame->data);

        headFrame = frame->nextFrame; //point to next frame

        free(frame);
        numOfMp3FrameSaving--;
    }
}

//remove from current frame to tail frame
void mp3RemoveTcpFrame(FrameStruct *frame)
{
    MP3LOG("mp3RemoveTcpFrame\n");
    for(;;)
    {
        if(frame == NULL) return; //has nothing to delete
        bool endProcess = frame->isTail;

        mp3RemoveFrame(frame);
        frame = headFrame;

        if(endProcess) return; //reach tail
    }
}

//return true ~ valid frame; false ~ fail, need read next frame
int64_t curTimeDebug3 = 0;
bool mp3CheckFrame(FrameStruct *frame)
{
    MP3LOG("mp3CheckFrame\n");
    
    static bool IsRemoveTail = false;

    if(frame == NULL) return false;
    if(frame->data == NULL)
    {
        headFrame = frame->nextFrame;
        free(frame);
    }

    if(IsRemoveTail && frame->isTail)
    {
        //remove tail
        mp3RemoveFrame(frame);
        IsRemoveTail = false;
        return false;
    }

    //check time
    int64_t curTime = TCP_UDP_GetNtpTime();
    int64_t offsetTime = TCP_UDP_GetNtpTime() - frame->timestamp;

    if(offsetTime > timePacket) //remove all frame to tail
    {
    	if(curTimeDebug3 == 0) curTimeDebug3 = TCP_UDP_GetNtpTime();
        mp3RemoveTcpFrame(frame);
        MP3LOG("mp3RemoveTcpFrame\n");
        return false;
    }
    else if(offsetTime > 2 * timeFrame) //remove just tail frame
    {
        MP3LOG("mp3 remove tail\n");
    	if(curTimeDebug3 == 0) curTimeDebug3 = TCP_UDP_GetNtpTime();
        if(frame->isTail)
        {
            mp3RemoveFrame(frame);
            IsRemoveTail = false;
            return false;
        }
        else
        { 
            IsRemoveTail = true;
            return true;
        }
    } 
    else if(offsetTime < -timeFrame)
    {
        return false; //still early to play
    }
    else // -24 -> 48 ms
    {
    	if(curTimeDebug3 == 0) curTimeDebug3 = TCP_UDP_GetNtpTime();
        IsRemoveTail = false;
        MP3LOG("mp3 vaild frame %d\n", frame->id);
        return true;
    }
}
