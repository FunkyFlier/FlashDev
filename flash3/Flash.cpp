#include "Flash.h"
#include "Defines.h"
#include "SPI.h"
#include <Streaming.h>
#include <Arduino.h> 

#define BLOCK_MASK_4K 0x000F
#define BLOCK_MASK_32K 0x007F
#define BLOCK_MASK_64K 0x00FF
#define LOG_RATE 100

enum LOG_STATES{
  ASSEMBLY_WRITE_BUFFER,
  CHECK_4K_READY,
  ERASE4K

};

uint16_t currentRecordNumber, currentRecordAddress, currentPageAddress, lowestRecordNumber, lowestRecordAddress;
uint32_u currentTime;
uint8_t writeBuffer[256];
boolean startLogging = false,endLogging = false;


void LoggingStateMachine(){
  static uint16_t nextBlockAddress = 0;
  static uint8_t pageCount = 0;
  static uint8_t loggingState = ASSEMBLY_WRITE_BUFFER;
  
  switch(loggingState){
  case ASSEMBLY_WRITE_BUFFER:
    if(VerifyWriteReady() == false){
      break;
    }
    if (endLogging == true && startLogging == false){
      if (currentPageAddress ==0){
        outInt16.val = 0x3FFF; 
      }else{
        outInt16.val = currentPageAddress - 1;
      }
      writeBuffer[0] = WRITE_COMPLETE_REC_END;
      writeBuffer[1] = 0xFF;
      writeBuffer[2] = 0xFF;
      writeBuffer[3] = 0xFF;
      writeBuffer[4] = 0x00;
      writeBuffer[5] = outInt16.buffer[0];
      writeBuffer[6] = outInt16.buffer[1];
      FlashWriteByteBlocking(currentPageAddress,    0,WRITE_COMPLETE_REC_END);
      loggingState = COMPLETE_RECORD;
      break;
    }
    LogBuilder();
    if ((currentPageAddress & 0x000F) == 0x000F){
      loggingState = CHECK_4K_READY;
      pageCount = 0;
      nextBlockAddress = (currentPageAddress & 0xFFF0) + 0x0010;
      if (nextBlockAddress > 0x3FF0){
        nextBlockAddress = 0;
      }
    }
    break;
  case COMPLETE_RECORD:
    if(VerifyWriteReady() == false){
      break;
    }
    writeBuffer[0] = 0;
    writeBuffer[1] = outInt16.buffer[0]);
    writeBuffer[2] = outInt16.buffer[1]);
    FlashWritePartialPage(currentRecordAddress,4,3,writeBuffer);
    currentRecordAddress += 1;
    
  break;
  case CHECK_4K_READY:
    if(VerifyWriteReady() == false){
      break;
    }
    if (FlashGetByte((nextBlockAddress + pageCount),0) != 0xFF){

      loggingState = ERASE4K;
      break;
    }
    pageCount++;
    if(pageCount == 16){
      loggingState = ASSEMBLY_WRITE_BUFFER;
    }
    break;
  case ERASE4K:
    if(VerifyWriteReady() == false){
      break;
    }
    FlashEraseBlock4k(nextBlockAddress);
    break;
  }
}

