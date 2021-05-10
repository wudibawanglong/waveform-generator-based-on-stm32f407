#include "dac.h"



#define TIM2_CLK 84000000
#define dianshu 128
u16 DAC_Data[dot_count] = {0};
uint16_t g_Wave1[128];

/* 正弦波数据，12bit，1个周期128个点, 0-4095之间变化 */
const uint16_t g_SineWave128[] = {
	2047 ,  2147 ,	2248 ,	2347 ,	2446 ,	2544 ,	2641 ,	2737 ,
	2830 ,  2922 ,	3012 ,	3099 ,	3184 ,	3266 ,	3346 ,	3422 ,
	3494 ,  3564 ,	3629 ,	3691 ,	3749 ,	3803 ,	3852 ,	3897 ,
	3938 ,	3974 ,	4006 ,	4033 ,	4055 ,	4072 ,	4084 ,	4092 ,
	4094 ,	4092 ,	4084 ,	4072 ,	4055 ,	4033 ,	4006 ,	3974 ,
	3938 ,	3897 ,	3852 ,	3803 ,	3749 ,	3691 ,	3629 ,	3564 ,
	3494 ,	3422 ,	3346 ,	3266 ,	3184 ,	3099 ,	3012 ,	2922 ,
	2830 ,	2737 ,	2641 ,	2544 ,	2446 ,	2347 ,	2248 ,	2147 ,
	2047 ,	1947 ,	1846 ,	1747 ,	1648 ,	1550 ,	1453 ,	1357 ,
	1264 ,	1172 ,	1082 ,	995  ,	910  ,	828  ,	748  ,	672  ,
	600  ,	530  ,	465  ,	403  ,	345  ,	291  ,	242  ,	197  ,
	156  ,	120  ,	88   ,	61   ,	39   ,	22   ,	10   ,	2    ,
	0    ,	2    ,	10   ,	22   ,	39   ,	61   ,	88   ,	120  ,
	156  ,	197  ,	242  ,	291  ,	345  ,	403  ,  465  ,	530  ,
	600  ,	672  ,	748  ,	828  ,	910  ,	995  ,	1082 ,	1172 ,
	1264 ,	1357 ,	1453 ,	1550 ,	1648 ,	1747 ,	1846 ,	1947
};

/* 64点Sin数据 */
const uint16_t g_SineWave64[] = {
	2048, 2252, 2454, 2652, 2844, 3027, 3202, 3364, 
	3514, 3649, 3768, 3870, 3954, 4019, 4065, 4090, 
	4095, 4080, 4045, 3989, 3915, 3822, 3711, 3584, 
	3441, 3285, 3116, 2937, 2748, 2553, 2353, 2150, 
	1946, 1743, 1543, 1348, 1159, 980, 811, 655, 512, 
	385, 274, 181, 107, 51, 16, 1, 6, 31, 77, 142, 
	226, 328, 447, 582, 732, 894, 1069, 1252, 1444, 
	1642, 1844, 2048
};



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
 * @brief		DAC初始化函数
 * 
 * @param		none
 * @return	none 
 */
void bsp_InitDAC1(void)
{	
	/* 配置GPIO */
	{
		GPIO_InitTypeDef GPIO_InitStructure;
		
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
		
		/* 配置DAC引脚为模拟模式  PA4 / DAC_OUT1 */
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
		GPIO_Init(GPIOA, &GPIO_InitStructure);
	}	

	/* DAC通道1配置 */
	{
		DAC_InitTypeDef DAC_InitStructure;
		
		/* 使能DAC时钟 */
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);		

		DAC_InitStructure.DAC_Trigger = DAC_Trigger_None;	/* 选择软件触发, 软件修改DAC数据寄存器 */
		DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
		DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;
		//DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
		DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Disable;
		DAC_Init(DAC_Channel_1, &DAC_InitStructure);
		DAC_Cmd(DAC_Channel_1, ENABLE);
	}
}


/**
 * @brief		DMA初始化函数
 * 
 * @param		_BufAddr : DMA数据缓冲区地址
 *					_Count   : 缓冲区样本个数
 *					_DacFreq  : DAC样本更新频率
 *
 * @return	none 
 */
