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
/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
APP_userdata_t  USER_Data;
uint16_t grp;
uint32_t addr;
APP_SCHE_TIME_t user_schedule[30];
uint8_t len = 0;

/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
bool    Check_Address(void);
int Exist_Schedule_Pos(void);
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
  uint8_t sche_id;
  APP_TIME_t time_start;
  APP_TIME_t time_end;
  int pos = -1;
  bool is_addressed = FALSE;
  
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
          if (USER_Data.type == APP_DATA_FRAME && (APP_DeviceAddressed(&USER_Data)
                                                   || USER_Data.broadcast == TRUE))
          {
            APP_GetLocalAddress(&grp, &addr);
            
            if(USER_Data.group == grp)
            {
              sche_id = USER_Data.data[0];
              time_start.hour = USER_Data.data[1];
              time_start.min = USER_Data.data[2];
              time_start.sec = USER_Data.data[3];
              time_end.hour = USER_Data.data[4];
              time_end.min = USER_Data.data[5];
              time_end.sec = USER_Data.data[6];
              pos = Exist_Schedule_Pos();
              is_addressed = Check_Address();
              APP_Set_Schedule(user_schedule, &len);
              
              if(is_addressed == TRUE && pos > -1) // Update existing schedule
              {
                user_schedule[pos].time_start = time_start;
                user_schedule[pos].time_end = time_end;
                
                DH_ShowLED(A_LED_DATA, A_LED_FLASH);
              }
              else if (is_addressed == FALSE && pos > -1) // Remove existing schedule from this node
              {
                int i;
                for(i = pos; i < len-1; i++)
                  user_schedule[i] = user_schedule[i+1];
                len--;
                
                DH_ShowLED(A_LED_DATA, A_LED_FLASH);
              }
              else if(is_addressed == TRUE && pos == -1) // Add schedule to this node
              {
                APP_Set_Schedule(user_schedule, &len);
                user_schedule[len].sche_id = sche_id;
                user_schedule[len].time_start = time_start;
                user_schedule[len].time_end = time_end;
                len++;
                
                DH_ShowLED(A_LED_DATA, A_LED_FLASH);
              }
              else
                DH_ShowLED(A_LED_ERROR, A_LED_FLASH);
              
              APP_Get_Schedule(user_schedule, len);
            }
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

bool Check_Address()
{
  int k;
  bool is_sche_set = FALSE;
  u32 node_addr;
  
  for(k = 7; k < USER_Data.len; k += 4)
  {
    node_addr = (((u32)(APP_GetWord(USER_Data.data + k))) << 16) | (((u32)(APP_GetWord(USER_Data.data + (k+2)))) & 0x0ffff);
    if(addr == node_addr)
      is_sche_set = TRUE;
  }
  
  return is_sche_set;
}

int Exist_Schedule_Pos()
{
  uint8_t i;
  int pos = -1;
  uint8_t new_sche_id = USER_Data.data[1];
  APP_Set_Schedule(user_schedule, &len);
  
  for(i = 0; i < len; i++)
  {
    if(new_sche_id == user_schedule[i].sche_id)
      pos = i;
  }
  return pos;
}
/******************* (C) COPYRIGHT 2007 STMicroelectronics *****END OF FILE****/

