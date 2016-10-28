#include "includes.h"
/*
此头文件包含以下寄寄存及宏定义
  ADC_CR2 = ADC1_ALIGN_RIGHT; // A/D data rigth align
	ADC_CR1 = 0x00; // ADC clock=main clock/2=4MHZm,sigle converterm,disable convert
	ADC_CSR = 0x06; // choose channel 6
	bitADC1_CR1_ADON = 1;	//ADC_CR1 = 0x01; // enable AD start
	bitADC1_CR1_ADON = 1;
	bitADC1_CSR_EOC = 0;
*/
#include "stm8s_adc1.h"
/*
此头文件包含以下函数
ADC1_GetFlagStatus(ADC1_FLAG_EOC)
ADC1_GetConversionValue()
*/

#define RES_UPVALUE	1667              // 20k/12 上拉电阻/12
#define RESISTOR_TEMP_UPLIMIT 42	    //110C
#define RESISTOR_TEMP_LOWLIMIT 59339	//-50C

//-50~110
const uint tempTable[161]={
59339,55161,51305,47745,44456,41415,38603,36001,33592,31360,
29291,27373,25593,23940,22405,20978,19652,18418,17270,16201,
15205,14277,13411,12603,11850,11146,10488,9874,9299,8762,
8259,7788,7346,6933,6545,6182,5841,5521,5220,4938,4673,4423,
4189,3968,3760,3565,3380,3207,3043,2889,2743,2606,2476,2354,
2238,2129,2026,1928,1836,1748,1666,1587,1513,1443,1376,1313,
1253,1196,1142,1091,1042,996,952,911,871,833,798,763,731,
700,671,643,616,591,566,543,521,500,480,461,443,425,409,393,
377,363,349,336,323,311,299,288,277,267,257,248,239,230,222,
214,206,199,192,185,179,173,167,161,156,150,145,140,136,131,
127,123,119,115,111,107,104,101,98,94,91,89,86,83,81,78,76,
74,71,69,67,65,63,61,60,58,56,54,53,51,50,49,47,46,45,43,42
};
/*对温度阻值表数据以及上拉电阻分别/12，主要为了使 uint型数据可以装下各温度点的阻值*/

/*
ADC1模块初始化
无参数，无返回
*/
void _ADC_Initial(void)
{
	uchar i;

	ADC_CR2 = ADC1_ALIGN_RIGHT; // A/D data right align
	ADC_CR1 = 0x00; // ADC clock=main clock/2=4MHZm,sigle converterm,disable convert
	ADC_CSR = 0x06; // choose channel 6
	//ADC_TDRL = 0x20;
	bitADC1_CR1_ADON = 1;	//ADC_CR1 = 0x01; // enable AD start
	
	for(i=0;i<100;i++); // wait at least 7us
	for(i=0;i<100;i++); // wait at least 7us
	bitADC1_CR1_ADON = 1;
	bitADC1_CSR_EOC = 0;
}
/*
读取ADC并计算温度值
参数：无。
返回：无。（末位为小数部分，前面为整数部分。）
使用方法：引入两个外部变量，CurrentTemp、CorrectTemp。
其中CurrentTemp为计算所得的温度值，CorrectTemp为校正温度值。
*/
void _AD_temperature(void)
{
	int temp0;
	uchar i;
	uint	ResistorValue;
	static uchar sumTimes = 0;
	static uint	sum_AD_value = 0;
	static signed int CurrentTempBkp = 0,CurrentTempValue = 0;	
	if (ADC1_GetFlagStatus(ADC1_FLAG_EOC)== 0) return;//是否转换完成？
	sum_AD_value += ADC1_GetConversionValue();//累加AD值
	bitADC1_CR1_ADON = 1;
	if (++sumTimes >= 16)//累加16次
	{
		sumTimes = 0;
		sum_AD_value >>= 4;//均值滤波
    //判断是否超过检测限值，如果超过限值则可添加错误代码，指示NTC可能开路或短路，否则计算阻值。
    //温度下限
		if(sum_AD_value >= 1000)//NTC may Open circuit
		{
			ResistorValue = RESISTOR_TEMP_LOWLIMIT;
		}
		else
		{
			ResistorValue = (long)sum_AD_value*RES_UPVALUE/(1024-sum_AD_value);
		}
    //温度上限
		if (ResistorValue >= RESISTOR_TEMP_LOWLIMIT)//-50`c ,
		{
			ResistorValue = RESISTOR_TEMP_LOWLIMIT;
			CurrentTempValue = -500;
			//ErrorCode = 1;
		}
		else if (ResistorValue < RESISTOR_TEMP_UPLIMIT)
		{
			ResistorValue = RESISTOR_TEMP_UPLIMIT;
			CurrentTempValue = 1110;
			//ErrorCode = 2;
		}	
		else //正常范围
		{
      //ErrorCode = 0;
			for (i = 0;ResistorValue <= tempTable[i];i++)if(i > 200)break;//计算温度所属区间
			if(i == 0)
			{
				CurrentTempValue = -500;
			}
			else
			{
        //计算小数部分
				temp0 = (long)10*(tempTable[i-1]-ResistorValue)/(tempTable[i-1]-tempTable[i])+1;
        //计算整体部分
				CurrentTempValue = -500+(i-1)*10+temp0;
        //再次进行限幅确认，-50~110摄氏度
				if (CurrentTempValue <= -500)
				{
					CurrentTempValue = -500;
				}
				else
				{
					if(CurrentTempValue > 1110)
					{
						CurrentTempValue = 1110;
					}
				}
			}
		}
    //一阶滞后滤波
		CurrentTempBkp = (13*CurrentTempBkp + CurrentTempValue*3)>>4;//filter
		CurrentTemp = CurrentTempBkp;//+ CorrectTemp*10;
		//isUpdateDisplay = Yes;
	  //加入温度校正
		CurrentTemp += CorrectTemp;
		sum_AD_value = 0;
	}
}
