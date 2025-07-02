#include <REGX52.H>
#include "LCD1602.h"
#include "DS18B20.h"
#include "Delay.h"
#include "AT24C02.h"
#include "Key.h"
#include "Timer0.h"
#include "Buzzer.h"

sbit LED1=P2^0;	//定义LED D1，连接到P2.0引脚

//数码管位选信号定义（用于禁用数码管）
sbit LSA=P2^2;
sbit LSB=P2^3;
sbit LSC=P2^4;

float T,TShow;
char TLow,THigh;
unsigned char KeyNum;
unsigned char AlarmFlag = 0;		//报警标志位
unsigned int AlarmCount = 0;		//报警计数器

void main()
{
	//禁用数码管显示（防止数码管干扰LCD显示）
	LSA=1;
	LSB=1;
	LSC=1;	//设置为111，禁用所有数码管位选
	P0=0x00;//清零P0口，确保数码管不显示内容
	
	DS18B20_ConvertT();		//上电先转换一次温度，防止第一次读数据错误
	Delay(1000);			//等待转换完成
	THigh=AT24C02_ReadByte(0);	//读取温度阈值数据
	TLow=AT24C02_ReadByte(1);
	if(THigh>125 || TLow<-55 || THigh<=TLow)
	{
		THigh=20;			//如果阈值非法，则设为默认值
		TLow=15;
	}
	LCD_Init();
	LCD_ShowString(1,1,"T:");
	LCD_ShowString(2,1,"TH:");
	LCD_ShowString(2,9,"TL:");
	LCD_ShowSignedNum(2,4,THigh,3);
	LCD_ShowSignedNum(2,12,TLow,3);
	Timer0_Init();
	
	while(1)
	{
		KeyNum=Key();
		
		/*温度读取及显示*/
		DS18B20_ConvertT();	//转换温度
		T=DS18B20_ReadT();	//读取温度
		if(T<0)				//如果温度小于0
		{
			LCD_ShowChar(1,3,'-');	//显示负号
			TShow=-T;		//将温度变为正数
		}
		else				//如果温度大于等于0
		{
			LCD_ShowChar(1,3,'+');	//显示正号
			TShow=T;
		}
		LCD_ShowNum(1,4,TShow,3);		//显示温度整数部分
		LCD_ShowChar(1,7,'.');		//显示小数点
		LCD_ShowNum(1,8,(unsigned long)(TShow*100)%100,2);//显示温度小数部分
		
		/*阈值判断及显示*/
		if(KeyNum)
		{
			if(KeyNum==1)	//K1按键，THigh自增
			{
				THigh++;
				if(THigh>125){THigh=125;}
			}
			if(KeyNum==2)	//K2按键，THigh自减
			{
				THigh--;
				if(THigh<=TLow){THigh++;}
			}
			if(KeyNum==3)	//K3按键，TLow自增
			{
				TLow++;
				if(TLow>=THigh){TLow--;}
			}
			if(KeyNum==4)	//K4按键，TLow自减
			{
				TLow--;
				if(TLow<-55){TLow=-55;}
			}
			LCD_ShowSignedNum(2,4,THigh,3);	//显示阈值数据
			LCD_ShowSignedNum(2,12,TLow,3);
			AT24C02_WriteByte(0,THigh);		//写入到At24C02中保存
			Delay(5);
			AT24C02_WriteByte(1,TLow);
			Delay(5);
		}
		if(T>THigh)			//越界判断
		{
			LCD_ShowString(1,13,"OV:H");
			AlarmFlag = 1;		//设置高温报警标志
			LED1 = 1;			//高温时LED D1熄灭
		}
		else if(T<TLow)
		{
			LCD_ShowString(1,13,"OV:L");
			AlarmFlag = 0;		//低温不报警，清除报警标志
			LED1 = 0;			//低温时LED D1常亮
		}
		else
		{
			LCD_ShowString(1,13,"    ");
			AlarmFlag = 0;		//正常温度，清除报警标志
			LED1 = 1;			//正常温度时LED D1熄灭
		}
	}
}

void Timer0_Routine() interrupt 1
{
	static unsigned int T0Count;
	TL0 = 0x18;		//设置定时初值
	TH0 = 0xFC;		//设置定时初值
	T0Count++;
	if(T0Count>=20)
	{
		T0Count=0;
		Key_Loop();	//每20ms调用一次按键驱动函数
	}
	
	//蜂鸣器报警逻辑
	if(AlarmFlag)		//如果有高温报警
	{
		AlarmCount++;
		if(AlarmCount >= 500)	//每500ms响一次（500*1ms）
		{
			AlarmCount = 0;
			Buzzer_Time(200);	//蜂鸣器响200ms
		}
	}
	else
	{
		AlarmCount = 0;		//清除计数器
	}
}
