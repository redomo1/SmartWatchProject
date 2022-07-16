/*
 * main.c
 *
 *  Created on: Jan 22, 2022
 *      Author: Eng-Rehab Soliman
 */

#include"LIB/LSTD_TYPES.h"

#include"MCAL/MDIO/MDIO_Interface.h"
#include"MCAL/MADC/MADC_Interface.h"
#include"MCAL/MGIE/MGIE_Interface.h"
#include "HAL/HCLCD/HCLCD_Interface.h"

#include"Free_RTOS/FreeRTOS.h"
#include"Free_RTOS/task.h"
#include"Free_RTOS/semphr.h"
#include "Free_RTOS/queue.h"

#include"util/delay.h"
/*******************************************/

/*Macro Definitions*/
#define TIME_SET_BUTTON_PIN   PIN2
#define TIME_SET_BUTTON_PORT  MDIO_PORTD

#define INCREAMENT_BUTTON_PIN   PIN3
#define INCREAMENT_BUTTON_PORT  MDIO_PORTD

#define CONFIRM_BUTTON_PIN   PIN4
#define CONFIRM_BUTTON_PORT  MDIO_PORTD

#define FEAUTERS_BUTTON_PIN   PIN5
#define FEAUTERS_BUTTON_PORT  MDIO_PORTD

#define BUTTON_PRESSED              0
#define BUTTON_NOT_PRESSED          1

#define YEAR_INIT                  2021
#define MONTH_INIT                  8
#define DAY_INIT                    11
#define HOUR_INIT                   23
#define MINUTES_INIT                59
#define SECOND_INIT                 50
/*************************************/

/* Tasks */
void Control(void*pv);
void LCD_Display(void*pv);
void TimeSecIncreament(void*pv);
void Date_TimeSet(void*pv);
void LM35Read(void*pv);
void HeartRateRead(void*pv);
void TimeSet_Buttons(void*pv);
void ADC_SetNotification(void);
/************************************/

/*LCD Queues*/
xQueueHandle LCDDateQueue;
xQueueHandle LCDTimeQueue;
xQueueHandle LCDFeaturesQueue;
/***************************************/

/*Semaphore Creation*/
xSemaphoreHandle ADCSem;
xSemaphoreHandle LCDSem;
xSemaphoreHandle TimeSem;
/*********************************/

/*Global Variables Definitions*/
u16 u16Year = YEAR_INIT;
u16 u16Month = MONTH_INIT;
u16 u16Day = DAY_INIT;
u8 u8Hour = HOUR_INIT;
u8 u8Minutes = MINUTES_INIT;
u8 u8Second = SECOND_INIT;
static u8 Set_TimeFlag ;
static u8 IncreamentFlag;
static u8 counter;
u16 Inreament_count;
static u8 Features_DispFlag=0;
static u8 Update_DispFlag=0;
u8 u8Temperatur;
u8 u8HeartRate;
u8 u8Flag=0;

