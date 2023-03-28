//#pragma GCC optimize("Ofast")
//#pragma GCC target("avx,avx2,fma")

#include <driver/i2s.h>


#define TRANSMIT_PIN 25
#define RECEIVE_PIN 34 //adc1_0
#define LED_BUILTIN 2
//    Bell 202 AFSK uses a 1200 Hz tone for mark (typically a binary 1) and 2200 Hz for space (typically a binary 0).
#define MARK_FREQ 1200
#define SPACE_FREQ 2200

//#define SAMPLE_RATE 38400
#define SAMPLE_RATE 76800
#define SQUELCH 2200
#define BAUD 1200
#define SAMPLE_BUFFER_SIZE 352 // 16 samples per bit * 11 bits per character(2 stop bits)*2 samples averaged 352 samples per character
#define SAMPLE_DATA_SIZE 32000 //about 90 characters

const int bitSize = ((SAMPLE_RATE/2)/BAUD);
const int bitTime = 1000000*(1.0/BAUD);//time in us

int16_t sampleData[SAMPLE_DATA_SIZE];
int16_t rawSamples[SAMPLE_BUFFER_SIZE];
int16_t sampleDataSize;

//19200 effective rate
//const int8_t hiI[16]={  0  ,  84  , 127  , 106 ,   33 ,  -57 , -118  ,-121  , -64 ,   25 ,  102,   128,    91 ,    8 ,  -78,  -126};
//const int8_t hiQ[16]={128  ,  96  ,  17  , -71 , -124 , -115 ,  -49  ,  41  , 111 ,  126 ,   78,    -8,   -91 , -128 , -102,   -25};
//const int8_t loI[16]={  0  ,  49  ,  91  , 118 ,  128 ,  118 ,   91  ,  49  ,   0 ,  -49 ,  -91,  -118,  -128 , -118 ,  -91,   -49};
//const int8_t loQ[16]={128  , 118  ,  91  ,  49 ,    0 ,  -49 ,  -91  ,-118  ,-128 , -118 ,  -91,   -49,     0 ,   49 ,   91,   118};

//const int16_t hiI[16]={0,   6593,   9914,   8315,   2588,  -4423,  -9239,  -9469,  -5000,   1951,   7934,   9979,   7071,    654,  -6088,  -9808};
//const int16_t hiQ[16]={10000,7518,    1305,   -5556,   -9659,   -8969,   -3827,    3214,    8660,    9808,    6088,    -654,   -7071,   -9979,   -7934,   -1951};
//const int16_t loI[16]={       0,    3827,    7071,    9239,   10000,    9239,    7071,    3827,       0,   -3827,   -7071,   -9239,  -10000,   -9239,   -7071,   -3827};
//const int16_t loQ[16]={10000,9239,    7071,    3827,       0,   -3827,   -7071,   -9239,  -10000,   -9239,  -7071 ,  -3827,       0,    3827,    7071 ,   9239};

//38400 rate
const int8_t hiI[32]={ 0, 45, 84,113,127,125,106, 75, 33,-13,-57,-93,-118,-128,-121,-99,-64,-21, 25, 68,102,122,128,117, 91, 53,  8,-37, -78,-109,-126, -126};
const int8_t hiQ[32]={128,120, 96, 60, 17,-29,-71,  -104,  -124,  -127,  -115,-87,-49, -4, 41, 81,111,126,126,109, 78, 37, -8,-53,-91,  -117,  -128,-122, -102,-68,-25, 21};
const int8_t loI[32]={ 0, 25, 49, 71, 91,106,118,126,128,126,118,106, 91, 71, 49, 25,  0,-25,-49,-71,-91,  -106,  -118,  -126,  -128,  -126,  -118,-106,-91,-71,-49,-25};
const int8_t loQ[32]={128,126,118,106, 91, 71, 49, 25,  0,-25,-49,-71,-91,  -106,  -118,  -126,  -128,  -126,  -118,  -106,-91,-71,-49,-25,  0, 25, 49,71, 91,106,118,126};

