//#pragma GCC optimize("Ofast")
//#pragma GCC target("avx,avx2,fma")
#include <stdint.h>
#include <driver/i2s.h>
#include <math.h>


#define TRANSMIT_PIN 25
#define RECEIVE_PIN 36 //adc1_0
#define LED_BUILTIN 2

#define SAMPLE_RATE 76800
#define OVERSAMPLING 4

//    Bell 202 AFSK uses a 1200 Hz tone for mark (typically a binary 1) and 2200 Hz for space (typically a binary 0).

#define MARK_FREQ 1200
#define SPACE_FREQ 2200

//ADC circuit
/*
                      ^ +3.3V
                      |
                      _
                     | |10k
                     |_|
                      |            | |10uF
   GPIO34-------------*------------| |----------- line in
(Default ADC pin)     |           +| |      
                      |
                      _
                     | |10k
                     |_|
                      |
                      |
                      V GND

*/
//this was figured by an analog read on the pin

#define ZERO_LEVEL 1865
#define SAMPLE_BUFFER_SIZE 1024
#define BIT_SIZE 16


const int16_t markSamples=(SAMPLE_RATE/OVERSAMPLING)/MARK_FREQ;
const int16_t spaceSamples=(SAMPLE_RATE/OVERSAMPLING)/SPACE_FREQ;

//19200 effective rate


const float loI[16]={  0  , 0.6593 ,  0.9914 ,  0.8315 ,  0.2588 , -0.4423  ,-0.9239  ,-0.9469 , -0.5000 ,  0.1951 ,  0.7934  , 0.9979 ,  0.7071 ,  0.0654 , -0.6088 , -0.9808};
const float loQ[16]={     1.000000 ,  0.751840 ,  0.130526 , -0.555570 , -0.965926  ,-0.896873 , -0.382683 ,  0.321439 ,  0.866025 ,  0.980785 ,  0.608761 , -0.065403 , -0.707107 , -0.997859 , -0.793353 , -0.195090 };
const float hiI[16]={          0   ,0.3827   ,0.7071 ,  0.9239 ,  1.0000  , 0.9239 ,  0.7071 ,  0.3827,  -0.0000,  -0.3827 , -0.7071 , -0.9239 , -1.0000 , -0.9239 , -0.7071 , -0.3827 };
const float hiQ[16]={     1.0000e+00 ,  9.2388e-01 ,  7.0711e-01 ,  3.8268e-01,  -1.6081e-16,  -3.8268e-01,  -7.0711e-01  ,-9.2388e-01 , -1.0000e+00 , -9.2388e-01,  -7.0711e-01 , -3.8268e-01  ,   7.0448e-16 ,  3.8268e-01 ,  7.0711e-01 ,  9.2388e-01 };

#define SQUELCH 30

#define BAUD 1200
const int bitTime =1000000*(1.0/BAUD);//time in us
const int samplesPerBit = SAMPLE_RATE/OVERSAMPLING/BAUD;


int16_t rawSamples[SAMPLE_BUFFER_SIZE];
int16_t sampleData[BIT_SIZE];


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


uint16_t raw_samples[SAMPLE_BUFFER_SIZE];


int8_t readBit(){
  size_t bytes = 0;

  int16_t sampleData[BIT_SIZE];

  i2s_read(I2S_NUM_0, rawSamples, sizeof(int16_t)*BIT_SIZE*OVERSAMPLING, &bytes, portMAX_DELAY);

  int16_t outloi = 0, outloq = 0, outhii = 0, outhiq = 0;

  for(int i=0;i<BIT_SIZE;i++){

    uint16_t index= i*OVERSAMPLING;
    sampleData[i] = (rawSamples[index]+ rawSamples[index + 1] + rawSamples[index + 2] + rawSamples[index + 3]) / OVERSAMPLING - ZERO_LEVEL;

    outloi += sampleData[i] * loI[i] ;
    outloq += sampleData[i] * loQ[i] ;
    outhii += sampleData[i] * hiI[i] ;
    outhiq += sampleData[i] * hiQ[i] ;
  }
  int16_t magMark=sqrt(outloi*outloi+outloq*outloq);
  int16_t magSpace=sqrt(outhii*outhii+outhiq*outhiq);
  int16_t diff=magMark-magSpace;

//  int16_t diff=sqrt((outloi * outloi + outloq * outloq) - (outhii * outhii + outhiq * outhiq));
//  printf("diff %d\n",diff);

  if (abs(diff)>SQUELCH)return (diff>0?0:1);

  return -1;
}

String txMessage;

void receiveASCII(void *v)
{


  uint64_t firstTicks, lastTicks;
  uint16_t newchar=0b0000000000000000;
  uint8_t newbit;
  //  String rxMessage;
int count;
   while(1){
     for( count=0;count<500;count++)if(readBit()==0)break; ///one start bit
     if (count==500) continue;
      newchar=0b0000000000000000;
      newbit=readBit();
      firstTicks=xTaskGetTickCount();
      for(int8_t bits=0;bits<8;bits++){
        newbit=readBit();
        if(newbit==-1)newbit=0;
        newchar |= ((bool)newbit << bits);

      }
      if(newchar>0){
//97=01100001=a
      for(int i=0;i<8;i++)printf("%d",(newchar>>i)&1);
      lastTicks=xTaskGetTickCount();
      printf(" %d %c %d\n",newchar&0xFF , newchar&0xFF ,lastTicks);
      vTaskDelay(1084702000-lastTicks);
      }
    }
}

//transmit
void transmitASCII(){
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

TaskHandle_t receiveHandle;


void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_11db);
  adc1_config_width(ADC_WIDTH_12Bit);
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_0);
  i2s_adc_enable(I2S_NUM_0);  
  xTaskCreatePinnedToCore(receiveASCII, "receive ascii", 32768, NULL, 1, &receiveHandle, 1);
//  xTaskCreatePinnedToCore(receiveASCII,"receive ascii",10000,NULL,configMAX_PRIORITIES,NULL);
/*
  display.init();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
*/
//Serial.println(ESP.getCpuFreqMHz());
}
 


void loop() {
transmitASCII();
//delay(1);
}



