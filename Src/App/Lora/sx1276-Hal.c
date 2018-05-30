/*
 * THE FOLLOWING FIRMWARE IS PROVIDED: (1) "AS IS" WITH NO WARRANTY; AND 
 * (2)TO ENABLE ACCESS TO CODING INFORMATION TO GUIDE AND FACILITATE CUSTOMER.
 * CONSEQUENTLY, SEMTECH SHALL NOT BE HELD LIABLE FOR ANY DIRECT, INDIRECT OR
 * CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE CONTENT
 * OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING INFORMATION
 * CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 * 
 * Copyright (C) SEMTECH S.A.
 */
/*! 
 * \file       sx1276-Hal.c
 * \brief      SX1276 Hardware Abstraction Layer
 *
 * \version    2.0.B2 
 * \date       Nov 21 2012
 * \author     Miguel Luis
 *
 * Last modified by Miguel Luis on Jun 19 2013
 */
#include <stdint.h>
#include <stdbool.h> 
#include "stm32f1xx_hal.h"
#include "bsp_spi.h"
#include "platform.h"

#if defined( USE_SX1276_RADIO )

#include "sx1276-Hal.h"


/*!
 * SX1276 RESET I/O definitions
 */
#define RESET_IOPORT                                GPIOC
#define RESET_PIN                                   GPIO_PIN_9


/*!
 * SX1276 SPI NSS I/O definitions
 */
#define NSS_IOPORT                                  GPIOB
#define NSS_PIN                                     GPIO_PIN_12


/*!
 * SX1276 DIO pins  I/O definitions
 */
#define DIO0_IOPORT                                 GPIOC
#define DIO0_PIN                                    GPIO_PIN_8

//#define DIO1_IOPORT                                 GPIOB
//#define DIO1_PIN                                    GPIO_Pin_8
//
//#define DIO2_IOPORT                                 GPIOA
//#define DIO2_PIN                                    GPIO_Pin_2
//
//#define DIO3_IOPORT                                 
//#define DIO3_PIN                                    RF_DIO3_PIN
//
//#define DIO4_IOPORT                                 
//#define DIO4_PIN                                    RF_DIO4_PIN
//
//#define DIO5_IOPORT                                 
//#define DIO5_PIN                                    RF_DIO5_PIN
//
//#define RXTX_IOPORT                                 
//#define RXTX_PIN                                    FEM_CTX_PIN



void SX1276InitIo( void )
{
    
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct;
    
	GPIO_InitStruct.Pin = NSS_PIN;  //cs
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(NSS_IOPORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(NSS_IOPORT, NSS_PIN, GPIO_PIN_SET);    

	GPIO_InitStruct.Pin = RESET_PIN;  //reset
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(RESET_IOPORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(RESET_IOPORT, RESET_PIN, GPIO_PIN_SET);  
    
	GPIO_InitStruct.Pin = DIO0_PIN;  //dio0
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(DIO0_IOPORT, &GPIO_InitStruct);
    
}

void SX1276SetReset( uint8_t state )
{

    if( state == RADIO_RESET_ON )
    {
        HAL_GPIO_WritePin(RESET_IOPORT, RESET_PIN, GPIO_PIN_RESET);  
    }
    else
    {
        HAL_GPIO_WritePin(RESET_IOPORT, RESET_PIN, GPIO_PIN_SET);  
    }
}

void SX1276Write( uint8_t addr, uint8_t data )
{
    SX1276WriteBuffer( addr, &data, 1 );
}

void SX1276Read( uint8_t addr, uint8_t *data )
{
    SX1276ReadBuffer( addr, data, 1 );
}

void SX1276WriteBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;
    //NSS = 0;
    HAL_GPIO_WritePin(NSS_IOPORT, NSS_PIN, GPIO_PIN_RESET);    

    SPI_SendByte( addr | 0x80 );
    for( i = 0; i < size; i++ )
    {
        SPI_SendByte( buffer[i] );
    }

    //NSS = 1;
    HAL_GPIO_WritePin(NSS_IOPORT, NSS_PIN, GPIO_PIN_SET);    
}

void SX1276ReadBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    //NSS = 0;
    HAL_GPIO_WritePin(NSS_IOPORT, NSS_PIN, GPIO_PIN_RESET);    

    SPI_SendByte( addr & 0x7F );

    for( i = 0; i < size; i++ )
    {
        buffer[i] = SPI_SendByte( 0xFF );
    }

    //NSS = 1;
    HAL_GPIO_WritePin(NSS_IOPORT, NSS_PIN, GPIO_PIN_SET);    
}

void SX1276WriteFifo( uint8_t *buffer, uint8_t size )
{
    SX1276WriteBuffer( 0, buffer, size );
}

void SX1276ReadFifo( uint8_t *buffer, uint8_t size )
{
    SX1276ReadBuffer( 0, buffer, size );
}

inline uint8_t SX1276ReadDio0( void )
{
    //return GPIO_ReadInputDataBit( DIO0_IOPORT, DIO0_PIN );
    return HAL_GPIO_ReadPin( DIO0_IOPORT, DIO0_PIN );
}

inline uint8_t SX1276ReadDio1( void )
{
    //return GPIO_ReadInputDataBit( DIO1_IOPORT, DIO1_PIN );
    return 0;
}

inline uint8_t SX1276ReadDio2( void )
{
    //return GPIO_ReadInputDataBit( DIO2_IOPORT, DIO2_PIN );
    return 0;
}

inline uint8_t SX1276ReadDio3( void )
{
    //return IoePinGet( RF_DIO3_PIN );
    return 0;
}

inline uint8_t SX1276ReadDio4( void )
{
    //return IoePinGet( RF_DIO4_PIN );
    return 0;
}

inline uint8_t SX1276ReadDio5( void )
{
    //return IoePinGet( RF_DIO5_PIN );
    return 0;
}

inline void SX1276WriteRxTx( uint8_t txEnable )
{
    if( txEnable != 0 )
    {
        //IoePinOn( FEM_CTX_PIN );
        //IoePinOff( FEM_CPS_PIN );
    }
    else
    {
        //IoePinOff( FEM_CTX_PIN );
        //IoePinOn( FEM_CPS_PIN );
    }
}

#endif // USE_SX1276_RADIO
