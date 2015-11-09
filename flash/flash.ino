#include <SPI.h>
#include <Streaming.h>

//however digitalWrite will work when using SPI 
#define GyroSSOutput() DDRL |= 1<<0 
#define GyroSSHigh() PORTL |= 1<<0 
#define GyroSSLow() PORTL &= ~(1<<0)

#define AccSSOutput() DDRL |= 1<<1 
#define AccSSHigh() PORTL |= 1<<1 
#define AccSSLow() PORTL &= ~(1<<1)

#define BaroSSOutput() DDRL |= 1<<2 
#define BaroSSHigh() PORTL |= 1<<2 
#define BaroSSLow() PORTL &= ~(1<<2)

#define MagSSOutput() DDRL |= 1<<3 
#define MagSSHigh() PORTL |= 1<<3
#define MagSSLow() PORTL &= ~(1<<3)

#define FlashSSOutput() DDRL |= 1<<4
#define FlashSSHigh() PORTL |= 1<<4
#define FlashSSLow() PORTL &= ~(1<<4)


#define READ_ARRAY 0x03
#define ERASE_4K 0x20
#define ERASE_32K 0x52
#define ERASE_64K 0xD8
#define ERASE_CHIP 0xC7
#define PROGRAM_PAGE 0x02

#define WRITE_ENABLE 0x06

#define READ_STATUS_REG 0x05

#define STATUS_WRITE 0x01

#define TOP_ADDRESS 0x3FF000

//first byte flags
#define ERASED 0XFF
#define WRITE_STARTED 0x7F
#define WRITE_COMPLETE 0x3F
#define WRITE_COMPLETE_REC_START 0x1F
#define WRITE_COMPLETE_REC_END 0x0F
#define START_OF_REC_LEN 7
uint16_t logNumber;


typedef union{
  uint32_t val;
  uint8_t buffer[4];
}
uint32_u;

typedef union {
  float val;
  uint8_t buffer[4];
}
float_u;


uint32_u pageIndex;

uint8_t buffer[256];

uint32_t time;

#define ADDRESS_LIMIT 0x3FFFFF

uint32_t addressIndex;
uint8_t statusByte;
float_u outFloat;


uint16_t pageAddress;
uint8_t  byteAddress;
uint32_u completeAddress;

uint8_t writeBuffer[256];

boolean writeEnabled,deviceReady;
uint8_t statusRegister;

uint16_t currentRecordNumber,currentPageAddress;


void setup(){
  Serial.begin(115200);

  GyroSSOutput();
  AccSSOutput();
  BaroSSOutput();
  MagSSOutput();
  FlashSSOutput();
  GyroSSHigh();
  AccSSHigh();
  BaroSSHigh();
  MagSSHigh();
  FlashSSHigh();

  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV2);   
  SPI.setDataMode(SPI_MODE0);

  pageIndex.val = 0;

  FlashInit();
  Serial<<"begin erase\r\n";
  EraseChip();

  FlashSSLow();
  SPI.transfer(READ_STATUS_REG);
  Serial.println(SPI.transfer(0),HEX);
  FlashSSHigh();
  delay(5);

  FlashSSLow();
  SPI.transfer(READ_ARRAY);
  SPI.transfer(pageIndex.buffer[2]);
  SPI.transfer(pageIndex.buffer[1]);
  SPI.transfer(pageIndex.buffer[0]);
  for(int i = 0; i <= 255; i++){
    buffer[i] = SPI.transfer(0);
  }

  FlashSSHigh();
  Serial<<"++++\r\n";
  for(int i = 0; i <= 255; i++){
    Serial.println(buffer[i]);
  }

  delay(5);

  delay(5);
  FlashSSLow();
  SPI.transfer(WRITE_ENABLE);
  FlashSSHigh();

  FlashSSLow();
  SPI.transfer(STATUS_WRITE);
  SPI.transfer(0x00);
  FlashSSHigh();



  FlashSSLow();
  SPI.transfer(READ_STATUS_REG);
  Serial.println(SPI.transfer(0),HEX);
  FlashSSHigh();

  delay(5);
  FlashSSLow();
  SPI.transfer(WRITE_ENABLE);
  FlashSSHigh();

  FlashSSLow();
  SPI.transfer(READ_STATUS_REG);
  Serial.println(SPI.transfer(0),HEX);
  FlashSSHigh();

  FlashSSLow();
  //SPI.transfer(WRITE_ENABLE);
  SPI.transfer(PROGRAM_PAGE);
  SPI.transfer(pageIndex.buffer[2]);
  SPI.transfer(pageIndex.buffer[1]);
  SPI.transfer(pageIndex.buffer[0]);
  for(int i = 0; i <= 255; i++){
    SPI.transfer((uint8_t)i);
  }
  FlashSSHigh();

  delay(5);

  FlashSSLow();
  SPI.transfer(READ_ARRAY);
  SPI.transfer(pageIndex.buffer[2]);
  SPI.transfer(pageIndex.buffer[1]);
  SPI.transfer(pageIndex.buffer[0]);
  for(int i = 0; i <= 255; i++){
    buffer[i] = SPI.transfer(0);
    //Serial.println(SPI.transfer(0),HEX);
  }

  FlashSSHigh();
  Serial<<"****\r\n";
  for(int i = 0; i <= 255; i++){
    Serial.println(buffer[i]);
  }
  EraseBlock(0x00);
  FlashSSLow();
  SPI.transfer(READ_ARRAY);
  SPI.transfer(pageIndex.buffer[2]);
  SPI.transfer(pageIndex.buffer[1]);
  SPI.transfer(pageIndex.buffer[0]);
  for(int i = 0; i <= 255; i++){
    buffer[i] = SPI.transfer(0);
    //Serial.println(SPI.transfer(0),HEX);
  }

  FlashSSHigh();
  Serial<<"qqqqq\r\n";
  for(int i = 0; i <= 255; i++){
    Serial.println(buffer[i]);
  }

  //time to search the start of each page
  Serial<<"start time\r\n";
  Serial<<millis()<<"\r\n";
  for(uint16_t i = 0; i <= 0x3FF; i++){
    pageIndex.val = i << 8;
    FlashSSLow();
    SPI.transfer(READ_ARRAY);
    SPI.transfer(pageIndex.buffer[2]);
    SPI.transfer(pageIndex.buffer[1]);
    SPI.transfer(pageIndex.buffer[0]);
    //Serial.print(i);
    //Serial.print(" ");
    //Serial.println(SPI.transfer(0),HEX);
    FlashSSHigh();
  }
  Serial<<millis()<<"\r\n";
  Serial<<"end time\r\n";



}

