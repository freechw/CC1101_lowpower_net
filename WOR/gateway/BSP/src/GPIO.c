/* Includes ------------------------------------------------------------------*/
#include "GPIO.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
void EXGPIO_configuration(void)
{
    GPIO_Init(GPIOB, GPIO_Pin_2,GPIO_Mode_Out_PP_High_Fast);//״ָ̬ʾ
    
    GPIO_Init(GPIOB, GPIO_Pin_1,GPIO_Mode_In_FL_IT);//IO����
    EXTI_SetPinSensitivity(EXTI_Pin_1, EXTI_Trigger_Rising_Falling);
    ITC_SetSoftwarePriority(EXTI1_IRQn,ITC_PriorityLevel_1); 
    
    //LOW POWER SETTINGS
    GPIO_Init(GPIOC, GPIO_Pin_4,GPIO_Mode_Out_PP_High_Fast);
    GPIO_Init(GPIOC, GPIO_Pin_0,GPIO_Mode_Out_PP_High_Fast);
    GPIO_Init(GPIOC, GPIO_Pin_1,GPIO_Mode_Out_PP_High_Fast);
    GPIO_Init(GPIOB, GPIO_Pin_2,GPIO_Mode_Out_PP_High_Fast);
    GPIO_Init(GPIOB, GPIO_Pin_3,GPIO_Mode_Out_PP_High_Fast);  
    
    GPIO_ResetBits(GPIOB, GPIO_Pin_2);//��ʼ����״̬
    GPIO_SetBits(GPIOC, GPIO_Pin_4);
    GPIO_SetBits(GPIOC, GPIO_Pin_0); 
    GPIO_SetBits(GPIOC, GPIO_Pin_1); 
    GPIO_SetBits(GPIOB, GPIO_Pin_3); 
}

void STM8_SendBusyMode(void)//æµ״̬
{
    GPIO_SetBits(GPIOB, GPIO_Pin_2);
}

void STM8_SendIdleMode(void)//����״̬
{
    GPIO_ResetBits(GPIOB, GPIO_Pin_2);
}