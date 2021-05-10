#ifndef __ADC_H
#define __ADC_H	
#include "sys.h" 


void  Adc_Init(void);
void TIM3_Int_Init_2(u16 arr,u16 psc);   //通用定时器3初始化;
void wave_form(void);

#endif 