void loop(){
  /*
  if (millis() - writeTimer > WRITE_TIME){
   
   }
   if (writeFlash == true){
   switch(writeState){
   case CHECK_FIRST_BYTE:
   break;
   case WRITE_BUFFER:
   break;
   case 
   
   }
   }*/

}
void FlashInit(){
  FlashSSLow();
  SPI.transfer(WRITE_ENABLE);
  FlashSSHigh();
  FlashSSLow();
  SPI.transfer(STATUS_WRITE);
  SPI.transfer(0x00);
  FlashSSHigh();

}
void LoggingInit(){
  SearchForLastRecord();
  SearchForFirstRecord();

}

void SearchForLastRecord(){
  uint8_t  firstByte;
  uint16_t recordNumber,lasPageAddress;
  boolean validRecord;
  uint32_u fullAddress;
  for(uint16_t i = 0; i <= 0x3FFF; i++){
    fullAddress.val = i << 8;
    FlashSSLow();
    SPI.transfer(READ_ARRAY);
    SPI.transfer(fullAddress.buffer[2]);
    SPI.transfer(fullAddress.buffer[1]);
    SPI.transfer(fullAddress.buffer[0]);
    firstByte = SPI.transfer(0);
    FlashSSHigh();
    if (firstByte == WRITE_COMPLETE_REC_START){
      validRecord = GetRecordNumber(&i,&recordNumber,&lasPageAddress);
      if(validRecord == true){

        if (recordNumber >= currentRecordNumber){
          currentRecordNumber = recordNumber + 1;
          currentPageAddress = lasPageAddress + 1;
        }

      }
    }

  }

}

boolean GetRecordNumber(uint16_t* index, uint16_t* recordNumber,uint16_t* endAddress){
  uint16_u inInt16;
  uint32_u fullAddress;
  uint8_t StartOfRecordBuffer[START_OF_REC_LEN];


  fullAddress = (index << 8) + 1;
  FlashSSLow();
  SPI.transfer(READ_ARRAY);
  SPI.transfer(fullAddress.buffer[2]);
  SPI.transfer(fullAddress.buffer[1]);
  SPI.transfer(fullAddress.buffer[0]);
  for(uint8_t i = 0; i < START_OF_REC_LEN; i++){
    StartOfRecordBuffer[i] = SPI.transfer(0);
    switch(i){
    case 0:
      if (StartOfRecordBuffer[i] != 0xAA){
        return false;
      }
      break;
    case 1:
      inInt16[0] = StartOfRecordBuffer[i];
      break;
    case 2:
      inInt16[1] = StartOfRecordBuffer[i];
      break;
    case 3://record complete

      break;
    case 4://last page LSB

      break;
    case 5://last page LSB

      break;
    case 6:
      if (StartOfRecordBuffer[i] != 0x55){
        return true;
      }
      else{
        return false;
      }
      break;
    }
  }
  FlashSSHigh();




}

