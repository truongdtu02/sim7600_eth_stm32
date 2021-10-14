#include "debugAPI.h"

DMA_POS_Struct dma_pos_array[DMA_POS_LOG_LEN];

unsigned int curOrderDmaPos = 0;

void savePosDma(int oldPos, int newPos, int returnTmp)
{
    int index = curOrderDmaPos % DMA_POS_LOG_LEN;
	dma_pos_array[index].order = curOrderDmaPos;
    dma_pos_array[index].old_pos = oldPos;
    dma_pos_array[index].new_pos = newPos;
    dma_pos_array[index].return_Tmp = returnTmp;
    curOrderDmaPos++;
}