int main(void)
{
	/* Initialization*/
	HCLCD_Vid4Bits_Init();
	MDIO_Error_State_SetPinDirection(TIME_SET_BUTTON_PIN,TIME_SET_BUTTON_PORT,PIN_INPUT);
	MDIO_Error_State_SetPinDirection(INCREAMENT_BUTTON_PIN,INCREAMENT_BUTTON_PORT,PIN_INPUT);
	MDIO_Error_State_SetPinDirection(CONFIRM_BUTTON_PIN,CONFIRM_BUTTON_PORT,PIN_INPUT);
	MDIO_Error_State_SetPinDirection(FEAUTERS_BUTTON_PIN,FEAUTERS_BUTTON_PORT,PIN_INPUT);

	MDIO_Error_State_SetPinValue(TIME_SET_BUTTON_PIN,TIME_SET_BUTTON_PORT,PIN_HIGH);
	MDIO_Error_State_SetPinValue(INCREAMENT_BUTTON_PIN,INCREAMENT_BUTTON_PORT,PIN_HIGH);
	MDIO_Error_State_SetPinValue(CONFIRM_BUTTON_PIN,CONFIRM_BUTTON_PORT,PIN_HIGH);
	MDIO_Error_State_SetPinValue(FEAUTERS_BUTTON_PIN,FEAUTERS_BUTTON_PORT,PIN_HIGH);
	MDIO_Error_State_SetPinDirection(PIN1,MDIO_PORTB,PIN_OUTPUT);
	MDIO_Error_State_SetPinDirection(PIN0,MDIO_PORTB,PIN_OUTPUT);

	MDIO_Error_State_SetPinDirection(PIN0,MDIO_PORTC,PIN_OUTPUT);

	/*Write WELCOME On LCD*/
	HCLCD_VidWriteString_4Bits("WELCOME...");
	_delay_ms(1000);
	/*Clear LCD*/
	HCLCD_VidWriteCommand_4Bits(0x01>>4);
	HCLCD_VidWriteCommand_4Bits(0x01);
	/*ADC Initialization*/
	MADC_VidSetCallBack(ADC_SetNotification);
	MADC_VidADCInterruptEnable();
	MADC_VidInit();
	MADC_u16ADC_StartConversion_With_Interrupt(CHANNEL_0);


	/*enable GIE */
	MGIE_VidEnable();
	/*Semaphore Counting Creation */
	ADCSem = xSemaphoreCreateCounting(1,0);
	vSemaphoreCreateBinary(LCDSem);

	/* Tasks Create*/
	xTaskCreate(Control          ,NULL,100,NULL,0,NULL);
	xTaskCreate(TimeSecIncreament,NULL,100,NULL,0,NULL);
	xTaskCreate(LCD_Display      ,NULL,300,NULL,1,NULL);
	xTaskCreate(TimeSet_Buttons  ,NULL,100,NULL,2,NULL);
	xTaskCreate(Date_TimeSet     ,NULL,120,NULL,3,NULL);
	xTaskCreate(LM35Read         ,NULL,70 ,NULL,5,NULL);
	xTaskCreate(HeartRateRead    ,NULL,70 ,NULL,6,NULL);

	/*Queues Create*/
	LCDDateQueue=xQueueCreate(3,sizeof(u16));
	LCDTimeQueue=xQueueCreate(3,sizeof(u8));
	LCDFeaturesQueue=xQueueCreate(3,sizeof(u8));

	vTaskStartScheduler();
	while(1)
	{

	}
	return 0;
}
/*
 * Init LCD
 * Check Pressed Button
 * Send the current Time and Date to LCD
 * Display Featuers
 * Switch back the current time
 * */
void Control(void*pv)
{

	static u8 Init_flag=0;
	u8 Loc_u8SemState=0;

	while(1)
	{
		Loc_u8SemState=xSemaphoreTake(LCDSem,10);
		/*Enter In First Time */
		if(Init_flag==0)
		{
			xQueueSend(LCDDateQueue,&u16Year,0);
			xQueueSend(LCDDateQueue,&u16Month,0);
			xQueueSend(LCDDateQueue,&u16Day,0);
			xQueueSend(LCDTimeQueue,&u8Hour,0);
			xQueueSend(LCDTimeQueue,&u8Minutes,0);
			xQueueSend(LCDTimeQueue,&u8Second,0);
			xSemaphoreGive(LCDSem);
			Init_flag=1;
		}
		else
		{
			/*Do Nothing*/
		}


		if(Loc_u8SemState == pdPASS)
		{
			if((Features_DispFlag == 0)&& (Update_DispFlag==1))
			{
				/*Send Current Date to LCDDateQueue*/

				xQueueSend(LCDDateQueue,&u16Year,3);
				xQueueSend(LCDDateQueue,&u16Month,3);
				xQueueSend(LCDDateQueue,&u16Day,3);

				/*Send Current Time to LCDTimeQueue*/
				xQueueSend(LCDTimeQueue,&u8Hour,3);
				xQueueSend(LCDTimeQueue,&u8Minutes,3);
				xQueueSend(LCDTimeQueue,&u8Second,3);
				Init_flag =1;
				/* Give Semaphore*/
				xSemaphoreGive(LCDSem);

			}
			/*When Feature Display Button is Pressed*/
			else if(Features_DispFlag==1)
			{
				if(Init_flag==1)
				{
					HCLCD_VidWriteCommand_4Bits(0x01>>4);
					HCLCD_VidWriteCommand_4Bits(0x01);
					Init_flag=2;
				}
				/*Send Temperature and HeartRate to LCDFeauteresQueue*/

				xQueueSend(LCDFeaturesQueue,&u8HeartRate,3);
				xQueueSend(LCDFeaturesQueue,&u8Temperatur,3);
				/*Give Semaphore*/
				xSemaphoreGive(LCDSem);
			}
			else
			{
				/*Do Nothing*/
			}
		}
		vTaskDelay(80);
	}
}

