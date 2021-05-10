#ifndef __DAC_H
#define __DAC_H	 
#include "sys.h"	     			    
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32F407开发板
//DAC 驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2014/5/6
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved 
////////////////////////////////////////////////////////////////////////////////// 	
#define dot_count 50 //点数  
#define sine      1	  //正弦波
#define triangle  2	  //三角波
#define square    3   //方波
#define sawtooth  4   //锯齿波

//信号发生函数
//参数：类型--频率--峰值--占空比
void Signal_Generator(u16 wf_mode, u16 v_peak, u32 frequency);


void bsp_InitDAC1(void);
void dac1_InitForDMA(uint32_t _BufAddr, uint32_t _Count, uint32_t _DacFreq);
void dac1_SetRectWave(uint16_t _low, uint16_t _high, uint32_t _freq, uint16_t _duty);
void dac1_SetRectWave_100kHz(uint16_t _low, uint16_t _high, uint32_t _freq, uint16_t _duty);
void dac1_SetSinWave(uint16_t _vpp, uint32_t _freq);
void dac1_SetSinWave_100kHz(uint16_t _vpp, uint32_t _freq);
void dac1_SetTriWave(uint16_t _low, uint16_t _high, uint32_t _freq, uint16_t _duty);
void dac1_SetTriWave_100kHz(uint16_t _low, uint16_t _high, uint32_t _freq, uint16_t _duty);

void Show_Wave(void);

#endif

















