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
    //Serial.println(SPI.transfer(0),HEX);
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
  //SPI.transfer(0x00);
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
  //SPI.transfer(0x00);
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
}

void loop(){
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
boolean CheackStatusReg(){
  uint8_t statusRegister;
  
  FlashSSLow();
  SPI.transfer(READ_STATUS_REG);
  statusRegister = SPI.transfer(0);
  FlashSSHigh();  

  if (statusRegister & 0x01) == 0x01){
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
  while( (statusByte & 0x01) == 0x01){
    FlashSSLow();
    SPI.transfer(READ_STATUS_REG);
    statusByte = SPI.transfer(0);
    Serial.println(statusByte,HEX);
    FlashSSHigh();
    delay(5);
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
  while( (statusByte & 0x01) == 0x01){
    FlashSSLow();
    SPI.transfer(READ_STATUS_REG);
    statusByte = SPI.transfer(0);
    Serial.println(statusByte,HEX);
    FlashSSHigh();
    delay(5);
  }
  Serial.println(millis());  
  return true;
}










