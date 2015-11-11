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
#define WRITE_COMPLETE_REC_END 0x2F
#define WRITE_COMPLETE_REC_START_END 0x0F
#define TO_ERASE 0x00
#define START_OF_REC_LEN 7


uint16_t logNumber;

typedef union{
  uint16_t val;
  uint8_t buffer[2];
}
uint16_u;

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

uint16_t currentRecordNumber, currentPageAddress,
lowestRecordNumber,  lowestRecordAddress;
//nextRecordNumber,    nextRecordAddress;


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
  FlashLogStateMachine();
}

void FlashLogStateMachine(){

  static uint8_t loggingState = 0;

  switch (loggingState){
  case BOUNDARY_CHECK:
    CheckBlockBounds(currentPageAddress);
    break;
  case WAIT_FOR_WRITE_READY:
    if (CheackStatusReg() == true){
      loggingState = BOUNDARY_CHECK;
    }
    break;
  case ASSEMBLE_WRITE_BUFFER:
    break;
  case WRITE_BUFFER_TO_ROM:
    
    break;
  }

}

void CheckBlockBounds(uint16_t startingAddress){
  uint16_t pageToErase;
  if ( (startingAddress & 0x000F) == 0){
    //check to see if each start byte is 0xFF and erase if not
  }
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
  SearchForLastRecord();//make search for first and last record?
  VerifyPageErasure();
  //SearchForFirstRecord();
  //set flash logging rates / types of data
}

void VerifyPageErasure(){
  //this function will finalize flash address pointers
  uint16_t pageToErase;
  if ( (currentPage & 0x000F) == 0){
    //starting page is on the block boundary 
    EraseBlock(currentPage);
  }
  else{
    //starting page is in the middle of a block
    //check to see if the rest of the pages in the block are erased
    //if not erased set all to zero and erase next block
    pageToErase = currentPage;
    while( (pageToErase & 0x000F) != 0){
      FlashClearPage(pageToErase);
      pageToErase++;
    }
    EraseBlock(pageToErase);
    currentPage = pageToErase;
  }
}

