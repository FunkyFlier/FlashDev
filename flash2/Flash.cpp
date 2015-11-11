#include "Flash.h"

/*boolean FlashEraseBlock4k(uint16_t);//nonblocking
boolean FlashEraseBlock32k(uint16_t);
boolean FlashEraseBlock64k(uint16_t);
void FlashEraseChip();//blocking

boolean FlashCheckStatusReg();

uint8_t FlashGetByte(uint32_t);
void FlashGetArray(uint32_t,uint8_t, unt8_t);
void FlashGetPage(uint16_t,uint8_t);

void FlashWriteByte(uint32_t, uint8_t);
void FlashWritePartialPage(uint32_t, uint8_t, uint8_t);
void FlashWritePage(uint16_t, uint8_t);
*/

#define 4K_BLOCK_MASK 0x000F
#define 32K_BLOCK_MASK 0x00FF
#define 64K_BLOCK_MASK 0x0FFF
boolean FlashEraseBlock4k(uint16_t blockAddress){
  //returns false on invalid block address
    
}
void FlashEraseBlock32k(uint16_t blockAddress){
}
void FlashEraseBlock64k(uint16_t blockAddress){
}
boolean FlashEraseChip(){
  FlashSSLow();
  SPI.transfer(WRITE_ENABLE);
  FlashSSHigh();

  FlashSSLow();
  SPI.transfer(ERASE_CHIP);
  FlashSSHigh();

  while(FlashCheckStatusReg() == false){
  } 

}

boolean FlashCheckStatusReg(){

  FlashSSLow();
  SPI.transfer(READ_STATUS_REG);
  statusRegister = SPI.transfer(0);
  FlashSSHigh();  

  if ((statusRegister & 0x01) == 0x01){
    return false;
  }
  else{
    return true;
  }

}

uint8_t FlashGetByte(uint32_t byteAddress){
}
void FlashGetArray(uint32_t byteAddress,uint8_t numBytes, uint8_t readBuffer[]){
}
void FlashGetPage(uint16_t pageAddress,uint8_t readBuffer[]){
}

void FlashWriteByte(uint32_t byteAddress, uint8_t writeByte);
void FlashWritePartialPage(uint32_t byteAddress, uint8_t numBytes, uint8_t writeBuffer[]);
void FlashWritePage(uint16_t pageAddress, uint8_t writeBuffer[]);
