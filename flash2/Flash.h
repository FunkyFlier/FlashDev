#ifndef FLASH_H
#define FLASH_H

#include <Arduino.h>

void FlashInit();

boolean FlashEraseBlock4k(uint16_t);//nonblocking
boolean FlashEraseBlock32k(uint16_t);
boolean FlashEraseBlock64k(uint16_t);
boolean FlashEraseChip();//blocking

uint8_t GetStatusReg();
//boolean DeviceReadyToWrite();
boolean VerifyWriteReady();
boolean CheckForSuccessfulWrite();

uint8_t FlashGetByte(uint16_t,uint8_t);
boolean FlashGetArray(uint16_t,uint8_t,uint8_t, uint8_t*);
boolean FlashGetPage(uint16_t,uint16_t,uint8_t*);

boolean FlashWriteByte(uint16_t,uint16_t, uint8_t);
boolean FlashWritePartialPage(uint16_t,uint8_t, uint16_t, uint8_t*);
boolean FlashWritePage(uint16_t, uint16_t, uint8_t*);

void DispStatRegs();

#endif
