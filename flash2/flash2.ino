#include <SPI.h>
#include <Streaming.h>
#include "Flash.h"
#include "Defines.h"



void setup(){
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
  
  Serial.begin(115200);
  /*for(uint16_t i = 0; i <= 255; i++){//????? ask zach
    Serial<<i<<"\r\n";
  }
  for(uint8_t j = 0; j <= 0xFF; j++){
    Serial<<j<<"\r\n";
  }*/
  Serial<<"start\r\n";
  FlashInit();
  Serial<<"erase chip\r\n";
  FlashEraseChip();
  Serial<<"fill flash\r\n";
  FillFlash();
  Serial<<"wait for status reg\r\n";
  while(GetStatusReg() & 0x01 != 0){
  } 
  Serial<<"dump flash\r\n";
  FlashDump();
}

void loop(){

}

void FillFlash(){
  uint16_u pageNumber;
  uint8_t outputArray[256];
  for(uint16_t i = 0; i <= 0x3FFF; i++){
    Serial<<"* "<<i<<"\r\n";
    
    Serial<<"======================\r\n";
    for(uint16_t j = 0; j <= 0xFF; j++){
      //Serial<<"++ "<<j<<"\r\n";
      outputArray[j] = (uint8_t)j;
    }
    Serial<<sizeof(outputArray);
    Serial<<"++++++++++++++++++++++++++++\r\n";
    pageNumber.val = i;
    outputArray[0] = pageNumber.buffer[0];
    outputArray[1] = pageNumber.buffer[1];
    while(DeviceReadyToWrite() == false){
      Serial<<millis()<<"\r\n";
      delay(50);
    }
    FlashWritePage(i,sizeof(outputArray),outputArray);
  }
}
void FlashDump(){
  uint8_t outputArray[256];
  for(uint16_t i = 0; i <= 0x3FFF; i++){
    FlashGetPage(i,outputArray);
    Serial<<"----------------\r\n";
    for(uint8_t j = 0; j < 256; j++){
      Serial.println(outputArray[j],HEX);
    }
  }
}








