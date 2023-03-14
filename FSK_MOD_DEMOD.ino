#pragma GCC optimize("Ofast")
#pragma GCC target("avx,avx2,fma")

#include "SSD1306Wire.h" // legacy include: `#include "SSD1306.h"`
#include "OLEDDisplayUi.h"
SSD1306Wire display(0x3c, SDA, SCL);  // ADDRESS, SDA, SCL  -  SDA and SCL usually populate automatically based on your board's pins_arduino.h e.g. https://github.com/esp8266/Arduino/blob/master/variants/nodemcu/pins_arduino.h
OLEDDisplayUi ui     ( &display );

#include <driver/i2s.h>


#define LED_BUILTIN 2
//these settings are very sensitive
#define TX_BUFFER_SIZE 256
#define RX_BUFFER_SIZE 32
#define SAMPLE_RATE_MIC 32000
#define SAMPLE_RATE_AMP 8000


// either wire your microphone to the same pins or change these to match your wiring
#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_32
#define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_33
#define I2S_MIC_SERIAL_DATA GPIO_NUM_35

#define I2S_OUT_SERIAL_CLOCK GPIO_NUM_27
#define I2S_OUT_LEFT_RIGHT_CLOCK GPIO_NUM_14
#define I2S_OUT_SERIAL_DATA GPIO_NUM_26

//MAX98357A
i2s_config_t i2s_out_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE_AMP,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 2,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0};

i2s_pin_config_t i2s_out_pins = {
 //   .mck_io_num = I2S_PIN_NO_CHANGE,
    .bck_io_num = I2S_OUT_SERIAL_CLOCK,
    .ws_io_num = I2S_OUT_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_OUT_SERIAL_DATA,
    .data_in_num = I2S_PIN_NO_CHANGE};
//works for sph0645
//changed to inmp441
i2s_config_t i2s_mic_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE_MIC,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 2,
    .dma_buf_len = 32,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0};

i2s_pin_config_t i2s_mic_pins = {
    .mck_io_num = I2S_PIN_NO_CHANGE,
    .bck_io_num = I2S_MIC_SERIAL_CLOCK,
    .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SERIAL_DATA};

//    Bell 202 AFSK uses a 1200 Hz tone for mark (typically a binary 1) and 2200 Hz for space (typically a binary 0).

///200 bw
//#define MARK_FREQUENCY 1600
//#define SPACE_FREQUENCY 1400
#define MARK_FREQUENCY 2200
#define SPACE_FREQUENCY 1200


//const float baud=45.45;
//const float baud=100;
const float baud=300;
//const float baud=300;


const int bitTime =1000000*(1.0/baud);

const uint samplesPerBit =SAMPLE_RATE_AMP/(baud);   


int32_t rxSamples[RX_BUFFER_SIZE];
int32_t txSamples[TX_BUFFER_SIZE];


//char baudot[33]="_T\nO HNM-LRGIPCVEZDBSYFXAWJ<UQK>";
  char baudot[33]="_E\nA SIU-DRJNFCKTZLWHYPQOBG<MXV>";

uint8_t baudotEncode(char c){
  switch(c){
    case '_':return 0b00000;
    case 'E':return 0b00001; 
    case 0xA:return 0b00010; 
    case 'A':return 0b00011; 
    case ' ':return 0b00100; 
    case 'S':return 0b00101; 
    case 'I':return 0b00110; 
    case 'U':return 0b00111; 
    
    case 0xD:return 0b01000; //Carriage Return
    case 'D':return 0b01001; 
    case 'R':return 0b01010; 
    case 'J':return 0b01011; 
    case 'N':return 0b01100; 
    case 'F':return 0b01101; 
    case 'C':return 0b01110; 
    case 'K':return 0b01111; 

    case 'T':return 0b10000; 
    case 'Z':return 0b10001; 
    case 'L':return 0b10010; 
    case 'W':return 0b10011; 
    case 'H':return 0b10100; 
    case 'Y':return 0b10101; 
    case 'P':return 0b10110; 
    case 'Q':return 0b10111; 

    case 'O':return 0b11000; 
    case 'B':return 0b11001; 
    case 'G':return 0b11010; 
    case '<':return 0b11011; 
    case 'M':return 0b11100; 
    case 'X':return 0b11101; 
    case 'V':return 0b11110; 
    case '>':return 0b11111; 
  }
    
return 0 ;
}

void i2sWriteTone(uint16_t f,uint &last ,uint16_t len){
  uint16_t gain=8000;
  int32_t s;
  size_t bytes = 0;
  
  for(uint16_t i=0;i<len;i++){
    s = gain * sin(((float) last * M_PI * 2 * f) / (float) (SAMPLE_RATE_AMP));
    i2s_write(I2S_NUM_1, &s, sizeof(int32_t) , &bytes, 100);///write one character
    last++;
  }
}

String message;

