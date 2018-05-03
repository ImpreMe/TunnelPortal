/*
************************************************************************************************************************
*                                                          CRC
*                                                Cyclic Redundancy Check
*
* File     : CRC.h
* Encoding : UTF-8
* By       : SXG
* Version  : V1.0
* Date     : 2016.05.19
************************************************************************************************************************
*/

#ifndef __CRC_H__
#define __CRC_H__


uint8_t CRC8_PLC_Calc(const void* buf, uint32_t len);
uint8_t CRC8_PLC_Calc2(const void* buf, uint32_t len, uint8_t init_val);

uint8_t CRC8_CCITT_Calc(const void* buf, uint32_t len);
uint8_t CRC8_CCITT_Calc2(const void* buf, uint32_t len, uint8_t init_val);

uint16_t CRC16_IBM_Calc(const void* buf, uint32_t len);
uint16_t CRC16_IBM_Calc2(const void* buf, uint32_t len, uint16_t init_val);

uint32_t CRC32_IEEE_Calc(const void* buf, uint32_t len);
uint32_t CRC32_IEEE_Calc2(const void* buf, uint32_t len, uint32_t init_val);


#endif /* __CRC_H__ */
