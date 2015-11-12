#include "Flash.h"
#include "Defines.h"
#include "SPI.h"
#include <Streaming.h>

#define BLOCK_MASK_4K 0x000F
#define BLOCK_MASK_32K 0x00FF
#define BLOCK_MASK_64K 0x0FFF

void FlashInit(){
  Serial<<"1\r\n";
  FlashSSLow();
  SPI.transfer(WRITE_ENABLE);
  FlashSSHigh();
  Serial<<"2\r\n";
  FlashSSLow();
  SPI.transfer(STATUS_WRITE);
  SPI.transfer(0x00);
  FlashSSHigh();
  Serial<<"3\r\n";
  while(GetStatusReg() & 0x01 != 0){
    Serial<<GetStatusReg()<<"\r\n";
  } 
}

boolean FlashEraseBlock4k(uint16_t blockAddress){
  uint32_u addressOutput;
  if (blockAddress > 0x3FF0){
    return false;
  }

  if (blockAddress & BLOCK_MASK_4K == 0x0000){
    addressOutput.val = blockAddress << 8;
    FlashSSLow();
    SPI.transfer(WRITE_ENABLE);
    FlashSSHigh();
    FlashSSLow();
    SPI.transfer(ERASE_4K);
    SPI.transfer(addressOutput.buffer[2]);
    SPI.transfer(addressOutput.buffer[1]);
    SPI.transfer(addressOutput.buffer[0]);
    FlashSSHigh();
    return true;
  }
  else{
    return false;
  } 
}
boolean FlashEraseBlock32k(uint16_t blockAddress){
  uint32_u addressOutput;
  if (blockAddress > 0x3F80){
    return false;
  }

  if (blockAddress & BLOCK_MASK_32K == 0x0000 || blockAddress & BLOCK_MASK_32K == 0x0080){
    addressOutput.val = blockAddress << 8;
    FlashSSLow();
    SPI.transfer(WRITE_ENABLE);
    FlashSSHigh();
    FlashSSLow();
    SPI.transfer(ERASE_32K);
    SPI.transfer(addressOutput.buffer[2]);
    SPI.transfer(addressOutput.buffer[1]);
    SPI.transfer(addressOutput.buffer[0]);
    FlashSSHigh();
    return true;
  }
  else{
    return false;
  } 
}
boolean FlashEraseBlock64k(uint16_t blockAddress){
  uint32_u addressOutput;
  if (blockAddress > 0x3F00){
    return false;
  }

  if (blockAddress & BLOCK_MASK_64K == 0x0000){
    addressOutput.val = blockAddress << 8;
    FlashSSLow();
    SPI.transfer(WRITE_ENABLE);
    FlashSSHigh();
    FlashSSLow();
    SPI.transfer(ERASE_64K);
    SPI.transfer(addressOutput.buffer[2]);
    SPI.transfer(addressOutput.buffer[1]);
    SPI.transfer(addressOutput.buffer[0]);
    FlashSSHigh();
    return true;
  }
  else{
    return false;
  } 
}
boolean FlashEraseChip(){
  FlashSSLow();
  SPI.transfer(WRITE_ENABLE);
  FlashSSHigh();

  FlashSSLow();
  SPI.transfer(ERASE_CHIP);
  FlashSSHigh();

  while(GetStatusReg() & 0x01 != 0){
  } 

}


uint8_t GetStatusReg(){
  FlashSSLow();
  SPI.transfer(READ_STATUS_REG);
  return (SPI.transfer(0) & 0x03);
  FlashSSHigh(); 
  //return 0 device ready write not enabled
  //return 1 device busy wrtie not enabled
  //return 2 device ready write enabled 
  //return 3 device busy write enabled 
}