void transmitFSK(String sendMessage){
  size_t bytes = 0;
    Serial.println(sendMessage);

sendMessage.toUpperCase();
unsigned long timer;


    uint bufferEnd=0;
    i2s_zero_dma_buffer(I2S_NUM_1);
    delay(10);
    i2s_start(I2S_NUM_1);
    
//STOP BIT
i2sWriteTone(MARK_FREQUENCY,bufferEnd ,samplesPerBit*1.5);

  for (int k=0;k<sendMessage.length();k++){

   Serial.println(sendMessage[k]);
//START BIT
    i2sWriteTone(SPACE_FREQUENCY,bufferEnd ,samplesPerBit);
 
    char c=sendMessage[k];
    uint8_t value=baudotEncode(c);

    for(int j=0;j<5;j++){
      if((value>>j) & 0b00000001){
        i2sWriteTone(MARK_FREQUENCY,bufferEnd ,samplesPerBit);
      }
      else{
        i2sWriteTone(SPACE_FREQUENCY,bufferEnd ,samplesPerBit);
      }
    }

//STOP BIT

    i2sWriteTone(MARK_FREQUENCY,bufferEnd ,samplesPerBit*1.5);

 }
 
delay(200);
i2s_stop(I2S_NUM_1);
 
}
//this receive is for inmp441 microphone
#define SQUELCH 1948000
bool getSampleData(){
  bool squelch=false;
  size_t bytes = 0;
     i2s_read(I2S_NUM_0, rxSamples, sizeof(int32_t)*RX_BUFFER_SIZE, &bytes, portMAX_DELAY);

    for(int i=0;i<RX_BUFFER_SIZE;i++){
      if(rxSamples[i] > SQUELCH){
        squelch=true;
        break;
      }     
    }

  return squelch; 
   

}
 const uint8_t markZero = SAMPLE_RATE_MIC/MARK_FREQUENCY/2;
  const uint8_t spaceZero = SAMPLE_RATE_MIC/SPACE_FREQUENCY/2;


int8_t readBit(int32_t samples[], uint16_t len){
   uint16_t zeros[2]={0,len};
   uint16_t count=0;
   uint16_t diff=0;
   getSampleData(); 
    
    for(uint16_t i=1;i<len;i++){
    if( samples[i-1]>0 && samples[i]<0  ||  samples[i-1]<0 && samples[i]>0)zeros[count++]=i;//read half cycles
    if(count==2)break;        
    }
    diff=zeros[1]-zeros[0];       
    if((diff <= (markZero+1))  && (diff >= (markZero-1)))return 1; //bandpass filter  
    if((diff <= (spaceZero+1)) && (diff >= (spaceZero-1)))return 0;   
  return -1;  

}
char receiveByte(){
    uint8_t count;
unsigned long startTime,endTime;
    uint8_t newchar=0b00000000;


  int8_t newbit=0;
  int8_t bits=0;

//about 1-2 ms per read
     startTime=micros();

   ///for(count=0;count<200;count++)if(readBit(rxSamples,RX_BUFFER_SIZE)==0)break; ///one start bit
   for(count=0;count<500;count++)if(readBit(rxSamples,RX_BUFFER_SIZE)==1)break; ///one start bit
    if (count==500)return 0;
    startTime=micros();
    while(readBit(rxSamples,RX_BUFFER_SIZE)==1);    
 //   if(millis()-startTime < bitTime*1.5)return 0;//make sure its a stop bit
    delayMicroseconds(bitTime*1.5);// skip 0

    for(bits=0;bits<5;bits++){//5 data bits
      newbit=readBit(rxSamples,RX_BUFFER_SIZE);
      //Serial.print(newbit);
      if(newbit==-1)newbit=readBit(rxSamples,RX_BUFFER_SIZE);//error correction
      if(newbit==-1)newbit=readBit(rxSamples,RX_BUFFER_SIZE);//error correction
      if(newbit==-1 )return 0;
      newchar |= (bool(newbit) << bits);
       delayMicroseconds(bitTime);
    }
//    delay(bitTime);
//    for(uint8_t i=0;i<200;i++)if(readBit(rxSamples,RX_BUFFER_SIZE)==0)break;//zero stop bit
      //   Serial.println();
        return baudot[newchar];
 
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  if(ESP_OK!=i2s_driver_install(I2S_NUM_0, &i2s_mic_config, 0, NULL)){
    Serial.println("setup fail");
    
  }
  if(ESP_OK!=i2s_set_pin(I2S_NUM_0, &i2s_mic_pins)){
    Serial.println("setup fail");

  }
  
  if(ESP_OK!=i2s_driver_install(I2S_NUM_1, &i2s_out_config, 0, NULL)){
    Serial.println("setup fail");
    
  }
  if(ESP_OK!=i2s_set_pin(I2S_NUM_1, &i2s_out_pins)){
    Serial.println("setup fail");

  }
  Serial.println("setup success");
//create lookup table  


//count=0;
  display.init();
  //display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_RIGHT);


}
 


void loop() {
unsigned long startTime,endTime;



//receive
  String rxMessage;
  while(getSampleData()){

    char c=receiveByte();

   if(c){
      rxMessage+=c;     
       Serial.print(c);
   }     
  }
  if (rxMessage.length()){
  display.clear();
  display.drawString(128, 20,rxMessage);
  display.display();
    rxMessage="";
  }
  
//transmit
  if(Serial.available()){
    char ch=Serial.read();
//    Serial.print(ch);
    if(ch=='\n'){
     transmitFSK(message);
      ch='\0';
      message='\0';
    }
    else{
      message+=ch;
    }    
  }
}