void SearchForFirstRecord(){

}

void FlashGetByte(uint32_t startingAddress){
  uint32_u pgIndx;
  pgIndx.val = startingAddress;
  FlashSSLow();
  SPI.transfer(READ_ARRAY);
  SPI.transfer(pgIndx.buffer[2]);
  SPI.transfer(pgIndx.buffer[1]);
  SPI.transfer(pgIndx.buffer[0]);
  for(int i = 0; i <= 0; i++){//remove
    buffer[i] = SPI.transfer(0);
    //Serial.println(SPI.transfer(0),HEX);
  }

  FlashSSHigh();
}

void FlashGetArray(uint32_t startingAddress, uint8_t sizeOfArray){
  uint32_u pgIndx;
  pgIndx.val = startingAddress;
  FlashSSLow();
  SPI.transfer(READ_ARRAY);
  SPI.transfer(pgIndx.buffer[2]);
  SPI.transfer(pgIndx.buffer[1]);
  SPI.transfer(pgIndx.buffer[0]);
  for(int i = 0; i <= (sizeOfArray - 1); i++){
    buffer[i] = SPI.transfer(0);
    //Serial.println(SPI.transfer(0),HEX);
  }

  FlashSSHigh();
}

void FlashGetPage(uint32_t startingAddress){
  uint32_u pgIndx;
  pgIndx.val = startingAddress;
  FlashSSLow();
  SPI.transfer(READ_ARRAY);
  SPI.transfer(pgIndx.buffer[2]);
  SPI.transfer(pgIndx.buffer[1]);
  SPI.transfer(pgIndx.buffer[0]);
  for(int i = 0; i <= 255; i++){
    buffer[i] = SPI.transfer(0);
    //Serial.println(SPI.transfer(0),HEX);
  }

  FlashSSHigh();
}

void FlashWriteByte(uint32_t startingAddress){
  FlashSSLow();
  SPI.transfer(WRITE_ENABLE);
  FlashSSHigh();

}

void FlashWritePage(uint32_t startingAddress){
  uint32_u pgIndx;
  pgIndx.val = startingAddress;
  FlashSSLow();
  SPI.transfer(WRITE_ENABLE);
  FlashSSHigh();
  FlashSSLow();
  SPI.transfer(READ_ARRAY);
  SPI.transfer(pgIndx.buffer[2]);
  SPI.transfer(pgIndx.buffer[1]);
  SPI.transfer(pgIndx.buffer[0]);
  for(int i = 0; i <= 255; i++){
    SPI.transfer((uint8_t)i);
  }
  FlashSSHigh();
}
void WritePartialPage(uint32_t startingAddress, uint8_t numBytes){
  FlashSSLow();
  SPI.transfer(WRITE_ENABLE);
  FlashSSHigh(); 
}

boolean CheackStatusReg(){


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
void EraseChip(){
  FlashSSLow();
  SPI.transfer(WRITE_ENABLE);
  FlashSSHigh();

  FlashSSLow();
  SPI.transfer(ERASE_CHIP);
  FlashSSHigh();

  FlashSSLow();
  SPI.transfer(READ_STATUS_REG);
  statusByte = SPI.transfer(0);
  Serial.println(statusByte,HEX);
  FlashSSHigh();

  Serial.println(millis());
  while(CheackStatusReg() == false){
    Serial.println(statusRegister,HEX);
  } 

  Serial.println(millis());  
}

boolean EraseBlock(uint32_t address){
  uint16_t addressLow;
  uint32_u addressOutput;

  if (address > TOP_ADDRESS){
    return false;
  }
  addressLow = address & 0x00000FFF;
  if (addressLow != 0){
    return false;
  } 

  FlashSSLow();
  SPI.transfer(WRITE_ENABLE);
  FlashSSHigh();



  addressOutput.val = address;
  FlashSSLow();
  SPI.transfer(ERASE_4K);
  SPI.transfer(addressOutput.buffer[2]);
  SPI.transfer(addressOutput.buffer[1]);
  SPI.transfer(addressOutput.buffer[0]);
  FlashSSHigh();
  Serial.println(millis());
  while(CheackStatusReg() == false){
    Serial.println(statusRegister,HEX);
  } 

  Serial.println(millis());  
  return true;
}




















