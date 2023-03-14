//only works with these optimizations and usin both cores


#pragma GCC optimize("Ofast")
#pragma GCC target("avx,avx2,fma")

#include <driver/i2s.h>
#include <rom/ets_sys.h>

#define TRANSMIT_PIN 25
#define RECEIVE_PIN 34 //adc1_0
#define LED_BUILTIN 2
//    Bell 202 AFSK uses a 1200 Hz tone for mark (typically a binary 1) and 2200 Hz for space (typically a binary 0).
#define MARK_FREQ 2200
#define SPACE_FREQ 1200

#define SAMPLE_BUFFER_SIZE 352 // 16 samples per bit * 11 bits per character(2 stop bits)*2 samples averaged 352 samples per character
#define BIT_SIZE 16

//I need to sample at right rate to capture at 1200 bps
#define SAMPLE_RATE 38400


//19200 effective rate
const int8_t hiI[16]={  0  ,  84  , 127  , 106 ,   33 ,  -57 , -118  ,-121  , -64 ,   25 ,  102,   128,    91 ,    8 ,  -78,  -126};
const int8_t hiQ[16]={128  ,  96  ,  17  , -71 , -124 , -115 ,  -49  ,  41  , 111 ,  126 ,   78,    -8,   -91 , -128 , -102,   -25};
const int8_t loI[16]={  0  ,  49  ,  91  , 118 ,  128 ,  118 ,   91  ,  49  ,   0 ,  -49 ,  -91,  -118,  -128 , -118 ,  -91,   -49};
const int8_t loQ[16]={128  , 118  ,  91  ,  49 ,    0 ,  -49 ,  -91  ,-118  ,-128 , -118 ,  -91,   -49,     0 ,   49 ,   91,   118};

#define SQUELCH 2000

#define BAUD 1200
const int bitTime =1000000*(1.0/BAUD);//time in us
const int byteTime =11*bitTime;

int16_t rawSamples[SAMPLE_BUFFER_SIZE];
int16_t sampleDataSize;
int16_t sampleData[32000];//2000 bits


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


 const uint8_t markZero = SAMPLE_RATE/2/MARK_FREQ/2;
  const uint8_t spaceZero = SAMPLE_RATE/2/SPACE_FREQ/2;

void getSampleData(){
  bool squelch;
  size_t bytes = 0;

 i2s_read(I2S_NUM_0, rawSamples, sizeof(int16_t)*SAMPLE_BUFFER_SIZE, &bytes, portMAX_DELAY);

  for(int i=0;i<SAMPLE_BUFFER_SIZE;i+=2){ 
    sampleData[sampleDataSize]=((rawSamples[i] & 0x0FFF)+(rawSamples[i+1] & 0x0FFF))/2;
    sampleDataSize++;
  }
}


int8_t readBit(uint16_t index){
  float av; 
  float outloi = 0, outloq = 0, outhii = 0, outhiq = 0;
  int16_t sample;
  
  for(int i=0;i<BIT_SIZE;i++){ 
    av+=sampleData[index+i];
  }

  av/=BIT_SIZE;

  for(int i=0;i<BIT_SIZE;i++){
    sample=sampleData[index+i]-=av;
//    Serial.println(sample);
    outloi += sample * loI[i];
    outloq += sample * loQ[i];
    outhii += sample * hiI[i];
    outhiq += sample * hiQ[i];
  }
  float magl=sqrt(outloi*outloi+outloq*outloq);
  float magh=sqrt(outhii*outhii+outhiq*outhiq);
/*
Serial.print(magl);
Serial.print(" ");
Serial.print(magh);
Serial.print(" ");
Serial.println(" ");
*/
  if(magl>magh)return 0;
  return 1;


}