void Date_TimeSet(void*pv)
{
	while(1)
	{
		/*When SetTimeButton is Pressed*/
		if(Set_TimeFlag == 1)
		{
			/*When IncreamentButton is Pressed*/
			if(IncreamentFlag == 1)
			{
				/*Check Confirm Button*/
				switch(counter)
				{
				case 0:
					u16Year++;
					break;
				case 1:
					u16Month++;
					if(u16Month > 12)
					{
						u16Month = 1;
					}
					break;


				case 2:
					u16Day++;

					if(u16Day > 30)
					{
						u16Day = 1;
					}

					break;
				case 3:
					u8Hour++;
					if(u8Hour >23)
					{
						u8Hour = 0;
					}

					break;
				case 4:
					u8Minutes++;

					if(u8Minutes == 60)
					{
						u8Minutes = 0;
					}
					break;
				case 5:
					u8Second++;

					if(u8Second == 60)
					{
						u8Second = 0;
					}
					/*Clear SetTimeFlag*/
					Set_TimeFlag = 0;
					/*Clear Counter*/
					counter = 0;
					break;
				default: break;

				}
				/*Clear IncreamentFlag*/
				IncreamentFlag = 0;
			}
		}

		vTaskDelay(100);
	}
}
void TimeSet_Buttons(void*pv)
{
	u8 Loc_u8TimeSetButtonVal = BUTTON_NOT_PRESSED;
	u8  Loc_u8IncreamentButtonVal = BUTTON_NOT_PRESSED;
	u8 Loc_u8ConfirmButtonVal = BUTTON_NOT_PRESSED;
	u8 Loc_u8FeaturesButtonVal = BUTTON_NOT_PRESSED;

	while(1)
	{
		MDIO_Error_State_GetPinValue(TIME_SET_BUTTON_PIN,TIME_SET_BUTTON_PORT,&Loc_u8TimeSetButtonVal);
		/*Check TimeSetButton*/
		if(Loc_u8TimeSetButtonVal == BUTTON_PRESSED)
		{
			Set_TimeFlag = 1;
			MDIO_Error_State_SetPinValue(PIN0,MDIO_PORTB,PIN_HIGH);

		}
		else
		{
			/*Do Nothing*/
		}
		MDIO_Error_State_GetPinValue(INCREAMENT_BUTTON_PIN,INCREAMENT_BUTTON_PORT,&Loc_u8IncreamentButtonVal);
		if(Loc_u8IncreamentButtonVal == BUTTON_PRESSED)
		{
			IncreamentFlag = 1;
		}
		else
		{
			/*Do Nothing*/
		}
		MDIO_Error_State_GetPinValue(CONFIRM_BUTTON_PIN,CONFIRM_BUTTON_PORT,&Loc_u8ConfirmButtonVal);
		if(Loc_u8ConfirmButtonVal == BUTTON_PRESSED)
		{
			Inreament_count = 0;
			counter++;
		}
		else
		{
			/*Do Nothing*/
		}

		MDIO_Error_State_GetPinValue(FEAUTERS_BUTTON_PIN,FEAUTERS_BUTTON_PORT,&Loc_u8FeaturesButtonVal);
		if(Loc_u8FeaturesButtonVal == BUTTON_PRESSED)
		{
			if(Features_DispFlag==0)
			{
				Features_DispFlag=1;
			}
			else if(Features_DispFlag==1)
			{
				Features_DispFlag=0;
			}
		}
		vTaskDelay(100);
	}
}

