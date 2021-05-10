/**
 * @project waveform generator
 * @author HEBUT Sirius
 * @brief 
 * @version 0.1
 * @date 2021-05-10
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "lcd.h"
#include "dac.h"
#include "key.h"
#include "fft.h"


/* 
	TIM6		DAC通道1
	TIM3		ADC采样定时器中断
	TIM4		扫频定时器中断
*/


/* 全局变量 */

u16 cnt = 0;  // 触摸屏计数

double THD;
double thd1,thd2,thd3,thd4,thd5=0;

u16 display_f_a_delay_cnt = 0;  // 频率幅值显示延时计数位
u16 wave_jugde = 0;  //当前发生的波形类型
long long Recent_A = 0;  // 当前幅值
long long Recent_Freq = 0;// 当前频率
u32 adc_get[1024];  // ADC捕获数组
float fft_inputbuf[FFT_LENGTH*2];	//FFT输入数组
float fft_outputbuf[FFT_LENGTH];	//FFT输出数组
u16 tim_1024_judge = 0;  // TIM采样完成
float32_t max = 0;
u32 max_index = 0;
float32_t second = 0;
u32 second_index = 0;
float32_t third = 0;
u32 third_index = 0;
u16 Freq_FFT;  // FFT测得的频率
float Vpp_Sin = 0, Vpp_tri = 0, Vpp_Squa =  0, VPP_ALL = 0;  // FFT 测得sin幅值
float32_t output_tmp[1024];  // FFT输出数据类型转换数组


/**
 * @brief 	主函数
 * 
 * @param		none
 * @return	none 
 */
int main(void)
{
	u16 i;
	delay_init(168); 			 // 初始化延时函数
	uart_init(115200);		 // 初始化串口波特率为115200
	BEEP_Init(); 					 // 蜂鸣器初始化
	beee(); 							 // 蜂鸣器
		printf("****\n");
	TIM4_Int_Init_2(420-1, 1000-1);  // 扫频中断初始化
		printf("****");
	TIM_Cmd(TIM4,DISABLE);  				 // 失能TIM4
		printf("****\n");
	Recent_A = 4095;								 // 初始化目前幅值
	Recent_Freq = 1000;							 // 初始化目前频率
	Adc_Init();										   // 初始化ADC
		printf("init");
	TIM3_Int_Init_2(42-1, 100-1);    // 初始化采样频率 Fs: 20kHz
		printf("init");
	arm_cfft_radix4_instance_f32 scfft;  // 初始化FFT
		printf("init");
	arm_cfft_radix4_init_f32(&scfft, FFT_LENGTH, 0, 1);  // 初始化scfft结构体，设定FFT相关参数    FFT运算 位反转
		printf("init");
	TFT_LCD_Init();										// 初始化TFT
	bsp_GUI_Init();										// 初始化界面
	
	while(1){
		/* 触摸屏处理 */
		tp_dev.scan(0);									// 扫描触摸屏
		if(tp_dev.sta&TP_PRES_DOWN)			// 触摸屏被按下
		{		
			if(tp_dev.sta&TP_PRES_DOWN)		// 触摸屏被按下
			{
				Button_App();								// 触摸屏按下动作
				cnt++;
				delay_ms(50);
			}
		}
		
		/* 按下预置键显示部分 */
		if(A_judge == 1 && Freq_judge == 0){
			LCD_ShowxNum(350,70,A_yuzhi,6,24,0XFE);
		}
		else if(Freq_judge == 1 && A_judge == 0){
			LCD_ShowxNum(350,10,Freq_yuzhi,6,24,0XFE);
		}
		else if(A_judge == 0 && Freq_judge == 0){
			LCD_ShowxNum(350,10,Recent_Freq,6,24,0XFE);
			LCD_ShowxNum(350,70,(((double)(Recent_A)/4095)*3.3*1000),6,24,0XFE);
		}
		
		/* 触摸屏被松开 */
		if(!(tp_dev.sta&TP_PRES_DOWN))//触摸屏被松开
		{		
			if(!(tp_dev.sta&TP_PRES_DOWN))//触摸屏被松开
			{
				cnt = 0;  // cnt置0
			}
		}
		Show_Wave();
		
		/* 扫频 */
		if(sweep_judge == 1) TIM_Cmd(TIM4,ENABLE);  // 使能
		else if(sweep_judge == 0) TIM_Cmd(TIM4,DISABLE);  // 失能
		
		/* FFT部分 */
		if(tim_1024_judge == 1){
			max = 0; second = 0; third = 0;
			tim_1024_judge = 0;
			TIM_Cmd(TIM3,DISABLE); //TIM3失能
			for(i=0;i<FFT_LENGTH;i++){  // 生成输入数组
				fft_inputbuf[2*i]=(float)adc_get[i]/4095;	//生成输入信号实部
				fft_inputbuf[2*i+1]=0;//虚部全部为0
			}
			
			arm_cfft_radix4_f32(&scfft,fft_inputbuf);	//FFT计算（基4）
			arm_cmplx_mag_f32(fft_inputbuf,fft_outputbuf,FFT_LENGTH);	//把运算结果复数求模得幅值 

			for(i = 0; i < 1024; i++){  // 类型转换
				output_tmp[i] = (float32_t)fft_outputbuf[i];
			}
			
			/* 求极值 */
			output_tmp[0] = 0;
			arm_max_f32(output_tmp, 1024, &max, &max_index);
			output_tmp[max_index] = 0;
			output_tmp[1024-max_index] = 0;
			arm_max_f32(output_tmp, 1024, &second, &second_index);
			output_tmp[second_index] = 0;
			output_tmp[1024-second_index] = 0;
			arm_max_f32(output_tmp, 1024, &third, &third_index);
			output_tmp[third_index] = 0;
			
			/* 求频率 */
			Freq_FFT = max_index*Fs/FFT_LENGTH;
			
			/* 转换成幅值 */
			if(wave_jugde == SIN){
				Vpp_Sin = (0.1+0.013*(double)max)*1000;
				VPP_ALL = Vpp_Sin;
				//printf("Vpp_Sin: %f", Vpp_Sin);
			}
			else if(wave_jugde == SQU){  // 方波
				Vpp_Squa = (0.38+0.01*(double)max)*1000;
				VPP_ALL = Vpp_Squa;
				//printf("Vpp_Squa: %f", Vpp_Squa);
			}
			else if(wave_jugde == TRI){  // 方波
				Vpp_tri = (0.31+0.017*(double)max)*1000;
				VPP_ALL = Vpp_tri;
				//printf("Vpp_tri: %f", Vpp_tri);
			}
			printf("\n\n\n");	 // 串口调试用
			/* 画频谱图 */
			for(i = 0; i < 1024; i++){
				LCD_DrawLine(i*310/1024, 670, i*310/1024, 670-output_tmp[i]);							//画线
			}
			LCD_Fill(0,400,310,670,BLACK);
			
			TIM_Cmd(TIM3,ENABLE);  // TIM3使能
		}// end of if(tim_1024_judge == 1)
		
		
		/* 采集的波形参数显示部分 */
		display_f_a_delay_cnt++;
		if(display_f_a_delay_cnt >= 17){
			display_f_a_delay_cnt = 0;
			LCD_ShowxNum(360,540,Freq_FFT,6,24,0XFE);
			LCD_ShowxNum(360,610,VPP_ALL*3,6,24,0XFE);
		}
		
	} // end of while(1)
} // end of main()

