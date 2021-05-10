#include "sweep.h"


/*
********************************************************************************
 
system_stm32f4xx.c 文件中 voidSetSysClock(void) 函数对时钟的配置如下：

HCLK = SYSCLK / 1     (AHB1Periph)
 
PCLK2 = HCLK / 2      (APB2Periph)
 
PCLK1 = HCLK / 4      (APB1Periph)
 
因为APB1 prescaler != 1, 所以 APB1上的TIMxCLK = PCLK1 x 2 = SystemCoreClock/ 2;
 
因为APB2 prescaler != 1, 所以 APB2上的TIMxCLK = PCLK2 x 2 =SystemCoreClock;
 
APB1 定时器有 TIM2, TIM3,TIM4, TIM5, TIM6, TIM7, TIM12, TIM13, TIM14
 
APB2 定时器有 TIM1, TIM8,TIM9, TIM10, TIM11
 
TIM 更新周期是 = TIMCLK / （TIM_Period + 1）/（TIM_Prescaler+ 1）
********************************************************************************
*/

/**
 * @brief 	扫频定时器中断初始化函数
 * 
 * @param		arr
 *					psc
 *
 * @return	none 
 */
void TIM4_Int_Init_2(u16 arr,u16 psc)   //通用定时器3初始化   84MHz
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;  
    NVIC_InitTypeDef NVIC_InitStruct;
 
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4,ENABLE);   //使能计时器的APB1时钟
 
		TIM_TimeBaseInitStructure.TIM_Period=arr; //设置自动重装值，该值由外部输入
		TIM_TimeBaseInitStructure.TIM_Prescaler=psc; //设置定时器预分频
		TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; //设置时钟分频（这里为1分频）
		TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up;//设置计数模式（向上计数）
		TIM_TimeBaseInit(TIM4,&TIM_TimeBaseInitStructure);TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE); //使能中断
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update); //事先清除中断标志位
		
		TIM_Cmd(TIM4,ENABLE);

		NVIC_InitStruct.NVIC_IRQChannel = TIM4_IRQn; //设置中断优先级及使能
		NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority=2;
		NVIC_InitStruct.NVIC_IRQChannelSubPriority=3;
		NVIC_InitStruct.NVIC_IRQChannelCmd=ENABLE;
		NVIC_Init(&NVIC_InitStruct);

}

/**
 * @brief 	TIM4中断服务函数
 * 
 * @param		none
 * @return	none 
 */
void TIM4_IRQHandler(void)  // 定时器中断服务函数
{
	if(TIM_GetITStatus(TIM4,TIM_IT_Update)==SET) //溢出，中断
	{
		//printf("tim2\n");
		Recent_Freq++;
		if(Recent_Freq >= 1000) Recent_Freq = 1;
		
		if(wave_jugde == SIN){
			if(Recent_Freq >= 50000) dac1_SetSinWave_100kHz(Recent_A,Recent_Freq);
			else if(Recent_Freq < 50000) dac1_SetSinWave(Recent_A,Recent_Freq);
		}
		else if(wave_jugde == TRI){
			if(Recent_Freq >= 50000) dac1_SetTriWave_100kHz(0, Recent_A, Recent_Freq, 50);
			else if(Recent_Freq < 50000) dac1_SetTriWave(0, Recent_A, Recent_Freq, 50);
		}
		else {
			if(Recent_Freq >= 50000) dac1_SetRectWave_100kHz(0, Recent_A, Recent_Freq, 50);
			else if(Recent_Freq < 50000) dac1_SetRectWave(0, Recent_A, Recent_Freq, 50);
		}
	}
	TIM_ClearITPendingBit(TIM4,TIM_IT_Update);  //重置计数器
}