void LM35Read(void*pv)
{
	u8 LOC_u8SemState=0;
	static u16 Loc_u16ADC_DigitalVal;

	while(1)
	{
		MADC_u16ADC_StartConversion_With_Interrupt(CHANNEL_1);
		LOC_u8SemState = xSemaphoreTake(ADCSem,5);
		if(LOC_u8SemState == pdPASS)
		{
			Loc_u16ADC_DigitalVal = MADC_u16ADCRead();
			u8Temperatur=(u8)(((Loc_u16ADC_DigitalVal*5000UL)/1024)/10);

		}
		else
		{
			//Do Nothing
		}
		vTaskDelay(10);
	}
}
void HeartRateRead(void*pv)
{

	u8 LOC_u8SemState=0;
	static u16 Loc_u16ADC_DigitalVal;
	while(1)
	{
		MADC_u16ADC_StartConversion_With_Interrupt(CHANNEL_0);
		LOC_u8SemState = xSemaphoreTake(ADCSem,5);
		if(LOC_u8SemState == pdPASS)
		{
			Loc_u16ADC_DigitalVal = MADC_u16ADCRead();
			u8HeartRate=(u8)(((Loc_u16ADC_DigitalVal*5000UL)/1024)/3);
		}
		else
		{
			//Do Nothing
		}
		vTaskDelay(10);
	}
}
void TimeSecIncreament(void*pv)
{
	Update_DispFlag=1;
	while(1)
	{
		if(Set_TimeFlag==0)
		{
			u8Second++;
			if(u8Second > 59)
			{
				u8Second = 0;
				u8Minutes++;
				if(u8Minutes > 59)
				{
					u8Minutes = 0;
					u8Hour++;
					if(u8Hour>23)
					{
						u8Hour = 0;
						u16Day++;
						if(u16Day>30)
						{
							u16Month++;
							if(u16Month>12)
							{
								u16Year++;
							}
						}

					}
				}

			}
		}

		vTaskDelay(1000);
	}
}
void ADC_SetNotification(void)
{
	xSemaphoreGive(ADCSem);


}