int8_t readBitZero(uint16_t index){
   float av; 
   uint16_t zeros[2]={0,BIT_SIZE};
   uint16_t count=0;
   uint16_t diff=0;
  int16_t sample[BIT_SIZE];

  for(int i=0;i<BIT_SIZE;i++){ 
    av+=sampleData[index+i];
  }

  av/=BIT_SIZE;

  for(int i=0;i<BIT_SIZE;i++){
    sample[i]=sampleData[index+i]-av;
  }    

  for(uint16_t i=1;i<BIT_SIZE;i++){      
    if( sample[i-1]>0 && sample[i]<0  ||  sample[i-1]<0 && sample[i]>0)zeros[count++]=i;//read half cycles
    if(count==2)break;        
  }
  diff=zeros[1]-zeros[0];  
//    Serial.println(diff);

  if((diff <= (markZero+1))  && (diff >= (markZero-1)))return 1; //bandpass filter  
  if((diff <= (spaceZero+1))  && (diff >= (spaceZero-1)))return 0; //bandpass filter  
//this one failed try the other one
  return readBit(index);   

}

//The approach used is to sample the incoming FSK signal at 9600 samples/second (8 times the 1200 Hz frequency used in Bell 202 modulation) and pass this through
// two digital filters, one tuned to 1200 Hz and the other to 2200 Hz.  The secret of the demodulation scheme is to implement the digital filters using tables of 
//Sine and Cosine values for each frequency.  The following bit of Java code shows the basic technique.  The data[] array passed to demodulate() contains audio FSK
// at 9600 Bps and the function is called for index = 0 through data.length - 8. After stepping though at least 8 samples, the value returned from demodulate() will
// be >0 if it has demodulated a 2200 Hz tone, or < 0 for a 1200 Hz tone.
/*
  const int8_t   coeffloi[] = { 64,  45,   0, -45, -64, -45,   0,  45};
  const int8_t  coeffloq[] = {  0,  45,  64,  45,   0, -45, -64, -45};
  const int8_t  coeffhii[] = { 64,   8, -62, -24,  55,  39, -45, -51};
  const int8_t  coeffhiq[] = {  0,  63,  17, -59, -32,  51,  45, -39};

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
void receiveASCII(void* v){
   uint16_t sample=0;
    size_t bytes=0;
    uint8_t newchar=0b00000000;
    int8_t newbit=0;
    uint16_t index=0;
 while(1){
   sample=0;
   bytes=0;
   newchar=0b00000000;
   newbit=0;
   index=0;

      
    i2s_read(I2S_NUM_0, &sample, sizeof(int16_t), &bytes, portMAX_DELAY);
    sample &=0x0FFF;
    if(sample > SQUELCH){

      getSampleData();//get one character worth of data

      while(readBitZero(index)==1){//find 0 start bit
        index+=BIT_SIZE;
      }

      //    Serial.print(readBit(index));

      index+=BIT_SIZE;

      for(uint8_t bits=0;bits<8;bits++){

        newbit=readBitZero(index);
      //        Serial.print(newbit);
        newchar |= (bool(newbit) << bits);

        index+=BIT_SIZE;


      }
      index+=BIT_SIZE*2;

      if(newchar>0)Serial.print((char)newchar);
      sampleDataSize=0;

   }



  
}


  
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_11db);
  adc1_config_width(ADC_WIDTH_12Bit);
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_6);//gpio34
  i2s_adc_enable(I2S_NUM_0);  
  xTaskCreate(receiveASCII,"receive ascii",10000,NULL,configMAX_PRIORITIES,NULL);
/*
  display.init();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
*/
//Serial.println(ESP.getCpuFreqMHz());
}
 
String txMessage;


void loop() {
/*
   uint16_t sample=0;
    size_t bytes=0;
    uint8_t newchar=0b00000000;
    int8_t newbit=0;
    uint16_t index=0;
 
    i2s_read(I2S_NUM_0, &sample, sizeof(int16_t), &bytes, portMAX_DELAY);
    sample &=0x0FFF;
    if(sample > SQUELCH){

      getSampleData();//get one character worth of data

      while(readBitZero(index)==1){//find 0 start bit
        index+=BIT_SIZE;
      }

      //    Serial.print(readBit(index));

      index+=BIT_SIZE;

      for(uint8_t bits=0;bits<8;bits++){

        newbit=readBitZero(index);
      //        Serial.print(newbit);
        newchar |= (bool(newbit) << bits);

        index+=BIT_SIZE;


      }
      index+=BIT_SIZE;
      if(newchar>0)Serial.print((char)newchar);
      sampleDataSize=0;
    }     
  
//}
*/



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

delay(10);
}




