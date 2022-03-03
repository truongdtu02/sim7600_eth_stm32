#include "debugAPI.h"

DMA_POS_Struct dma_pos_array[DMA_POS_LOG_LEN];

int curOrderDmaPos = 0;
long int cnt_save = 0;

void savePosDma(int oldPos, int newPos, int returnTmp, int ndtr)
{
	dma_pos_array[curOrderDmaPos].order = cnt_save++;
    dma_pos_array[curOrderDmaPos].old_pos = oldPos;
    dma_pos_array[curOrderDmaPos].new_pos = newPos;
    dma_pos_array[curOrderDmaPos].return_Tmp = returnTmp;
    dma_pos_array[curOrderDmaPos].ndtr = ndtr;
    curOrderDmaPos++;
    if(curOrderDmaPos >= DMA_POS_LOG_LEN)
        curOrderDmaPos = 0;
}
