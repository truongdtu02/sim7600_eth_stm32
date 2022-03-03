#ifndef DEBUG_API_H_
#define DEBUG_API_H_

#define DMA_POS_LOG_LEN 100

typedef struct
{
	long int order;
    int old_pos;
    int new_pos;
    int return_Tmp;
    int ndtr;
} DMA_POS_Struct;

void savePosDma(int oldPos, int newPos, int returnTmp, int ndtr);

#endif //DEBUG_API_H_