uint8_t FlashGetByte(uint16_t pageAddress, uint8_t byteAddress){
  uint32_u addressOutput;
  addressOutput.val == (pageAddress << 8) + byteAddress;
  FlashSSLow();
  SPI.transfer(READ_ARRAY);
  SPI.transfer(addressOutput.buffer[2]);
  SPI.transfer(addressOutput.buffer[1]);
  SPI.transfer(addressOutput.buffer[0]);
  return SPI.transfer(0);

  FlashSSHigh();

}
boolean FlashGetArray(uint16_t pageAddress, uint8_t byteAddress,uint16_t numBytes, uint8_t readBuffer[]){
  uint32_u addressOutput;
  if (sizeof(readBuffer) != numBytes){
    return false;
  }
  if (numBytes < (256 - byteAddress) ){
    return false;
  }
  addressOutput.val == (pageAddress << 8) + byteAddress;
  FlashSSLow();
  SPI.transfer(READ_ARRAY);
  SPI.transfer(addressOutput.buffer[2]);
  SPI.transfer(addressOutput.buffer[1]);
  SPI.transfer(addressOutput.buffer[0]);
  for(uint8_t i = 0; i < numBytes; i++){
    readBuffer[i] = SPI.transfer(0x00);
  }
  FlashSSHigh();
  return true;
}
boolean FlashGetPage(uint16_t pageAddress,uint8_t readBuffer[]){
  uint32_u addressOutput;
  if (sizeof(readBuffer) != 256){
    return false;
  }
  addressOutput.val == (pageAddress << 8);
  FlashSSLow();
  SPI.transfer(READ_ARRAY);
  SPI.transfer(addressOutput.buffer[2]);
  SPI.transfer(addressOutput.buffer[1]);
  SPI.transfer(addressOutput.buffer[0]);
  for(uint8_t i = 0; i < 256; i++){
    readBuffer[i] = SPI.transfer(0x00);
  }
  FlashSSHigh();
  return true;

}
void WriteEnable(){
  FlashSSLow();
  SPI.transfer(WRITE_ENABLE);
  FlashSSHigh();

}
boolean VerifyWriteReady(){
  uint8_t statusReg;
  statusReg = GetStatusReg();
  switch(statusReg){
  case 0://device ready write not enabled
    WriteEnable();
    return true;
    break;
  case 1://device busy wrtie not enabled
    return false;
    break;
  case 2://device ready write enabled
    return true;
    break;
  case 3://device busy write enabled 
    return false;
    break;
  default:
    return false;
    break;
  }
}
boolean FlashWriteByte(uint16_t pageAddress, uint8_t byteAddress, uint8_t writeByte){
  uint32_u addressOutput;

  if (VerifyWriteReady() == false){
    return false;
  }
  addressOutput.val = (pageAddress << 8) | byteAddress;
  FlashSSLow();
  SPI.transfer(PROGRAM_PAGE);
  SPI.transfer(addressOutput.buffer[2]);
  SPI.transfer(addressOutput.buffer[1]);
  SPI.transfer(addressOutput.buffer[0]);
  SPI.transfer(writeByte);
  FlashSSHigh();
  return true;
}
boolean FlashWritePartialPage(uint16_t pageAddress, uint8_t byteAddress, uint8_t numBytes, uint8_t writeBuffer[]){
  uint32_u addressOutput;

  if (VerifyWriteReady() == false){
    return false;
  }
  if (sizeof(writeBuffer) != numBytes){
    return false;
  }
  if (numBytes < (256 - byteAddress) ){
    return false;
  }
  addressOutput.val = (pageAddress << 8) | byteAddress;
  FlashSSLow();
  SPI.transfer(PROGRAM_PAGE);
  SPI.transfer(addressOutput.buffer[2]);
  SPI.transfer(addressOutput.buffer[1]);
  SPI.transfer(addressOutput.buffer[0]);
  for(uint8_t i = 0; i < numBytes; i++){
    SPI.transfer(writeBuffer[i]);
  }
  FlashSSHigh();
  return true;
}
boolean FlashWritePage(uint16_t pageAddress, uint8_t writeBuffer[]){
  uint32_u addressOutput;

  if (VerifyWriteReady() == false){
    return false;
  }
  Serial<<"sizeof write buffer "<<sizeof(writeBuffer)<<"\r\n";
  if (sizeof(writeBuffer) != 256){
    return false;
  }
  addressOutput.val = (pageAddress << 8);
  FlashSSLow();
  SPI.transfer(PROGRAM_PAGE);
  SPI.transfer(addressOutput.buffer[2]);
  SPI.transfer(addressOutput.buffer[1]);
  SPI.transfer(addressOutput.buffer[0]);
  for(uint8_t i = 0; i < 256; i++){
    SPI.transfer(writeBuffer[i]);
  }
  FlashSSHigh();
  return true;  
}

boolean DeviceReadyToWrite(){
  uint8_t statusRegister;
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