void dac1_InitForDMA(uint32_t _BufAddr, uint32_t _Count, uint32_t _DacFreq)
{	
	uint16_t usPeriod;
	uint16_t usPrescaler;
	uint32_t uiTIMxCLK;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	
	DMA_Cmd(DMA1_Stream5, DISABLE);
	DAC_DMACmd(DAC_Channel_1, DISABLE);
	TIM_Cmd(TIM6, DISABLE);
	

	/* TIM6配置 */
	{
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);

		uiTIMxCLK = SystemCoreClock / 2;
		
		if (_DacFreq < 100)
		{
			usPrescaler = 10000 - 1;						 /* 分频比 = 10000 */
			usPeriod =  (uiTIMxCLK / 10000) / _DacFreq  - 1; /* 自动重装的值 */
		}
		else if (_DacFreq < 3000)
		{
			usPrescaler = 100 - 1;							/* 分频比 = 100 */
			usPeriod =  (uiTIMxCLK / 100) / _DacFreq  - 1;	/* 自动重装的值 */
		}
		else	/* 大于4K的频率，无需分频 */
		{
			usPrescaler = 0;						/* 分频比 = 1 */
			usPeriod = uiTIMxCLK / _DacFreq - 1;	/* 自动重装的值 */
		}

		TIM_TimeBaseStructure.TIM_Period = usPeriod;
		TIM_TimeBaseStructure.TIM_Prescaler = usPrescaler;
		TIM_TimeBaseStructure.TIM_ClockDivision = 0;
		TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
		TIM_TimeBaseStructure.TIM_RepetitionCounter = 0x0000;		/* TIM1 和 TIM8 必须设置 */	

		TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);

		/* 选择TIM6做DAC的触发时钟 */
		TIM_SelectOutputTrigger(TIM6, TIM_TRGOSource_Update);
	}

	/* DAC通道1配置 */
	{
		DAC_InitTypeDef DAC_InitStructure;
		
		/* 使能DAC时钟 */
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);		

		DAC_InitStructure.DAC_Trigger = DAC_Trigger_T6_TRGO;
		DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
		DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;
		//DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
		DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Disable;
		DAC_Init(DAC_Channel_1, &DAC_InitStructure);
		DAC_Cmd(DAC_Channel_1, ENABLE);
	}

	/* DMA1_Stream5配置 */
	{
		DMA_InitTypeDef DMA_InitStructure;

		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

		/* 配置DMA1 Stream 5 channel 7用于DAC1 */
		DMA_InitStructure.DMA_Channel = DMA_Channel_7;
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&DAC->DHR12R1; 
		DMA_InitStructure.DMA_Memory0BaseAddr = _BufAddr;	
		DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;		
		DMA_InitStructure.DMA_BufferSize = _Count;
		DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
		DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
		DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
		DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_HalfWord;
		DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;								//循环模式
		DMA_InitStructure.DMA_Priority = DMA_Priority_High;
		DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
		DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
		DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
		DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
		DMA_Init(DMA1_Stream5, &DMA_InitStructure);
		DMA_Cmd(DMA1_Stream5, ENABLE);

		/* 使能DAC通道1的DMA */
		DAC_DMACmd(DAC_Channel_1, ENABLE);
	}

	/* 使能定时器 */
	TIM_Cmd(TIM6, ENABLE);
}


/**
 * @brief		DAC1输出方波
 * 
 * @param		_low  : 低电平时DAC
 *					_high : 高电平时DAC
 *					_freq : 频率 Hz
 *					_duty : 占空比 2% - 98%, 调节步数 1%
 *
 * @return	none 
 */
void dac1_SetRectWave(uint16_t _low, uint16_t _high, uint32_t _freq, uint16_t _duty)
{	
	uint16_t i;
	//uint16_t g_Wave1[128];
	
	TIM_Cmd(TIM6, DISABLE);
	
	for (i = 0; i < (_duty * 128) / 100; i++)
	{
		g_Wave1[i] = _high;
	}
	for (; i < 128; i++)
	{
		g_Wave1[i] = _low;
	}
	
	dac1_InitForDMA((uint32_t)&g_Wave1, 128, _freq * 128);
}

/**
 * @brief		DAC1输出100kHz方波
 * 
 * @param		_low  : 低电平时DAC
 *					_high : 高电平时DAC
 *					_freq : 频率 Hz
 *					_duty : 占空比 2% - 98%, 调节步数 1%
 *
 * @return	none 
 */
void dac1_SetRectWave_100kHz(uint16_t _low, uint16_t _high, uint32_t _freq, uint16_t _duty)
{	
	uint16_t i;
	TIM_Cmd(TIM6, DISABLE);
	for (i = 0; i < (_duty * 64) / 100; i++)
	{
		g_Wave1[i] = _high;
	}
	for (; i < 64; i++)
	{
		g_Wave1[i] = _low;
	}
	dac1_InitForDMA((uint32_t)&g_Wave1, 64, _freq * 64);
}


/**
 * @brief		DAC1输出正弦波
 * 
 * @param		_vpp : 幅度 0-4095
 *					_freq : 频率
 * 
 * @return	none 
 */