void LCD_Display(void*pv)
{
	static u16 LOC_u16Year;
	static u16 LOC_u16Month;
	static u16 LOC_u16Day;
	static u8 LOC_u8Hour;
	static u8 LOC_u8Minutes;
	static u8 LOC_u8Second;
	static u8 LOC_u8Temperature;
	static u8 LOC_u8HeartRate;
	u8 Loc_u8SemState=0;


	while(1)
	{
		Loc_u8SemState=xSemaphoreTake(LCDSem,10);
		if(Loc_u8SemState==pdPASS)
		{
			xQueueReceive(LCDDateQueue,&LOC_u16Year,0);
			xQueueReceive(LCDDateQueue,&LOC_u16Month,0);
			xQueueReceive(LCDDateQueue,&LOC_u16Day,0);

			xQueueReceive(LCDTimeQueue,&LOC_u8Hour,0);
			xQueueReceive(LCDTimeQueue,&LOC_u8Minutes,0);
			xQueueReceive(LCDTimeQueue,&LOC_u8Second,0);

			/*Features*/
			if(Features_DispFlag==1){
				xQueueReceive(LCDFeaturesQueue,&LOC_u8Temperature,5);
				xQueueReceive(LCDFeaturesQueue,&LOC_u8HeartRate,5);
			}

			xSemaphoreGive(LCDSem);
		}
		else
		{

		}
		if(Features_DispFlag==0)
		{

			HCLCD_VidSetPosition_4BitsMode(HCLCD_LINE1,2);
			HCLCD_VidSendChar_4Bits('/');
			HCLCD_VidSetPosition_4BitsMode(HCLCD_LINE1,5);
			HCLCD_VidSendChar_4Bits('/');
			HCLCD_VidSetPosition_4BitsMode(HCLCD_LINE2,2);
			HCLCD_VidSendChar_4Bits(':');
			HCLCD_VidSetPosition_4BitsMode(HCLCD_LINE2,5);
			HCLCD_VidSendChar_4Bits(':');
			HCLCD_VidSetPosition_4BitsMode(HCLCD_LINE1,0);
			HCLCD_VidSendChar_4Bits((LOC_u16Day/10)+48);
			HCLCD_VidSendChar_4Bits((LOC_u16Day%10)+48);



			HCLCD_VidSetPosition_4BitsMode(HCLCD_LINE1,3);
			HCLCD_VidSendChar_4Bits((LOC_u16Month/10)+48);
			HCLCD_VidSendChar_4Bits((LOC_u16Month%10)+48);



			HCLCD_VidSetPosition_4BitsMode(HCLCD_LINE1,6);
			HCLCD_VidWriteNumber_4Bits(LOC_u16Year);
			HCLCD_VidSetPosition_4BitsMode(HCLCD_LINE1,10);
			HCLCD_VidWriteString_4Bits("       ");
			HCLCD_VidSetPosition_4BitsMode(HCLCD_LINE2,0);
			HCLCD_VidSendChar_4Bits((LOC_u8Hour/10)+48);
			HCLCD_VidSendChar_4Bits((LOC_u8Hour%10)+48);



			HCLCD_VidSetPosition_4BitsMode(HCLCD_LINE2,3);
			HCLCD_VidSendChar_4Bits((LOC_u8Minutes/10)+48);
			HCLCD_VidSendChar_4Bits((LOC_u8Minutes%10)+48);



			HCLCD_VidSetPosition_4BitsMode(HCLCD_LINE2,6);
			HCLCD_VidSendChar_4Bits((LOC_u8Second/10)+48);
			HCLCD_VidSendChar_4Bits((LOC_u8Second%10)+48);
			HCLCD_VidSetPosition_4BitsMode(HCLCD_LINE1,0);
			HCLCD_VidSetPosition_4BitsMode(HCLCD_LINE2,8);

			HCLCD_VidWriteString_4Bits("       ");
		}
		else if(Features_DispFlag==1)
		{

			HCLCD_VidSetPosition_4BitsMode(HCLCD_LINE1,0);
			HCLCD_VidWriteString_4Bits("Temp=");
			HCLCD_VidSetPosition_4BitsMode(HCLCD_LINE1,7);
			HCLCD_VidSendChar_4Bits((LOC_u8Temperature/10)+48);
			HCLCD_VidSendChar_4Bits((LOC_u8Temperature%10)+48);
			HCLCD_VidWriteString_4Bits("C   ");

			HCLCD_VidSetPosition_4BitsMode(HCLCD_LINE2,0);
			HCLCD_VidWriteString_4Bits("Hrate=");
			HCLCD_VidSetPosition_4BitsMode(HCLCD_LINE2,7);
			HCLCD_VidSendChar_4Bits((LOC_u8HeartRate/10)+48);
			HCLCD_VidSendChar_4Bits((LOC_u8HeartRate%10)+48);
			HCLCD_VidWriteString_4Bits(" Bpm ");
		}
		else
		{
			/*Do Nothing*/
		}
		vTaskDelay(100);
	}

}



