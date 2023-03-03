#pragma GCC optimize("Ofast")
#pragma GCC target("avx,avx2,fma")

#include <driver/i2s.h>
/*
#include "SSD1306Wire.h" // legacy include: `#include "SSD1306.h"`
#include "OLEDDisplayUi.h"
SSD1306Wire display(0x3c, SDA, SCL);  // ADDRESS, SDA, SCL  -  SDA and SCL usually populate automatically based on your board's pins_arduino.h e.g. https://github.com/esp8266/Arduino/blob/master/variants/nodemcu/pins_arduino.h
OLEDDisplayUi ui     ( &display );
*/

#define TRANSMIT_PIN 25
#define RECEIVE_PIN 36 //adc1_0
#define LED_BUILTIN 2
//    Bell 202 AFSK uses a 1200 Hz tone for mark (typically a binary 1) and 2200 Hz for space (typically a binary 0).
#define MARK_FREQ 2200
#define SPACE_FREQ 1200

#define SAMPLE_BUFFER_SIZE 128
#define SAMPLE_RATE 96000

#define SQUELCH 100

const float baud=300;
const int bitTime =1000000*(1.0/baud);//time in us

int16_t rawSamples[SAMPLE_BUFFER_SIZE];
int16_t sampleData[SAMPLE_BUFFER_SIZE/4];


i2s_config_t i2s_config = {
      // I2S with ADC
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
      .intr_alloc_flags = 0,
      .dma_buf_count = 2,
      .dma_buf_len = SAMPLE_BUFFER_SIZE, 
      .use_apll = true,  
};




void transmitFSK(String sendMessage){
  size_t bytes = 0;
    Serial.println(sendMessage);

//sendMessage.toUpperCase();
    
//STOP BIT
  tone(TRANSMIT_PIN,MARK_FREQ);

  delayMicroseconds(bitTime*1.5);  
  for (int k=0;k<sendMessage.length();k++){

   Serial.println(sendMessage[k]);
//START BIT
  tone(TRANSMIT_PIN,SPACE_FREQ);
  delayMicroseconds(bitTime);  
 
    char c=sendMessage[k];

    for(int j=0;j<8;j++){
      if((c>>j) & 0b00000001){
        tone(TRANSMIT_PIN,MARK_FREQ);
        delayMicroseconds(bitTime);  
      }
      else{
        tone(TRANSMIT_PIN,SPACE_FREQ);
        delayMicroseconds(bitTime);  

      }
    }

//STOP BIT

  tone(TRANSMIT_PIN,MARK_FREQ);
  delayMicroseconds(bitTime*1.5);  
 }
  noTone(TRANSMIT_PIN);
 
  delay(200);
 
}


 const uint8_t markZero = SAMPLE_RATE/4/MARK_FREQ/2;
  const uint8_t spaceZero = SAMPLE_RATE/4/SPACE_FREQ/2;

bool getSampleData(){
  
  bool squelch=false;
  double av;
  size_t bytes = 0;

  
  esp_err_t result=i2s_read(I2S_NUM_0, rawSamples, sizeof(int16_t)*SAMPLE_BUFFER_SIZE, &bytes, portMAX_DELAY);

  int samples_read = bytes / sizeof(uint16_t);

  if (result != ESP_OK){
    Serial.println("read error");
    return false;
  }

  // dump the samples out to the serial channel.
  if(samples_read<SAMPLE_BUFFER_SIZE){
    Serial.println("read error");
    return false;
  }

  for(int i=0,j=0;i<SAMPLE_BUFFER_SIZE;i+=4){
    sampleData[j]=((rawSamples[i] & 0x0FFF)+(rawSamples[i+1] & 0x0FFF)+
                (rawSamples[i+2] & 0x0FFF)+(rawSamples[i+3] & 0x0FFF))/4;
    av+=sampleData[j];
    j++;
  }
  
    av=av/(sizeof(sampleData)/sizeof(sampleData[0]));
//    Serial.println(av);    
    for(int i=0;i<(SAMPLE_BUFFER_SIZE/4);i++){
      sampleData[i]-=av;
      if(sampleData[i] > SQUELCH)squelch=true;     
//      Serial.println(sampleData[i]);
    }

  return squelch;   

}

