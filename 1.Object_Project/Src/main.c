#include "main.h"
#include "stdio.h"
#include "string.h"
#include "RCC\bsp_rcc.h"
#include "KEY_LED\bsp_key_led.h"
#include "LCD\bsp_lcd.h"


#include "UART\bsp_uart.h"
#include "IIC\bsp_iic.h"
#include "ADC\bsp_adc.h"
#include "TIM\bsp_tim.h"
#include "RTC\bsp_rtc.h"

// *** ���ٱ���������
__IO uint32_t KEY_msTick = 0;
__IO uint32_t LED_msTick = 0;
__IO uint32_t LCD_msTick = 0;
__IO uint32_t UART_msTick = 0;

// *** ������ر���
uint8_t KEY_Val, KEY_Down, KEY_Up, KEY_Old;


// *** LED��ر���
uint8_t ucled;


// *** LCD��ر���
uint8_t LCD_Str_Buf[25];



// *** ������ر���
uint16_t counter = 0;
uint8_t str[40];
uint8_t rx_buffer;


// *** 24C02��ر���
uint8_t str_24C02_Write_Buf[20] = {1, 2, 3, 4};
uint8_t str_24C02_Read_Buf[20];

// *** �ɱ������ر���
uint8_t R4017_Val = 0x77;

// *** ADC��ر���
//  ��





//*pwm��ر���
uint16_t PWM_T_Count;
uint16_t PWM_D_Count;
float PWM_Duty;


//*rtc��ر���
RTC_TimeTypeDef H_M_S_Time;
RTC_DateTypeDef Y_M_D_Date;




// *** ������
void KEY_Proc(void);
void LED_Proc(void);
void LCD_Proc(void);
void UART_Proc(void);

int main(void)
{
    HAL_Init();/* Reset of all peripherals, Initializes the Flash interface and the Systick. */

    SystemClock_Config();/* Configure the system clock */

    KEY_LED_GPIO_Init();
    LCD_Init();
    LCD_Clear(White);
    LCD_SetBackColor(White);
    LCD_SetTextColor(Blue); // �����ճ�ʼ�����


    // *** ���������ʼ�� *** //
    USART1_UART_Init();

    I2CInit();
    ADC1_Init();
    ADC2_Init();


    PWM_INPUT_TIM2_Init();
    PWM_OUTPUT_TIM3_Init();
    BASIC_TIM6_Init();
    SQU_OUTPUT_TIM15_Init();
    PWM_OUTPUT_TIM17_Init();


    MX_RTC_Init();

    //*���ڽ����жϴ�
    HAL_UART_Receive_IT(&huart1, (uint8_t *)(&rx_buffer), 1);


    //*�򿪻�����ʱ��
    HAL_TIM_Base_Start_IT(&htim6);//ÿ100ms����һ���ж�

    //*���벶��PWM����
    HAL_TIM_Base_Start(&htim2);  /* ������ʱ�� */
    HAL_TIM_IC_Start_IT(&htim2,TIM_CHANNEL_1);		  /* ������ʱ��ͨ�����벶�񲢿����ж� */
    HAL_TIM_IC_Start_IT(&htim2,TIM_CHANNEL_2);

    //*�������PA2����
    HAL_TIM_OC_Start_IT(&htim15,TIM_CHANNEL_1);

    //*������ʱ��3�Ͷ�ʱ��17ͨ�����
    HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_1);	//PA6
    HAL_TIM_PWM_Start(&htim17,TIM_CHANNEL_1);		//PA7

    while (1)
    {
        KEY_Proc();
        LED_Proc();
        LCD_Proc();
        UART_Proc();
    }

}