//const int16_t hiI[32]={0,3523,6593,8819,9914,9739,8315,5825,2588,-980,-4423,-7299,-9239,-9995,-9469,-7730,-5000,-1629,1951,5281,7934,9569,9979,  9109,7071,4127, 654,  -2903,  -6088,  -8492,  -9808,  -9866};
//const int16_t hiQ[32]={10000, 9359, 7518, 4714, 1305,-2271,-5556,-8128,-9659,-9952,-8969,-6836,-3827, -327, 3214, 6344, 8660, 9866, 9808, 8492,6088, 2903,-654,-4127,-7071,-9109,-9979,-9569,-7934,-5281,-1951, 1629};
//const int16_t loI[32]={0, 1951, 3827, 5556, 7071, 8315, 9239, 9808,10000, 9808, 9239, 8315, 7071, 5556, 3827, 1951, 0,-1951,-3827,-5556,  -7071,-8315,-9239,-9808,  -10000,-9808,-9239,-8315,-7071,-5556,-3827,-1951};
//const int16_t loQ[32]={10000, 9808, 9239, 8315, 7071, 5556, 3827, 1951, 0,-1951,-3827,-5556,-7071,-8315,-9239,-9808,-10000,-9808,-9239,-8315,  -7071,-5556,-3827,-1951, 0, 1951, 3827, 5556, 7071, 8315, 9239, 9808};


//const int16_t hiI[32]={0,   11541,   21603,   28896,   32485,   31909,   27243,   19085,    8480,   -3212,  -14492,  -23914,  -30271,  -32747,  -31026,  -25328,  -16382,   -5337,    6392,   17302,    25994,   31354,   32695,   29844,   23168,   13522,    2143,   -9511,  -19946,  -27824,  -32135,  -32327 };
//const int16_t hiQ[32]={32765 ,  30665 ,  24634  , 15445 ,   4277,   -7440,  -18203,  -26633,  -31649,  -32607,  -29386 , -22398,  -12539 ,  -1072,   10532,   20786,   28375,   32327,   32135  , 27824,     19946   , 9511   ,-2143  ,-13522 , -23168,  -29844 , -32695 , -31354 , -25994 , -17302 ,  -6392 ,   5337};
//const int16_t loI[32]={0 ,   6392,   12539,   18203,   23168,   27243,   30271,   32135,   32765 ,  32135,   30271,   27243,   23168,   18203,   12539,    6392,     0,   -6392,  -12539,  -18203 ,   -23168,  -27243  -30271,  -32135,  -32765,  -32135,  -30271,  -27243,  -23168,  -18203,  -12539,   -6392,  };
//const int16_t loQ[32]={32765,   32135,   30271 ,  27243,   23168,   18203,   12539,    6392,       0,   -6392,  -12539,  -18203,  -23168,  -27243,  -30271,  -32135,  -32765,  -32135,  -30271,  -27243,     -23168,  -18203,  -12539,   -6392,       0,    6392,   12539,   18203,   23168,   27243,   30271,   32135};