void dac1_SetSinWave(uint16_t _vpp, uint32_t _freq)
{
	uint32_t i;
	uint32_t dac;
	
	TIM_Cmd(TIM6, DISABLE);
		
	/* 调整正弦波幅度 */		
	for (i = 0; i < 128; i++)
	{
		dac = (g_SineWave128[i] * _vpp) / 4095;
		if (dac > 4095)
		{
			dac = 4095;	
		}
		g_Wave1[i] = dac;
	}
	
	dac1_InitForDMA((uint32_t)&g_Wave1, 128, _freq * 128);
}

/**
 * @brief		DAC1输出100kHz正弦波
 * 
 * @param		_vpp : 幅度 0-4095
 *					_freq : 频率
 * 
 * @return	none 
 */
void dac1_SetSinWave_100kHz(uint16_t _vpp, uint32_t _freq)
{
	uint32_t i;
	uint32_t dac;
	
	TIM_Cmd(TIM6, DISABLE);
		
	/* 调整正弦波幅度 */		
	for (i = 0; i < 64; i++)
	{
		dac = (g_SineWave64[i] * _vpp) / 4095;
		if (dac > 4095)
		{
			dac = 4095;	
		}
		g_Wave1[i] = dac;
	}
	dac1_InitForDMA((uint32_t)&g_Wave1, 64, _freq * 64);
}


/**
 * @brief		DAC1输出三角波
 * 
 * @param		_low : 低电平时DAC
 *					_high : 高电平时DAC
 *					_freq : 频率 Hz
 *					_duty : 占空比
 *
 * @return	none 
 */
void dac1_SetTriWave(uint16_t _low, uint16_t _high, uint32_t _freq, uint16_t _duty)
{	
	uint32_t i;
	uint16_t dac;
	uint16_t m;
	
	TIM_Cmd(TIM6, DISABLE);
		
	/* 构造三角波数组，128个样本，从 _low 到 _high */		
	m = (_duty * 128) / 100;
	
	if (m == 0)
	{
		m = 1;
	}
	
	if (m > 127)
	{
		m = 127;
	}
	for (i = 0; i < m; i++)
	{
		dac = _low + ((_high - _low) * i) / m;
		g_Wave1[i] = dac;
	}
	for (; i < 128; i++)
	{
		dac = _high - ((_high - _low) * (i - m)) / (128 - m);
		g_Wave1[i] = dac;
	}	
	
	dac1_InitForDMA((uint32_t)&g_Wave1, 128, _freq * 128);
}

/**
 * @brief		DAC1输出100kHz三角波
 * 
 * @param		_low : 低电平时DAC
 *					_high : 高电平时DAC
 *					_freq : 频率 Hz
 *					_duty : 占空比
 *
 * @return	none 
 */
void dac1_SetTriWave_100kHz(uint16_t _low, uint16_t _high, uint32_t _freq, uint16_t _duty)
{	
	uint32_t i;
	uint16_t dac;
	uint16_t m;
	
	TIM_Cmd(TIM6, DISABLE);
		
	/* 构造三角波数组，64个样本，从 _low 到 _high */		
	m = (_duty * 64) / 100;
	
	if (m == 0)
	{
		m = 1;
	}
	
	if (m > 63)
	{
		m = 63;
	}
	for (i = 0; i < m; i++)
	{
		dac = _low + ((_high - _low) * i) / m;
		g_Wave1[i] = dac;
	}
	for (; i < 64; i++)
	{
		dac = _high - ((_high - _low) * (i - m)) / (64 - m);
		g_Wave1[i] = dac;
	}	
	
	dac1_InitForDMA((uint32_t)&g_Wave1, 64, _freq * 64);
}


/**
 * @brief 	在屏幕上显示输出的波形
 * 
 * @param		none
 * @return	none 
 */
void Show_Wave(void)
{
	LCD_Fast_DrawPoint(23, 23, YELLOW);
	int i;
	for(i = 0; i < 128; i++){  // 横坐标
		LCD_Fast_DrawPoint(i*240/dianshu, 220-g_Wave1[i]*200/4095-10, YELLOW);
	}
}






/**************************************下面的代码没有用***************************************************************************/













//定时器2初始化
//arr：自动重装值。
//psc：时钟预分频数
//定时器溢出时间计算方法:Tout=((arr+1)*(psc+1))/Ft us.
//Ft=定时器工作频率,单位:Mhz
void TIM2_Int_Init(u32 arr,u32 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);//使能TIM2时钟
	
  TIM_TimeBaseInitStructure.TIM_Period = arr;//自动重装载值
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;//定时器分频
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up;//向上计数模式
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStructure);//初始化TIM2
	
	TIM_SelectOutputTrigger(TIM2, TIM_TRGOSource_Update);//输出触发信号
	TIM_Cmd(TIM2,ENABLE);//使能定时器2
}
 
