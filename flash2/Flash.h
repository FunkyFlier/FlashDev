#ifndef FLASH_H
#define FLASH_H

#include <Arduino.h>

boolean FlashEraseBlock4k(uint16_t);//nonblocking
boolean FlashEraseBlock32k(uint16_t);
boolean FlashEraseBlock64k(uint16_t);
boolean FlashEraseChip();//blocking

boolean FlashCheckStatusReg();

uint8_t FlashGetByte(uint32_t);
void FlashGetArray(uint32_t,uint8_t, uint8_t);
void FlashGetPage(uint16_t,uint8_t);

void FlashWriteByte(uint32_t, uint8_t);
void FlashWritePartialPage(uint32_t, uint8_t, uint8_t);
void FlashWritePage(uint16_t, uint8_t);



#endif