i2s_config_t i2s_config = {
      // I2S with ADC
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
      .intr_alloc_flags = 0,
      .dma_buf_count = 2,
      .dma_buf_len = 8, 
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


bool getSampleData(){
  size_t bytes = 0;
  bool squelch=false;
 i2s_read(I2S_NUM_0, rawSamples, sizeof(int16_t)*SAMPLE_BUFFER_SIZE, &bytes, portMAX_DELAY);

  for(int i=0;i<SAMPLE_BUFFER_SIZE;i+=2){ 
    sampleData[sampleDataSize]=((rawSamples[i] & 0x0FFF)+(rawSamples[i+1] & 0x0FFF))/2;
    if(sampleData[sampleDataSize]>SQUELCH)squelch=true;
    sampleDataSize++;
  }
  return squelch;  
}

void DCRemove(void){
  long av=0; 

  for(int i=0;i<sampleDataSize;i++){ 
    av+=sampleData[i];
  }

  av/=sampleDataSize;

  for(int i=0;i<sampleDataSize;i++){ 
    sampleData[i]-=av;
//    Serial.println(sampleData[i]);delay(100);
  }

}

int8_t readBit(uint16_t index){
  float outloi = 0, outloq = 0, outhii = 0, outhiq = 0;
  int16_t sample;
  

  for(int i=0;i<sizeof(loI)/sizeof(loI[0]);i++){
    sample=sampleData[index+i];
//    Serial.println(sample);delay(100);
    outloi += sample * (loI[i] );
    outloq += sample * (loQ[i] );
    outhii += sample * (hiI[i] );
    outhiq += sample * (hiQ[i] );
  }

  float magMark=sqrt(outloi*outloi+outloq*outloq);
  float magSpace=sqrt(outhii*outhii+outhiq*outhiq);
/*
//Serial.print(" {MARK=");
Serial.print(magMark);
Serial.print(" ");
Serial.print(magSpace);
Serial.print(" ");
Serial.println(" ");
*/

  if(magMark<10000 && magSpace <10000)return -1;
  if(magMark>magSpace)return 1;
  return 0;


}


 const uint8_t markZero = SAMPLE_RATE/2/MARK_FREQ/2;
 const uint8_t spaceZero = SAMPLE_RATE/2/SPACE_FREQ/2;

int8_t readBitZero(uint16_t index){
   float av; 
   uint16_t zeros[2]={0,bitSize};
   uint16_t count=0;
   uint16_t diff=0;
   int16_t sample[bitSize];
//remove average


  for(int i=0;i<bitSize;i++){
    sample[i]=sampleData[index+i];
//    Serial.println(sample[i]); delay(100);   
  }    

  for(uint16_t i=1;i<bitSize;i++){      
    if( ((sample[i-1]>0) && (sample[i]<0))  ||  ((sample[i-1]<0) && (sample[i]>0)))zeros[count++]=i;//read half cycles
    if(count==2)break;        
  }
  
  diff=zeros[1]-zeros[0];  
//    Serial.print(diff);
//    Serial.print(" ");

  if((diff <= (markZero+1))  && (diff >= (markZero-1)))return 1; //bandpass filter  
  if((diff <= (spaceZero+1))  && (diff >= (spaceZero-1)))return 0; //bandpass filter  
//this one failed try the other one
  return -1;   

}


String txMessage;
void receiveASCII(){
  uint16_t sample=0;
  size_t bytes=0;
  uint8_t newchar=0b00000000;
  int8_t newbit=0;
  uint16_t index=0;
  String rxMessage;    
   
  i2s_read(I2S_NUM_0, &sample, sizeof(int16_t), &bytes, portMAX_DELAY);
  sample &=0x0FFF;
  if(sample > SQUELCH){
    sampleDataSize=0;
    while(getSampleData() && (sampleDataSize<SAMPLE_DATA_SIZE));//get all data recorded
    DCRemove();
    index=0;
    while(index<sampleDataSize){//start bit decoder
      newbit=0;
      newchar=0b00000000;
      index+=bitSize; //skip stop bit
//      index+=bitSize; //skip stop bit
      
      while(readBit(index)==1){index+=bitSize/2;}//look for start bit
      for(int8_t bits=0;bits<8;bits++){
        index+=bitSize;
        newbit=readBit(index);
//        Serial.print(newbit);
        if(newbit==-1){newchar=0;break;}          
        newchar |= (bool(newbit) << bits);
      }

      if(newchar>0){
//        Serial.printf(" %X ",newchar);
        Serial.print((char)newchar);
        
      }
    }  //end bit decoder  

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

void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_11db);
  adc1_config_width(ADC_WIDTH_12Bit);
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_6);//gpio34
  i2s_adc_enable(I2S_NUM_0);  
//  xTaskCreate(receiveASCII,"receive ascii",10000,NULL,configMAX_PRIORITIES,NULL);
/*
  display.init();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
*/
//Serial.println(ESP.getCpuFreqMHz());
}
 


void loop() {
receiveASCII();
transmitASCII();
//delay(1);
}