void SearchForLastRecord(){
  uint8_t  firstByte;
  uint16_t recordNumber,lasPageAddress;
  boolean validRecord,recordComplete;
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
    if (firstByte == WRITE_COMPLETE_REC_START || firstByte == WRITE_COMPLETE_REC_START_END){
      validRecord = GetRecordNumber(&i,&recordNumber,&lasPageAddress,&recordComplete);
      if(validRecord == true){
        if (recordComplete == false){
          CompleteRecord(&i,&recordNumber);
        }
        if (recordNumber >= currentRecordNumber){ 
          currentRecordNumber = recordNumber + 1;
          currentPageAddress = lasPageAddress + 1;
        }
        if (recordNumber == 0xFFFF){//or 3FFF?
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

void CompleteRecord(uint16_t *index,uint16_t *startingRecordNumber){
  boolean endOfRecordFound = false;
  uint8_t startByte;
  uint16_t searchCount = 0;
  uint16_u recordNumber,endAddress;
  uint32_u searchAddress,startingAddress;
  while(endOfRecordFound == false){

    searchAddress.val = (*index + searchCount) << 8;
    if (searchAddress.val > 0x3FFF00){
      searchAddress.val -= 0x3FFF00;
    }
    startByte = FlashGetByte(searchAddress.val);
    switch(startByte){
    case WRITE_COMPLETE://verify record number
      FlashGetArray(searchAddress.val + 1,2,recordNumber.buffer);
      if (recordNumber.val != *startingRecordNumber){
        endOfRecordFound = true;
        searchAddress.val -= 0x100;
        while(CheackStatusReg() == false){
        }
        FlashWriteByte(searchAddress.val,WRITE_COMPLETE_REC_END);
        while(CheackStatusReg() == false){
        }
        endAddress.val =  searchAddress.val >> 8;
        startingAddress.val = *index << 8;
        FlashWriteByte(startingAddress.val + 4,0x00);
        FlashWriteByte(startingAddress.val + 5,endAddress.buffer[0]);
        FlashWriteByte(startingAddress.val + 6,endAddress.buffer[1]);
      }
      break;
    case WRITE_COMPLETE_REC_START://check to see if next page is same number
      if (searchAddress.val != *index << 8){
        endOfRecordFound = true;
        if (searchAddress.val == 0){
          searchAddress.val = 0x3FFF;
        }
        else{
          searchAddress.val -= 0x100;
        }
        endAddress.val =  searchAddress.val >> 8;
        FlashWriteByte(searchAddress.val,WRITE_COMPLETE_REC_END);
        startingAddress.val = *index << 8;
        FlashWriteByte(startingAddress.val + 4,0x00);
        FlashWriteByte(startingAddress.val + 5,endAddress.buffer[0]);
        FlashWriteByte(startingAddress.val + 6,endAddress.buffer[1]);
      }
      searchAddress.val += 0x100;
      if (searchAddress.val > 0x3FFF00){
        searchAddress.val -= 0x3FFF00;
      }
      FlashGetArray(searchAddress.val + 1,2,recordNumber.buffer);
      if (recordNumber.val != *startingRecordNumber){
        endOfRecordFound = true;
        while(CheackStatusReg() == false){
        }
        startingAddress.val = *index << 8;
        FlashWriteByte(startingAddress.val,WRITE_COMPLETE_REC_END);
        endAddress.val =  *index;
        FlashWriteByte(startingAddress.val + 4,0x00);
        FlashWriteByte(startingAddress.val + 5,endAddress.buffer[0]);
        FlashWriteByte(startingAddress.val + 6,endAddress.buffer[1]);
      }
      break;
    case WRITE_COMPLETE_REC_END://
      FlashGetArray(searchAddress.val + 1,2,recordNumber.buffer);
      if (recordNumber.val != *startingRecordNumber){
        endOfRecordFound = true;
        while(CheackStatusReg() == false){
        }
        startingAddress.val = *index << 8;
        FlashWriteByte(startingAddress.val,WRITE_COMPLETE_REC_END);
        endAddress.val =  searchAddress.val >> 8;
        FlashWriteByte(startingAddress.val + 4,0x00);
        FlashWriteByte(startingAddress.val + 5,endAddress.buffer[0]);
        FlashWriteByte(startingAddress.val + 6,endAddress.buffer[1]);
      }
      break;
    default:
      endOfRecordFound = true;
      if (searchAddress.val - 0x100 == *index){
        while(CheackStatusReg() == false){
        }
        startingAddress.val = *index << 8;
        FlashWriteByte(startingAddress.val,WRITE_COMPLETE_REC_END);
        endAddress.val =  *index;
        FlashWriteByte(startingAddress.val + 4,0x00);
        FlashWriteByte(startingAddress.val + 5,endAddress.buffer[0]);
        FlashWriteByte(startingAddress.val + 6,endAddress.buffer[1]);
      }
      else{
        endOfRecordFound = true;
        while(CheackStatusReg() == false){
        }
        startingAddress.val = *index << 8;
        FlashWriteByte(startingAddress.val,WRITE_COMPLETE_REC_END);
        endAddress.val =  *index;
        FlashWriteByte(startingAddress.val + 4,0x00);
        FlashWriteByte(startingAddress.val + 5,endAddress.buffer[0]);
        FlashWriteByte(startingAddress.val + 6,endAddress.buffer[1]);
      }

      break;
    }

    if (searchAddress.val == *index - 1){
      //todo last possible address handling
    }
    searchCount++;
    if (searchCount == 0x4000){
      endOfRecordFound = true;
    }
  }

}

boolean GetRecordNumber(uint16_t *index, uint16_t *recordNumber, uint16_t *endAddress, uint8_t *recordComplete){
  uint16_u inInt16;
  uint32_u fullAddress;
  uint8_t StartOfRecordBuffer[START_OF_REC_LEN];


  fullAddress.val = (*index << 8) + 1;
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
      inInt16.buffer[0] = StartOfRecordBuffer[i];
      break;
    case 2:
      inInt16.buffer[1] = StartOfRecordBuffer[i];
      *recordNumber = inInt16.val;
      break;
    case 3://record complete
      if (StartOfRecordBuffer[i] == 0x00){
        *recordComplete = true;
      }
      else{
        *recordComplete = false;
      }
      break;
    case 4://last page LSB
      if (*recordComplete == true){
        inInt16.buffer[0] = StartOfRecordBuffer[i];
      }
      break;
    case 5://last page LSB
      if (*recordComplete == true){
        inInt16.buffer[1] = StartOfRecordBuffer[i];
        *endAddress = inInt16.val;
      }
      else{
        *endAddress = 0;
      }
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
  return false;



}

/*void SearchForFirstRecord(){
 
 }*/

uint8_t FlashGetByte(uint32_t startingAddress){
  uint32_u pgIndx;
  pgIndx.val = startingAddress;
  FlashSSLow();
  SPI.transfer(READ_ARRAY);
  SPI.transfer(pgIndx.buffer[2]);
  SPI.transfer(pgIndx.buffer[1]);
  SPI.transfer(pgIndx.buffer[0]);
  return SPI.transfer(0);

  FlashSSHigh();
}

void FlashGetArray(uint32_t startingAddress, uint8_t sizeOfArray, uint8_t outputArray[]){
  uint32_u pgIndx;
  pgIndx.val = startingAddress;
  FlashSSLow();
  SPI.transfer(READ_ARRAY);
  SPI.transfer(pgIndx.buffer[2]);
  SPI.transfer(pgIndx.buffer[1]);
  SPI.transfer(pgIndx.buffer[0]);
  for(int i = 0; i <= (sizeOfArray - 1); i++){
    outputArray[i] = SPI.transfer(0);
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

void FlashWriteByte(uint32_t startingAddress,uint8_t data){
  uint32_u pageIndex;
  pageIndex.val = startingAddress;
  FlashSSLow();
  SPI.transfer(PROGRAM_PAGE);
  SPI.transfer(pageIndex.buffer[2]);
  SPI.transfer(pageIndex.buffer[1]);
  SPI.transfer(pageIndex.buffer[0]);
  SPI.transfer(data);
  FlashSSHigh();

}

void FlashWritePage(uint32_t startingAddress, uint8_t buffer[]){
  uint32_u pgIndx;
  pgIndx.val = startingAddress;
  FlashSSLow();
  SPI.transfer(WRITE_ENABLE);
  FlashSSHigh();
  FlashSSLow();
  SPI.transfer(PROGRAM_PAGE);
  SPI.transfer(pgIndx.buffer[2]);
  SPI.transfer(pgIndx.buffer[1]);
  SPI.transfer(pgIndx.buffer[0]);
  for(int i = 0; i <= 255; i++){
    SPI.transfer(buffer[i]);
  }
  FlashSSHigh();
}
void WritePartialPage(uint32_t startingAddress, uint8_t numBytes, uint8_t buffer){
  //to do check for boundary conditions and wrap around
  uint32_u pgIndx;
  pgIndx.val = startingAddress;
  FlashSSLow();
  SPI.transfer(WRITE_ENABLE);
  FlashSSHigh();
  FlashSSLow();
  SPI.transfer(PROGRAM_PAGE);
  SPI.transfer(pgIndx.buffer[2]);
  SPI.transfer(pgIndx.buffer[1]);
  SPI.transfer(pgIndx.buffer[0]);
  for(int i = 0; i <= numBytes; i++){
    SPI.transfer(buffer[i]);
  }
  FlashSSHigh();
}

void FlashClearPage(uint32_t startingAddress){
  //to do check for boundary conditions and wrap around
  uint32_u pgIndx;
  pgIndx.val = startingAddress;
  FlashSSLow();
  SPI.transfer(WRITE_ENABLE);
  FlashSSHigh();
  FlashSSLow();
  SPI.transfer(PROGRAM_PAGE);
  SPI.transfer(pgIndx.buffer[2]);
  SPI.transfer(pgIndx.buffer[1]);
  SPI.transfer(pgIndx.buffer[0]);
  for(int i = 0; i <= numBytes; i++){
    SPI.transfer(0x00);
  }
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

boolean EraseBlock(uint32_t address){//need to return anything?
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
































