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


  Serial<<"dump flash 1\r\n";
  FlashDump();
  Serial<<"fill flash\r\n";
  FillFlash();
  Serial<<"dump flash 2\r\n";
  FlashDump();
}

void loop(){

}

void FillFlash(){
  uint16_u pageNumber;
  uint8_t outputArray[256];
  for(uint16_t j = 0; j <= 256; j++){
    outputArray[j] = (uint8_t)j;
    Serial<<_HEX(j)<<"\r\n";
  }


  //for(uint16_t i = 0; i <= 0x3FFF; i++){
  for(uint16_t i = 0; i <= 5; i++){
    Serial<<"* "<<i<<"\r\n";
    pageNumber.val = i;
    outputArray[0] = pageNumber.buffer[0];
    outputArray[1] = pageNumber.buffer[1];
    while(VerifyWriteReady() == false){
      Serial<<"wait for ready fill flash\r\n";
      Serial<<_HEX(GetStatusReg())<<"\r\n";
      //Serial<<millis()<<"\r\n";
      delay(1000);
    }
    FlashWritePage(i,sizeof(outputArray),outputArray);
  }
}
void FlashDump(){
  uint8_t outputArray[256];
  uint8_t inByte;
  //for(uint16_t i = 0; i <= 0x3FFF; i++){
  //Serial<<"a\r\n";
  for(uint16_t i = 0; i <= 5; i++){
    //Serial<<"b\r\n";
    for(uint16_t j = 0; j < 256; j++){
      outputArray[j] = 0;
    }
    while(VerifyWriteReady() == false){
      Serial<<"dump write ready\r\n";
      DispStatRegs();
      //Serial<<_HEX(GetStatusReg())<<"\r\n";
      //Serial<<millis()<<"\r\n";
      //delay(1000);
    }
    if (FlashGetPage(i,sizeof(outputArray),outputArray) == false){
      Serial<<"failed to get page\r\n";
      while(1){}
    }
    else{
      Serial<<"----- "<<i<<"\r\n";
      for(uint16_t j = 0; j < 256; j++){
        //Serial.print(outputArray[j],HEX);
        Serial<<_HEX(outputArray[j])<<","<<_HEX(FlashGetByte(i,(uint8_t)j))<<"\r\n";
      }
    }
    //FlashGetPage(i,sizeof(outputArray),outputArray);

  }
  Serial<<"c\r\n";

}