//The approach used is to sample the incoming FSK signal at 9600 samples/second (8 times the 1200 Hz frequency used in Bell 202 modulation) and pass this through
// two digital filters, one tuned to 1200 Hz and the other to 2200 Hz.  The secret of the demodulation scheme is to implement the digital filters using tables of 
//Sine and Cosine values for each frequency.  The following bit of Java code shows the basic technique.  The data[] array passed to demodulate() contains audio FSK
// at 9600 Bps and the function is called for index = 0 through data.length - 8. After stepping though at least 8 samples, the value returned from demodulate() will
// be >0 if it has demodulated a 2200 Hz tone, or < 0 for a 1200 Hz tone.
/*
  static byte   coeffloi[] = { 64,  45,   0, -45, -64, -45,   0,  45};
  static byte   coeffloq[] = {  0,  45,  64,  45,   0, -45, -64, -45};
  static byte   coeffhii[] = { 64,   8, -62, -24,  55,  39, -45, -51};
  static byte   coeffhiq[] = {  0,  63,  17, -59, -32,  51,  45, -39};

static int demodulate (int16_t data[], int idx) {
  int32_t outloi = 0, outloq = 0, outhii = 0, outhiq = 0;
  for (int ii = 0; ii < 8; ii++) {
    byte sample = data[ii + idx];
    outloi += sample * coeffloi[ii];
    outloq += sample * coeffloq[ii];
    outhii += sample * coeffhii[ii];
    outhiq += sample * coeffhiq[ii];
  }
  return (outhii >> 8) * (outhii >> 8) + (outhiq >> 8) * (outhiq >> 8) - (outloi >> 8) * (outloi >> 8) - (outloq >> 8) * (outloq >> 8);
}
*/

inline int8_t readBit(){
  uint16_t len =SAMPLE_BUFFER_SIZE/4;
   uint16_t zeros[2]={0,len};
   uint16_t count=0;
   uint16_t diff=0;
  
  getSampleData(); 
   
    for(uint16_t i=1;i<len;i++){
    if( sampleData[i-1]>0 && sampleData[i]<0  ||
      sampleData[i-1]<0 && sampleData[i]>0)zeros[count++]=i;//read half cycles
    if(count==2)break;        
    }
    diff=zeros[1]-zeros[0];       
    if((diff <= (markZero+2))  && (diff >= (markZero-2)))return 1; //bandpass filter  
    if((diff <= (spaceZero+2)) && (diff >= (spaceZero-2)))return 0;   
//return 0;    
  return -1;  

}

char receiveASCII(){
    uint8_t count;
unsigned long startTime,endTime;
    uint8_t newchar=0b00000000;


  int8_t newbit=0;
  int8_t bits=0;

//about 1-2 ms per read
//     startTime=micros();

   for(count=0;count<500;count++)if(readBit()==1)break;// wait for stop bit
    if (count==500)return 0;
    while(readBit()==1); //wait for stop bit to finish   
    delayMicroseconds(bitTime*1.2);// skip 0 start bit

    for(bits=0;bits<8;bits++){//7 data bits
//      startTime=micros();
      newbit=readBit();
      //Serial.print(newbit);
      if(newbit==-1)newbit=readBit();//error correction
      if(newbit==-1)newbit=readBit();//error correction
      if(newbit==-1)return 0;
      newchar |= (bool(newbit) << bits);
      delayMicroseconds(bitTime);
    }
//    delay(bitTime);
//    for(uint8_t i=0;i<200;i++)if(readBit(rawSamples,RX_BUFFER_SIZE)==0)break;//zero stop bit
      //   Serial.println();
        return newchar;
 

}

void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_11db);
  adc1_config_width(ADC_WIDTH_12Bit);
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_6);//gpio34
  i2s_adc_enable(I2S_NUM_0);  

/*
  display.init();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
*/
}
 
String txMessage;


void loop() {
unsigned long startTime,endTime;



//receive

  String rxMessage;
  char c;

  while(getSampleData()){

    c=receiveASCII();

   if(c){
      rxMessage+=c;     
   }     
  }
  if (rxMessage.length()){
    Serial.print(rxMessage);
//  display.clear();
//  display.drawString(128, 20,rxMessage);
//  display.display();
    c='\0';
    rxMessage=c;
  }
  
//transmit
  if(Serial.available()){
    char ch=Serial.read();
//    Serial.print(ch);
    if(ch=='\n'){
     transmitFSK(txMessage);
      ch='\0';
      txMessage='\0';
    }
    else{
      txMessage+=ch;
    }    
  }
}