void KEY_Proc(void)
{
    if(uwTick - KEY_msTick < 15)	return ;
    KEY_msTick = uwTick;

    KEY_Val = KEY_Scan();
    KEY_Down = KEY_Val & (KEY_Val ^ KEY_Old);
    KEY_Up = ~KEY_Val & (KEY_Val ^ KEY_Old);
    KEY_Old = KEY_Val;

    switch(KEY_Down)
    {
    case 1:
    {
        ucled = 0x01;

        R4017_Val ++;
        str_24C02_Write_Buf[0]++;
        str_24C02_Write_Buf[1]++;
        str_24C02_Write_Buf[2]++;
        str_24C02_Write_Buf[3]++;
        break;
    }
    case 2:
    {
        ucled = 0x03;

        break;
    }
    case 3:
    {
        ucled = 0x07;
        iic_24c02_write(str_24C02_Write_Buf, 1, 4);
        write_resistor(R4017_Val);

        break;
    }
    case 4:
    {
        ucled = 0x0F;
        iic_24c02_read(str_24C02_Read_Buf, 1, 4);

        break;
    }
    default: // ����ؼ��־������ǣ���ǰ���ס
        break;
    }

}
void LED_Proc(void)
{
    if(uwTick - LED_msTick < 80)	return ;
    LED_msTick = uwTick;


    LED_Disp(ucled);
}

void LCD_Proc(void)
{
    if(uwTick - LCD_msTick < 150)	return ;
    LCD_msTick = uwTick;

    sprintf((char *)LCD_Str_Buf, "        %x", ucled);
    LCD_DisplayStringLine(Line0, LCD_Str_Buf);

    sprintf((char *)LCD_Str_Buf, "R24C02:%02x-%02x-%02x-%02x", str_24C02_Read_Buf[0], str_24C02_Read_Buf[1], str_24C02_Read_Buf[2], str_24C02_Read_Buf[3]);
    LCD_DisplayStringLine(Line1, LCD_Str_Buf);

    sprintf((char *)LCD_Str_Buf, "EEPROM-4017:%02x", read_resistor());
    LCD_DisplayStringLine(Line2, LCD_Str_Buf);

    //*ADC����
    sprintf((char *)LCD_Str_Buf, "R38_Vol:%6.3fV",((((float)getADC1())/4096)*3.3));
    LCD_DisplayStringLine(Line3, LCD_Str_Buf);

    sprintf((char *)LCD_Str_Buf, "R37_Vol:%6.3fV",((((float)getADC2())/4096)*3.3));
    LCD_DisplayStringLine(Line4, LCD_Str_Buf);



    //*PWM���벶����ԣ�����ռ�ձȺ�Ƶ��
    sprintf((char *)LCD_Str_Buf, "R40P:%05dHz,%4.1f%%",(unsigned int)(1000000/PWM_T_Count),PWM_Duty*100);
    LCD_DisplayStringLine(Line5, LCD_Str_Buf);




    //*RTC������ʾ
    HAL_RTC_GetTime(&hrtc, &H_M_S_Time, RTC_FORMAT_BIN);//��ȡ���ں�ʱ�����ͬʱʹ��
    HAL_RTC_GetDate(&hrtc, &Y_M_D_Date, RTC_FORMAT_BIN);
    sprintf((char *)LCD_Str_Buf, "Time:%02d-%02d-%02d",(unsigned int)H_M_S_Time.Hours,(unsigned int)H_M_S_Time.Minutes,(unsigned int)H_M_S_Time.Seconds);
    LCD_DisplayStringLine(Line6, LCD_Str_Buf);
}


void UART_Proc(void)
{
    if(uwTick - UART_msTick < 200)	return ;
    UART_msTick = uwTick;


//	sprintf((char *)str, "Num=%04d\r\n", counter);
//	HAL_UART_Transmit(&huart1, str, strlen(str), 50);
//
//	if(++ counter >= 999)	counter = 0;
}





//���ڽ����жϻص�����
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    counter++;
    sprintf(str, "%04d:Hello,world.\r\n", counter);
    HAL_UART_Transmit(&huart1,(unsigned char *)str, strlen(str), 50);

    HAL_UART_Receive_IT(&huart1, (uint8_t *)(&rx_buffer), 1);
}



void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM6)
    {
        if(++counter >= 10)
        {
            counter = 0;
            sprintf(str, "Hello,world.\r\n");
            HAL_UART_Transmit(&huart1,(unsigned char *)str, strlen(str), 50);
        }
    }

}


void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM15)
    {
        if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
            __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_1, (__HAL_TIM_GetCounter(htim)+500));
    }
}


void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{

    if(htim->Instance == TIM2)
    {
        if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
        {
            PWM_T_Count = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1) + 1;
            PWM_Duty = (float)PWM_D_Count/PWM_T_Count;
        }
        else if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
            PWM_D_Count = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2) + 1;

    }


}







/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}