void LogBuilder(){
  static uint8_t logIndex = 0;
  uint16_u outInt16;
  if (millis() - currentTime >= LOG_RATE){
    currentTime = millis();
    if (startLogging == true){//start of log
      startLogging = false;
      logIndex = 0;
      outInt16.val = currentRecordNumber;
      writeBuffer[logIndex++] = 0x7F;
      writeBuffer[logIndex++] = 0xAA;
      writeBuffer[logIndex++] = outInt16.buffer[0];
      writeBuffer[logIndex++] = outInt16.buffer[1];
      writeBuffer[logIndex++] = 0xFF;
      writeBuffer[logIndex++] = 0xFF;
      writeBuffer[logIndex++] = 0xFF;
      writeBuffer[logIndex++] = 0x55;
    }
    /*if (endLogging == true){
      if(logIndex == 0){
        currentPageAddress -= 1;
      }
      outInt16.val = currentPageAddress;
      writeBuffer[0] = WRITE_COMPLETE_REC_END;
      writeBuffer[1] = 0xFF;
      writeBuffer[2] = 0xFF;
      writeBuffer[3] = 0xFF;
      writeBuffer[4] = 0x00;
      writeBuffer[5] = outInt16.buffer[0];
      writeBuffer[6] = outInt16.buffer[1];
      //FlashWritePartialPage(currentPageAddress,
      FlashWriteByteBlocking(currentPageAddress,    0,WRITE_COMPLETE_REC_END);
      outInt16.val = currentPageAddress;
      FlashWriteByteBlocking(currentRecordAddress , 4,0x00);
      FlashWriteByteBlocking(currentRecordAddress , 5,outInt16.buffer[0]);
      FlashWriteByteBlocking(currentRecordAddress , 6,outInt16.buffer[1]);
    }*/
    if (logIndex == 0 ){
      
      outInt16.val = currentRecordNumber;
      writeBuffer[logIndex++] = outInt16.buffer[0];
      writeBuffer[logIndex++] = outInt16.buffer[1];
    }
    
  }

}

void LoggingInit(){
  SearchForLastRecord();
  VerifyPageWriteReady(); 
}

void VerifyPageWriteReady(){
  //if first byte is 0xFF assume that the page is write ready
  uint16_t next4KBoundary; 
  next4KBoundary = (currentPageAddress & 0xFFF0) + 0x0010;
      if (next4KBoundary > 0x3FF0){
        next4KBoundary = 0;
      }
  if ((currentPageAddress & BLOCK_MASK_4K) == 0x00){
    FlashEraseBlock4k(currentPageAddress);
  }
  else{

    while(VerifyWriteReady() == false){
    }
    FlashEraseBlock4k(next4KBoundary);
    CheckEraseToPageBounds(currentPageAddress);
  }
  currentRecordAddress = currentPageAddress;
  startLogging = true;
}

void CheckEraseToPageBounds(uint16_t currentAddress){
  
  uint8_t numPagesToCheck;
  uint16_t next4KBoundary;
  numPagesToCheck = currentAddress & 0x00F;
  next4KBoundary = (currentAddress & 0xFFF0) + 0x0010;

  for(uint8_t i = 0; i < numPagesToCheck; i++){
    while(VerifyWriteReady() == false){
    }
    if(FlashGetByte((currentAddress + i),0) != 0xFF){
      for(uint8_t i = 0; i < numPagesToCheck; i++){
        while(VerifyWriteReady() == false){
        }
        ClearPage(currentAddress + i);
      }
      currentPageAddress = next4KBoundary;
      return;
    }
  }

}
void SearchForLastRecord(){
  uint8_t  firstByte;
  uint16_t recordNumber,lasPageAddress;
  boolean validRecord,recordComplete;
  uint32_u fullAddress;
  for(uint16_t i = 0; i <= 0x3FFF; i++){
    fullAddress.val = (uint32_t)i << 8;
    FlashSSLow();
    SPI.transfer(READ_ARRAY);
    SPI.transfer(fullAddress.buffer[2]);
    SPI.transfer(fullAddress.buffer[1]);
    SPI.transfer(fullAddress.buffer[0]);
    firstByte = SPI.transfer(0);
    FlashSSHigh();
    if (firstByte == WRITE_COMPLETE_REC_START || firstByte == WRITE_COMPLETE_REC_START_END){
      validRecord = GetRecordNumber(i,&recordNumber,&lasPageAddress,&recordComplete);
      //handle incomplete record for WRITE_COMPLETE_REC_START_END
      if(validRecord == true){
        if (recordComplete == false){
          CompleteRecord(i,&recordNumber);
        }
        if (recordNumber >= currentRecordNumber){ 
          currentRecordNumber = recordNumber + 1;
          currentPageAddress = lasPageAddress + 1;
        }
        if (recordNumber == 0x3FFF){//or 3FFF?
          currentRecordNumber = 0;
          currentPageAddress = 0;
        }

        if (recordNumber <= lowestRecordNumber){
          lowestRecordNumber = recordNumber;
          lowestRecordAddress = i;
        }
      }
    }

  }

}
boolean GetRecordNumber(uint16_t index, uint16_t *recordNumber, uint16_t *endAddress, uint8_t *recordComplete){
  uint16_u inInt16;
  uint32_u fullAddress;
  uint8_t inByte;


  fullAddress.val = ((uint32_t)index << 8) + 1;
  FlashSSLow();
  SPI.transfer(READ_ARRAY);
  SPI.transfer(fullAddress.buffer[2]);
  SPI.transfer(fullAddress.buffer[1]);
  SPI.transfer(fullAddress.buffer[0]);
  for(uint8_t i = 0; i < START_OF_REC_LEN; i++){
    inByte = SPI.transfer(0);
    switch(i){
    case 0:
      if (inByte != 0xAA){
        FlashSSHigh();
        return false;
      }
      break;
    case 1:
      inInt16.buffer[0] = inByte;
      break;
    case 2:
      inInt16.buffer[1] = inByte;
      *recordNumber = inInt16.val;
      break;
    case 3://record complete
      if (inByte == 0x00){
        *recordComplete = true;
      }
      else{
        *recordComplete = false;
      }
      break;
    case 4://last page LSB
      if (*recordComplete == true){
        inInt16.buffer[0] = inByte;
      }
      break;
    case 5://last page LSB
      if (*recordComplete == true){
        inInt16.buffer[1] = inByte;
        *endAddress = inInt16.val;
      }
      else{
        *endAddress = 0;
      }
      break;
    case 6:
      if (inByte!= 0x55){
        FlashSSHigh();
        return true;
      }
      else{
        FlashSSHigh();
        return false;
      }
      break;
    }
  }
  FlashSSHigh();
  return false;



}

