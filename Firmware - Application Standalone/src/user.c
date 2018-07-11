/******************** (C) COPYRIGHT 2007 STMicroelectronics ********************
* File Name          : user.c
* Author             : Systems LAB - IMS (Industrial & Multisegment Sector)
* Date First Issued  : 03/07/2012
* Description        : User defined routines
********************************************************************************
* History:
* 03/07/2012: V0.1
********************************************************************************
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "user.h"
#include "dongle.h"
#include "application.h"
#include "interfaceconfig.h"

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  u16 GROUP;
  u32 *NODE_ADDRESSES;
}GROUP_info;
/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
APP_userdata_t  USER_Data;
u32 nodeAddrs[3] = {0x00000001, 0x00000002, 0x00000003};
u16 grp;
u32 addr;

/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : USER_Program
* Description    : User state machine
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USER_Program(void)
{
  static u8 USER_SM = 0xff;
  APP_userflag_t res;  
  
  switch (USER_SM)
  {
    case 0xff: // Init
      if (APP_ApplicationReady())
        USER_SM = 0;
      break;
      
    case 0: // Idle
      res = APP_ReceiveUserData(&USER_Data);
      //res = USER_DATA_ARRIVED;
      if (res == USER_DATA_ARRIVED)
      {
        if((USER_Data.source == SOURCE_COMM)) //Data/Error frame arrived from USART
        {
          if (APP_DeviceAddressed(&USER_Data) || USER_Data.broadcast == TRUE)
          {
            
          }
          else if (USER_Data.type == APP_ERROR_FRAME)
          {
            // Ciclic data - Error
            DH_ShowLED(A_LED_ERROR, A_LED_FLASH);
            // Set error frame
            USER_Data.type = APP_ERROR_FRAME;
            USER_Data.len = 2;
            USER_Data.data[0] = (u8)(APP_ERROR_COMMUNICATION >> 8);  // Error code MSB
            USER_Data.data[1] = (u8)(APP_ERROR_COMMUNICATION);       // Error code LSB  
            USER_SM = 1;
          }
          else
          {
            USER_SM = 1;
            DH_ShowLED(A_LED_DATA, A_LED_ON);
            DH_SetTimeout(20); // Set timeout to 20 sec
            USER_Data.source = SOURCE_PLM;
          }
        }
        else //Data/Error frame arrived from PLM
        {
          if(APP_DeviceAddressed(&USER_Data) && USER_Data.type == APP_DATA_FRAME)
          {
            
          }
          else
          {
            
          }
        }
      }
      else if (res == USER_DATA_COMMUNICATION_ERROR)
        DH_ShowLED(A_LED_ERROR, A_LED_FLASH);
      break;
      
    case 1: // Transmission Start
      res = APP_TransmitUserData(&USER_Data);
      if (res == USER_DATA_TRANSMISSION_END)
      {
        DH_ShowLED(A_LED_BOTH, A_LED_OFF);
        USER_SM = 0;
      }
      else if ((res == USER_DATA_COMMUNICATION_ERROR) || DH_TimeoutElapsed())
      {
        DH_ShowLED(A_LED_ERROR, A_LED_FLASH);
        USER_SM = 0;
      }
      break;
  }
}

/******************* (C) COPYRIGHT 2007 STMicroelectronics *****END OF FILE****/

