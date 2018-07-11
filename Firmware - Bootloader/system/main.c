/**
  ******************************************************************************
  * @file    main.c 
  * @author  IMS Systems Lab
  * @version V4.0
  * @date    08-April-2011
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */  

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f10x_flash.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define FLASH_BASE_ADDRESS      ((uint32_t)0x08000000)
#define BOOT_LOADER_SIZE        (0x1000)  /*  4 Kb (3 Kb Boot loader firmware -max-, 1 Kb boot loader data) */
#define IMAGE_MAX_SIZE          0xF000
#define BOOT_DATA_SIZE          (0x400)   /*  1 Kb */
#define FLASH_PAGE_SIZE         (0x400)   /*  1 Kb */
#define BOOT_DATA_ADDRESS       (FLASH_BASE_ADDRESS + BOOT_LOADER_SIZE - BOOT_DATA_SIZE)
#define ACTIVE_IMAGE_ADDRESS    (BOOT_DATA_ADDRESS)

#define FIRST_IMAGE_ADDRESS     (FLASH_BASE_ADDRESS + BOOT_LOADER_SIZE)
#define SECOND_IMAGE_ADDRESS    (FIRST_IMAGE_ADDRESS + IMAGE_MAX_SIZE)
#define DEFAULT_IMAGE_ADDRESS   FIRST_IMAGE_ADDRESS

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : main
* Description    : Main program 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
main(void)
{
  int *application_address;  
  int vector_table_address;
  
  /* Read the active image and default vector table address */
  application_address = (int *)ACTIVE_IMAGE_ADDRESS;
  
  /* Verify if the active image address is correct, otherwise point to default address */
  if ((*application_address != FIRST_IMAGE_ADDRESS) && (*application_address != SECOND_IMAGE_ADDRESS))
  {
    /* Unlock the Flash Program Erase controller */
    FLASH_Unlock();
    /* Erase the data page */
    while (FLASH_ErasePage(BOOT_DATA_ADDRESS) != FLASH_COMPLETE);
    /* Write in the active image address pointer the address of the default boot image, image 1 */
    while (FLASH_ProgramWord(ACTIVE_IMAGE_ADDRESS, DEFAULT_IMAGE_ADDRESS) != FLASH_COMPLETE);
    
    /* Verify the writing of correct address and set the active vector table offset */
    application_address = (int *)ACTIVE_IMAGE_ADDRESS;
    if (*application_address != DEFAULT_IMAGE_ADDRESS)
      while(1); /* Fatal error! */
  }

  /* Set the correct offsets before jump to new image */
  vector_table_address = *application_address;
  application_address = (int *)(vector_table_address + 4);
    
  /* Jump to application */
  __set_MSP(*(int*)vector_table_address);
  *(int *)(0xE000ED08) = vector_table_address;     /* SCB->VTOR */
  ((void (*)(void))(*application_address))();
  while(1);
}


/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