void CompleteRecord(uint16_t index, uint16_t startingRecordNumber){
  boolean endOfRecordFound = false;
  uint8_t startByte;
  uint16_t searchCount = 0;
  uint16_u recordNumber,endAddress;
  uint16_t searchAddress,startingAddress;

  while(endOfRecordFound == false){

    //searchAddress = ((uint32_t)index + searchCount) << 8;
    searchAddress = index + searchCount;
    if (searchAddress > 0x3FFF){
      searchAddress -= 0x3FFF;
    }
    startByte = FlashGetByte(searchAddress,0);
    switch(startByte){
    case WRITE_COMPLETE://verify record number
      FlashGetArray(searchAddress,1,sizeof(recordNumber.buffer),recordNumber.buffer);
      if (recordNumber.val != startingRecordNumber){
        endOfRecordFound = true;
        searchAddress -= 1;
        endAddress.val =  searchAddress;
        startingAddress = index;
        FlashWriteByteBlocking(searchAddress,   0,WRITE_COMPLETE_REC_END);

        FlashWriteByteBlocking(startingAddress, 4,0x00);
        FlashWriteByteBlocking(startingAddress, 5,endAddress.buffer[0]);
        FlashWriteByteBlocking(startingAddress, 6,endAddress.buffer[1]);
      }
      break;
    case WRITE_COMPLETE_REC_START:
      if (searchAddress != index){
        endOfRecordFound = true;
        if (searchAddress == 0){
          searchAddress = 0x3FFF;
        }
        else{
          searchAddress -= 1;
        }
        endAddress.val =  searchAddress;
        startingAddress = index;

        FlashWriteByteBlocking(searchAddress,    0,WRITE_COMPLETE_REC_END);

        FlashWriteByteBlocking(startingAddress , 4,0x00);
        FlashWriteByteBlocking(startingAddress , 5,endAddress.buffer[0]);
        FlashWriteByteBlocking(startingAddress , 6,endAddress.buffer[1]);
        break;
      }
      searchAddress += 1;
      if (searchAddress > 0x3FFF){
        searchAddress -= 0x3FFF;
      }
      FlashGetArray(searchAddress, 1,2,recordNumber.buffer);
      if (recordNumber.val != startingRecordNumber){
        endOfRecordFound = true;
        startingAddress = index ;
        endAddress.val =  index;
        FlashWriteByteBlocking(startingAddress,  0,WRITE_COMPLETE_REC_END);

        FlashWriteByteBlocking(startingAddress , 4,0x00);
        FlashWriteByteBlocking(startingAddress , 5,endAddress.buffer[0]);
        FlashWriteByteBlocking(startingAddress , 6,endAddress.buffer[1]);
      }
      break;
    case WRITE_COMPLETE_REC_END://
      FlashGetArray((searchAddress + 1) , 2,sizeof(recordNumber.buffer),recordNumber.buffer);
      startingAddress = index;
      if (recordNumber.val != startingRecordNumber){
        //startingAddress = index;
        endAddress.val =  searchAddress - 1;
        FlashWriteByteBlocking(endAddress.val, 0 ,WRITE_COMPLETE_REC_END);


      }
      FlashWriteByteBlocking(startingAddress , 4,0x00);
      FlashWriteByteBlocking(startingAddress , 5,endAddress.buffer[0]);
      FlashWriteByteBlocking(startingAddress , 6,endAddress.buffer[1]);
      endOfRecordFound = true;

      break;
    default:
      endOfRecordFound = true;

      if (searchAddress == 0){
        searchAddress -= 0x3FFF;
      }
      else{
        searchAddress -= 1;
      }
      if (searchAddress == index){
        while(VerifyWriteReady() == false){
        }
        startingAddress = index;
        endAddress.val =  index;
        FlashWriteByteBlocking(startingAddress, 0,WRITE_COMPLETE_REC_END);

        FlashWriteByteBlocking(startingAddress , 4,0x00);
        FlashWriteByteBlocking(startingAddress , 5,endAddress.buffer[0]);
        FlashWriteByteBlocking(startingAddress , 6,endAddress.buffer[1]);
      }
      else{
        endOfRecordFound = true;
        while(VerifyWriteReady() == false){
        }
        startingAddress = index;
        endAddress.val =  index;
        FlashWriteByteBlocking(startingAddress, 0 ,WRITE_COMPLETE_REC_END);

        FlashWriteByteBlocking(startingAddress , 4,0x00);
        FlashWriteByteBlocking(startingAddress , 5,endAddress.buffer[0]);
        FlashWriteByteBlocking(startingAddress , 6,endAddress.buffer[1]);
      }

      break;
    }

    searchCount++;
    if (searchCount == 0x3FFF){
      endOfRecordFound = true;
      startingAddress = index ;

      if (index == 0){
        endAddress.val =  0x3FFF;
      }
      else{
        endAddress.val =  index - 1;
      }

      FlashWriteByteBlocking(startingAddress,  0,WRITE_COMPLETE_REC_END);

      FlashWriteByteBlocking(startingAddress , 4,0x00);
      FlashWriteByteBlocking(startingAddress , 5,endAddress.buffer[0]);
      FlashWriteByteBlocking(startingAddress , 6,endAddress.buffer[1]);
    }
  }

}


