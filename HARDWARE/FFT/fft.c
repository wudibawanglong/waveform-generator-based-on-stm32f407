#include "fft.h"


/**
 * @brief 	找第二大值
 * 
 * @param		fft_outputbuf[] : fft输出的数组
 *					len : 数组长度
 *
 * @return	none 
 */
void Find_Second(float fft_outputbuf[], u16 len)
{
	u16 i;
	for(i = 1; i < len; i++) {
		if(i == max_index) continue;
		if(fft_outputbuf[i] > second){
			second = fft_outputbuf[i];
			second_index = i;
		}
	}
}

/**
 * @brief 	找第三大值
 * 
 * @param		fft_outputbuf[] : fft输出的数组
 *					len : 数组长度
 *
 * @return	none 
 */
void Find_Third(float fft_outputbuf[], u16 len)
{
	u16 i;
	for (i = 1; i < len; i++){
		if(i == max_index || i == second_index) continue;
		if(fft_outputbuf[i] > third){
			third = fft_outputbuf[i];
			third_index = i;
		}
	}
}
