#include "adc.h"
#include "delay.h"		


/*
	ADC3  CH1    PA1
	DMA2  CH2  Stream1     DMA2_Stream1_IRQHandler
	TIM3  
*/


u16 cnt_tim = 0; // 采样计数




//初始化ADC															   
void  Adc_Init(void)
{    
  GPIO_InitTypeDef  GPIO_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStructure;
	ADC_InitTypeDef       ADC_InitStructure;
	
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//使能GPIOA时钟
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE); //使能ADC1时钟

  //先初始化ADC1通道5 IO口
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;//PA5 通道5
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;//模拟输入
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;//不带上下拉
  GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化  
 
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1,ENABLE);	  //ADC1复位
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1,DISABLE);	//复位结束	 
 
	
  ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;//独立模式
  ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;//两个采样阶段之间的延迟5个时钟
  ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled; //DMA失能
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;//预分频4分频。ADCCLK=PCLK2/4=84/4=21Mhz,ADC时钟最好不要超过36Mhz 
  ADC_CommonInit(&ADC_CommonInitStructure);//初始化
	
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;//12位模式
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;//非扫描模式	
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;//关闭连续转换
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;//禁止触发检测，使用软件触发
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;//右对齐	
  ADC_InitStructure.ADC_NbrOfConversion = 1;//1个转换在规则序列中 也就是只转换规则序列1 
  ADC_Init(ADC1, &ADC_InitStructure);//ADC初始化
	
 
	ADC_Cmd(ADC1, ENABLE);//开启AD转换器	

}	


//获得ADC值
//ch: @ref ADC_channels 
//通道值 0~16取值范围为：ADC_Channel_0~ADC_Channel_16
//返回值:转换结果
u16 Get_Adc(u8 ch)   
{
	  	//设置指定ADC的规则组通道，一个序列，采样时间
	ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_480Cycles );	//ADC1,ADC通道,480个周期,提高采样时间可以提高精确度			    
  
	ADC_SoftwareStartConv(ADC1);		//使能指定的ADC1的软件转换启动功能	
	 
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));//等待转换结束

	return ADC_GetConversionValue(ADC1);	//返回最近一次ADC1规则组的转换结果
}


//获取通道ch的转换值，取times次,然后平均 
//ch:通道编号
//times:获取次数
//返回值:通道ch的times次转换结果平均值
u16 Get_Adc_Average(u8 ch,u8 times)
{
	u32 temp_val=0;
	u8 t;
	for(t=0;t<times;t++)
	{
		temp_val+=Get_Adc(ch);
		delay_ms(5);
	}
	return temp_val/times;
}

void TIM3_Int_Init_2(u16 arr,u16 psc)   //通用定时器3初始化
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;  
    NVIC_InitTypeDef NVIC_InitStructure;
 
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);   //使能计时器的APB1时钟
 
    TIM_TimeBaseInitStructure.TIM_Period = arr; //自动重装载值
	
    TIM_TimeBaseInitStructure.TIM_Prescaler=psc;   //定时器分频
    TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //上溢
    TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;
 
    TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStructure);  //初始化TIM3
 
    TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);        //使用更新中断
    TIM_Cmd(TIM3,ENABLE);                           //使能定时器
 
    NVIC_InitStructure.NVIC_IRQChannel=TIM3_IRQn;   //定时器三中断
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x01; //中断抢占优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03;    //子优先级
    NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}




void TIM3_IRQHandler(void)  // 定时器中断服务函数
{
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET) //溢出，中断
	{
		//u16 x = 1;
		u16 tmp = Get_Adc(5);
		adc_get[cnt_tim] = tmp;
		cnt_tim++;
		if(cnt_tim >= 1024){
			tim_1024_judge = 1;
			cnt_tim=0;
		}
		
	}
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);  //重置计数器
}


void wave_form(void)
{
	
}