//low level functions read / write / erase /init
//init
void FlashInit(){
  while(VerifyWriteReady() == false){
    Serial<<"init waiting for status reg to clear 1\r\n";
    Serial<<GetStatusReg()<<"\r\n";
    delay(1000);
  } 

  FlashSSLow();
  SPI.transfer(WRITE_ENABLE);
  FlashSSHigh();
  FlashSSLow();
  SPI.transfer(STATUS_WRITE);
  SPI.transfer(0x00);
  FlashSSHigh();
  while(VerifyWriteReady() == false){
    Serial<<"init waiting for status reg to clear 2\r\n";
    Serial<<GetStatusReg()<<"\r\n";
    delay(1000);
  } 
}
//read
uint8_t FlashGetByte(uint16_t pageAddress, uint8_t byteAddress){
  uint32_u addressOutput;
  uint8_t inByte;
  addressOutput.val = ((uint32_t)pageAddress << 8) + (uint32_t)byteAddress;
  FlashSSLow();
  SPI.transfer(READ_ARRAY);
  SPI.transfer(addressOutput.buffer[2]);
  SPI.transfer(addressOutput.buffer[1]);
  SPI.transfer(addressOutput.buffer[0]);
  inByte = SPI.transfer(0);
  FlashSSHigh();
  return inByte;
}