//DAC通道1输出初始化
void Dac1_Init(void)
{  
	//定义结构体
  GPIO_InitTypeDef  GPIO_InitStructure;
	DAC_InitTypeDef DAC_InitType;
	DMA_InitTypeDef DMA_InitStructure;	
	
	//使能时钟
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//使能GPIOA时钟
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);//使能DAC时钟
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);//使能DMA1时钟
	   
	//模拟模式
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;//PA4
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;//模拟输入
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//悬空
  GPIO_Init(GPIOA, &GPIO_InitStructure);//PA4初始化

	//DAC配置
	DAC_InitType.DAC_Trigger=DAC_Trigger_T2_TRGO;//选择触发方式，定时器2触发
	DAC_InitType.DAC_WaveGeneration=DAC_WaveGeneration_None;//不使用波形发生
	DAC_InitType.DAC_LFSRUnmask_TriangleAmplitude=DAC_LFSRUnmask_Bit0;//屏蔽、幅值设置
	DAC_InitType.DAC_OutputBuffer=DAC_OutputBuffer_Disable ;//DAC1输出缓存关闭 BOFF1=1
  DAC_Init(DAC_Channel_1,&DAC_InitType);//初始化DAC通道1	
	DAC_Cmd(DAC_Channel_1, ENABLE);//使能DMA
	
	//DMA配置
  //DMA1_Stream5
	DMA_DeInit(DMA1_Stream5);
	//通道选择
  DMA_InitStructure.DMA_Channel = DMA_Channel_7; 
	//DMA外设地址
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&DAC->DHR12R1;	
	//DMA存储器0地址
	DMA_InitStructure.DMA_Memory0BaseAddr = (u32)DAC_Data;  
  //DMA传输方向---存储器到外设模式
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;	
	//设置DMA在传输时缓冲区的长度  
	DMA_InitStructure.DMA_BufferSize = dot_count;
	//外设地址寄存器不变
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  //内存地址寄存器递增  
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; 
  //外设数据字长 
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; 
  //内存数据字长
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;	
	//工作在循环缓存模式 
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  //设置DMA为高优先级别
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  //指定如果FIFO模式或直接模式将用于指定的流:不使能FIFO模式
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;  
  //指定FIFO阈值水平 
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
  //指定的Burst转移配置内存传输  
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  //指定的Burst转移配置外围转移  
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;  
	//配置DMA的通道   
	DMA_Init(DMA1_Stream5, &DMA_InitStructure);
	//使能DMA
  DMA_Cmd(DMA1_Stream5, ENABLE);
	//使能ADC_DMA
  DAC_DMACmd(DAC_Channel_1, ENABLE);
}

//信号发生函数
//参数：类型--频率--峰值
void Signal_Generator(u16 wf_mode, u16 v_peak, u32 frequency)
{
	u16 i;
	u32 peak;
	u32 psc;
	u32 arr;
	float temp1;
	u16 temp2;
	
	//峰值
	peak = v_peak*4095/3300;//电压转换为通道值
	
	//频率
	frequency = frequency*dot_count;
	TIM_Cmd(TIM2, DISABLE);//失能TIM2
	if(frequency < 100)
	{
		psc = 9999;//分频比为10000
		arr = (TIM2_CLK/10000)/frequency-1;//自动装载值
	}	
	else if(frequency < 3000)
	{
		psc = 99;//分频比为100
		arr = (TIM2_CLK/100)/frequency-1;
	}
	else
	{
		psc = 0;//分频比为1
		arr = TIM2_CLK/frequency-1;
	}
	TIM_Cmd(TIM2, ENABLE);//使能TIM2
	TIM2_Int_Init(arr,psc);
	
	//波形
	switch(wf_mode)
	{
		//正弦波
		case sine:
		{
				temp1 = 2*3.141592/dot_count;
				for(i=0 ; i< dot_count; i++)
				{
					DAC_Data[i] = (u16)(0.5*peak*(sin(temp1 * i)+1));
				}
				break;
		}
		//三角波
		case triangle:
		{
			temp2 = dot_count/2;
			for(i=0; i<temp2; i++)//左半部分函数
			{
				DAC_Data[i] = (u16)(i*peak/temp2);
			}
			for(i=temp2; i<dot_count; i++)//右半部分函数
			{
				DAC_Data[i] = (u16)(peak-peak*(i-temp2)/temp2);
			}
			break;
		}
		//方波
		case square:
		{
			temp2 = dot_count/2;
			for(i=0; i<temp2; i++)
			{
				DAC_Data[i] = peak;
			}
			for(i=temp2; i<dot_count; i++)
			{
				DAC_Data[i] = 0;
			}
			break;
		}
		//锯齿波
		case sawtooth:
		{
			for(i=0 ; i<dot_count; i++)
			{
				DAC_Data[i] = (u16)(peak*i/dot_count); 
			}
			break;			
		}
		default:break;
	}
}