boolean FlashGetArray(uint16_t pageAddress, uint8_t byteAddress,uint16_t numBytes, uint8_t readBuffer[]){
  uint32_u addressOutput;
  if (sizeof(readBuffer) != numBytes){
    return false;
  }
  if (numBytes > 256){
    return false;
  }
  if (numBytes < (256 - byteAddress) ){
    return false;
  }
  addressOutput.val = ((uint32_t)pageAddress << 8) + (uint32_t)byteAddress;
  FlashSSLow();
  SPI.transfer(READ_ARRAY);
  SPI.transfer(addressOutput.buffer[2]);
  SPI.transfer(addressOutput.buffer[1]);
  SPI.transfer(addressOutput.buffer[0]);
  for(uint16_t i = 0; i < numBytes; i++){
    readBuffer[i] = SPI.transfer(0x00);
  }
  FlashSSHigh();
  return true;
}
boolean FlashGetPage(uint16_t pageAddress,uint16_t numBytes,uint8_t readBuffer[]){
  uint32_u addressOutput;
  if (numBytes != 256){
    return false;
  }
  addressOutput.val = ((uint32_t)pageAddress << 8);

  FlashSSLow();
  SPI.transfer(READ_ARRAY);
  SPI.transfer(addressOutput.buffer[2]);
  SPI.transfer(addressOutput.buffer[1]);
  SPI.transfer(addressOutput.buffer[0]);
  for(uint16_t i = 0; i < 256; i++){
    readBuffer[i] = SPI.transfer(0x00);
  }
  FlashSSHigh();
  return true;

}
//write
boolean FlashWriteByte(uint16_t pageAddress, uint8_t byteAddress, uint8_t writeByte){
  uint32_u addressOutput;

  while(VerifyWriteReady() == false){
  } 
  addressOutput.val = ((uint32_t)pageAddress << 8) + (uint32_t)byteAddress;
  FlashSSLow();
  SPI.transfer(PROGRAM_PAGE);
  SPI.transfer(addressOutput.buffer[2]);
  SPI.transfer(addressOutput.buffer[1]);
  SPI.transfer(addressOutput.buffer[0]);
  SPI.transfer(writeByte);
  FlashSSHigh();
  return true;
}

boolean FlashWriteByteBlocking(uint16_t pageAddress, uint8_t byteAddress, uint8_t writeByte){
  uint32_u addressOutput;

  if (VerifyWriteReady() == false){
    return false;
  }
  addressOutput.val = ((uint32_t)pageAddress << 8) + (uint32_t)byteAddress;
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
  addressOutput.val = ((uint32_t)pageAddress << 8) + (uint32_t)byteAddress;
  FlashSSLow();
  SPI.transfer(PROGRAM_PAGE);
  SPI.transfer(addressOutput.buffer[2]);
  SPI.transfer(addressOutput.buffer[1]);
  SPI.transfer(addressOutput.buffer[0]);
  for(uint16_t i = 0; i < numBytes; i++){
    SPI.transfer(writeBuffer[i]);
  }
  FlashSSHigh();
  return true;
}

boolean FlashWritePage(uint16_t pageAddress, uint16_t numBytes, uint8_t writeBuffer[]){
  uint32_u addressOutput;

  if (VerifyWriteReady() == false){
    return false;
  }
  if (numBytes != 256){
    return false;
  }
  addressOutput.val = ((uint32_t)pageAddress << 8);
  FlashSSLow();
  SPI.transfer(PROGRAM_PAGE);
  SPI.transfer(addressOutput.buffer[2]);
  SPI.transfer(addressOutput.buffer[1]);
  SPI.transfer(addressOutput.buffer[0]);
  for(uint16_t i = 0; i < 256; i++){
    SPI.transfer(writeBuffer[i]);
  }
  FlashSSHigh();
  return true;  
}
//erase
boolean ClearPage(uint16_t pageAddress){
  uint32_u addressOutput;
  if (VerifyWriteReady() == false){
    return false;
  }
  addressOutput.val = ((uint32_t)pageAddress << 8);
  FlashSSLow();
  SPI.transfer(PROGRAM_PAGE);
  SPI.transfer(addressOutput.buffer[2]);
  SPI.transfer(addressOutput.buffer[1]);
  SPI.transfer(addressOutput.buffer[0]);
  for(uint16_t i = 0; i < 256; i++){
    SPI.transfer(0x00);
  }
  FlashSSHigh();
  return true; 
}

boolean FlashEraseBlock4k(uint16_t blockAddress){
  uint32_u addressOutput;
  if (blockAddress > 0x3FF0){
    return false;
  }

  if ((blockAddress & BLOCK_MASK_4K) == 0x0000){
    addressOutput.val = ((uint32_t)blockAddress << 8);
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

  if ((blockAddress & BLOCK_MASK_32K) == 0x0000 ){//|| (blockAddress & BLOCK_MASK_32K) == 0x0080){
    addressOutput.val = ((uint32_t)blockAddress << 8);
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

  if ((blockAddress & BLOCK_MASK_64K) == 0x0000){
    addressOutput.val = ((uint32_t)blockAddress << 8);
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

  while(VerifyWriteReady() == false){
    Serial<<"Erase wait 1\r\n";
    delay(1000);
  }
  FlashSSLow();
  SPI.transfer(WRITE_ENABLE);
  FlashSSHigh();

  FlashSSLow();
  SPI.transfer(ERASE_CHIP);
  FlashSSHigh();
  while(VerifyWriteReady() == false){
    Serial<<"Erase wait 2\r\n";
    delay(1000);
  }
  Serial<<_HEX(GetStatusReg())<<"\r\n";
  if (CheckForSuccessfulWrite() == true){
    Serial<<"erase successful\r\n";
  }
  else{
    Serial<<"erase failed\r\n";
    Serial<<_HEX(GetStatusReg())<<"\r\n";
    while(1){
      Serial<<"erased failed holding\r\n";
      delay(3000);
    }
  }

}
//status
boolean CheckForSuccessfulWrite(){
  uint8_t statusReg;
  statusReg = GetStatusReg();
  if ( (statusReg & WRITE_ERROR_MASK) == 0x00){
    return true;
  }
  else{
    return false;
  }
}

uint8_t GetStatusReg(){
  uint8_t inByte1,inByte2;
  FlashSSLow();
  SPI.transfer(READ_STATUS_REG);
  inByte1 = SPI.transfer(0);
  inByte2 = SPI.transfer(0);
  FlashSSHigh(); 
  return inByte1;
  //return 0 device ready write not enabled
  //return 1 device busy wrtie not enabled
  //return 2 device ready write enabled 
  //return 3 device busy write enabled 
}
void DispStatRegs(){
  uint8_t inByte1,inByte2;
  FlashSSLow();
  SPI.transfer(READ_STATUS_REG);
  inByte1 = SPI.transfer(0);
  inByte2 = SPI.transfer(0);
  FlashSSHigh(); 
  Serial<<"SB1: "<<_HEX(inByte1)<<"\r\nSB2: "<<_HEX(inByte2)<<"\r\n";
}

void WriteEnable(){
  FlashSSLow();
  SPI.transfer(WRITE_ENABLE);
  FlashSSHigh();

}
boolean VerifyWriteReady(){
  uint8_t statusReg;
  statusReg = GetStatusReg() & 0x03;
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






























