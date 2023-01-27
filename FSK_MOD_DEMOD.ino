#include "SSD1306Wire.h" // legacy include: `#include "SSD1306.h"`
#include "OLEDDisplayUi.h"
SSD1306Wire display(0x3c, SDA, SCL);  // ADDRESS, SDA, SCL  -  SDA and SCL usually populate automatically based on your board's pins_arduino.h e.g. https://github.com/esp8266/Arduino/blob/master/variants/nodemcu/pins_arduino.h
OLEDDisplayUi ui     ( &display );

#include <driver/i2s.h>
//#include <Goertzel.h>


#define LED_BUILTIN 2
//these settings are very sensitive
#define TX_BUFFER_SIZE 4096
#define RX_BUFFER_SIZE 64
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
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0};

i2s_pin_config_t i2s_mic_pins = {
    .mck_io_num = I2S_PIN_NO_CHANGE,
    .bck_io_num = I2S_MIC_SERIAL_CLOCK,
    .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SERIAL_DATA};

//    Bell 202 AFSK uses a 1200 Hz tone for mark (typically a binary 1) and 2200 Hz for space (typically a binary 0).

///200 bw
//#define MARK_FREQUENCY 1600
//#define SPACE_FREQUENCY 1400
#define MARK_FREQUENCY 2200
#define SPACE_FREQUENCY 1200
/*
hw_timer_t * timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
volatile uint32_t isrCounter = 0;
volatile uint32_t lastIsrAt = 0;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
*/
//int freqs[8]={697,770,852,941,1209,1336,1477,1633};



const int baud=45;
const int bitTime =1000*(1.0/(float)baud)+2;

const uint samplesPerBit =SAMPLE_RATE_AMP/(baud-1);   


int32_t rxSamples[RX_BUFFER_SIZE];
int32_t txSamples[TX_BUFFER_SIZE];

/*
i found 2 different sets of baudot code

Binary 	Decimal 	Hex 	Octal 	Letter 	Figure
00000 	0 	0 	0 	Blank 	Blank
00001 	1 	1 	1 	T 	5
00010 	2 	2 	2 	CR 	CR
00011 	3 	3 	3 	O 	9
00100 	4 	4 	4 	Space 	Space
00101 	5 	5 	5 	H 
00110 	6 	6 	6 	N 	,
00111 	7 	7 	7 	M 	.
01000 	8 	8 	10 	Line Feed 	Line Feed
01001 	9 	9 	11 	L 	)
01010 	10 	A 	12 	R 	4
01011 	11 	B 	13 	G 	&
01100 	12 	C 	14 	I 	8
01101 	13 	D 	15 	P 	0
01110 	14 	E 	16 	C 	:
01111 	15 	F 	17 	V 	;
10000 	16 	10 	20 	E 	3
10001 	17 	11 	21 	Z 	"
10010 	18 	12 	22 	D 	$
10011 	19 	13 	23 	B 	?
10100 	20 	14 	24 	S 	BEL
10101 	21 	15 	25 	Y 	6
10110 	22 	16 	26 	F 	!
10111 	23 	17 	27 	X 	/
11000 	24 	18 	30 	A 	-
11001 	25 	19 	31 	W 	2
11010 	26 	1A 	32 	J 	'
11011 	27 	1B 	33 	Figure Shift 	
11100 	28 	1C 	34 	U 	7
11101 	29 	1D 	35 	Q 	1
11110 	30 	1E 	36 	K 	(
11111 	31 	1F 	37 	Letter Shift 	
# 	Ltr 	Fig 	Hex 	Bin 	 
0 	NUL 	00 	000·00 	NULL, Nothing (blank tape)
1 	E 	3 	01 	000·01 	 
2 	LF 	02 	000·10 	Line Feed (new line)
3 	A 	- 	03 	000·11 	 
4 	SP 	04 	001·00 	Space
5 	S 	' 	05 	001·01 	 
6 	I 	8 	06 	001·10 	 
7 	U 	7 	07 	001·11 	 
8 	CR 	08 	010·00 	Carriage Return
9 	D 	ENC 	09 	010·01 	Enquiry (Who are you?, WRU)
10 	R 	4 	0A 	010·10 	 
11 	J 	BEL 	0B 	010·11 	BELL (ring bell at the other end)
12 	N 	, 	0C 	011·00 	 
13 	F 	! 	0D 	011·01 	Can also be %
14 	C 	: 	0E 	011·10 	 
15 	K 	( 	0F 	011·11 	 
16 	T 	5 	10 	100·00 	 
17 	Z 	+ 	11 	100·01 	 
18 	L 	) 	12 	100·10 	 
19 	W 	2 	13 	100·11 	 
20 	H 	$ 	14 	101·00 	Currency symbol — Can also be £
21 	Y 	6 	15 	101·01 	 
22 	P 	0 	16 	101·10 	 
23 	Q 	1 	17 	101·11 	 
24 	O 	9 	18 	110·00 	 
25 	B 	? 	19 	110·01 	 
26 	G 	& 	1A 	110·10 	Can also be @ 
27 	FIGS 	1B 	110·11 	Figures (Shift on)
28 	M 	. 	1C 	111·00 	 
29 	X 	/ 	1D 	111·01 	 
30 	V 	; 	1E 	111·10 	 
31 	LTRS 	1F 	11·111 	Letters (Shift off) 
*/
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
/*
void  generateTone(int32_t *buff,uint16_t f,uint &last,uint16_t len){
  uint16_t gain=8000;
  
  for(uint16_t i=0;i<len;i++){
      buff[i] = gain * sin(((float) last * M_PI * 2 * f) / (float) (SAMPLE_RATE_AMP));
      last++;      
  }
}
*/
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

void transmitFSK(){
  size_t bytes = 0;
    Serial.println(message);

message.toUpperCase();
unsigned long timer;


    uint bufferEnd=0;
    i2s_zero_dma_buffer(I2S_NUM_1);
    i2s_start(I2S_NUM_1);
    
//STOP BIT
i2sWriteTone(MARK_FREQUENCY,bufferEnd ,samplesPerBit*1.5);

  for (int k=0;k<message.length();k++){

   Serial.println(message[k]);
//START BIT
    i2sWriteTone(SPACE_FREQUENCY,bufferEnd ,samplesPerBit);
 
    char c=message[k];
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
#define SQUELCH 1748000
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
 const uint8_t markZero = SAMPLE_RATE_MIC/MARK_FREQUENCY;
  const uint8_t spaceZero = SAMPLE_RATE_MIC/SPACE_FREQUENCY;


int8_t readBit(int32_t samples[], uint16_t len){
   uint16_t zeros[2]={0,len};
   uint16_t count=0;
   uint16_t diff=0;
   getSampleData(); 
    
    for(uint16_t i=1;i<len;i++){
    if(samples[i-1]>0 && samples[i]<0 )zeros[count++]=i;
    if(count==2)break;        
    }
    diff=zeros[1]-zeros[0];       
    if((diff <= (markZero+2))  && (diff >= (markZero-2)))return 1; //bandpass filter  
    if((diff <= (spaceZero+2)) && (diff >= (spaceZero-2)))return 0;   
  return -1;  

}
/*
volatile int8_t newBit;
void ARDUINO_ISR_ATTR onTimer(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  newBit=readBit(rxSamples,RX_BUFFER_SIZE);
  isrCounter++;
  lastIsrAt = millis();
  portEXIT_CRITICAL_ISR(&timerMux);
  // Give a semaphore that we can check in the loop
  xSemaphoreGiveFromISR(timerSemaphore, NULL);
  // It is safe to use digitalRead/Write here if you want to toggle an output
}
*/
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

uint count=0;

//count=0;
Serial.println(count);
  display.init();
  //display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_RIGHT);

/*
// Create semaphore to inform us when the timer has fired
  timerSemaphore = xSemaphoreCreateBinary();
  
 // Use 1st timer of 4 (counted from zero).
  // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more
  // info).
  timer = timerBegin(0, 80, true);

  // Attach onTimer function to our timer.
  timerAttachInterrupt(timer, &onTimer, true);

  // Set alarm to call onTimer function every second (value in microseconds).
  // Repeat the alarm (third parameter)
  timerAlarmWrite(timer, 2200, true);

  // Start an alarm
 // timerAlarmEnable(timer);  
*/
}
 

char receiveByte(){
    uint8_t count;
unsigned long startTime,endTime;
    uint8_t newchar=0b00000000;


  int8_t newbit=0;
  int8_t bits=0;

//about 1-2 ms per read
     startTime=millis();

   ///for(count=0;count<200;count++)if(readBit(rxSamples,RX_BUFFER_SIZE)==0)break; ///one start bit
   for(count=0;count<200;count++)if(readBit(rxSamples,RX_BUFFER_SIZE)==1)break; ///one start bit
    if (count==200)return 0;
    while(readBit(rxSamples,RX_BUFFER_SIZE)==1);    

        delay(bitTime);// skip 0

    for(bits=0;bits<5;bits++){//5 data bits
     startTime=millis();
      newbit=readBit(rxSamples,RX_BUFFER_SIZE);
      //Serial.print(newbit);
      if(newbit==-1)newbit=readBit(rxSamples,RX_BUFFER_SIZE);//error correction
      if(newbit==-1 )return 0;
      newchar |= (bool(newbit) << bits);
       delay(bitTime);
    }
//    delay(bitTime);
//    for(uint8_t i=0;i<200;i++)if(readBit(rxSamples,RX_BUFFER_SIZE)==0)break;//zero stop bit
      //   Serial.println();
        return baudot[newchar];
 
}

void loop() {
unsigned long startTime,endTime;


/*
if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE){
    uint32_t isrCount = 0, isrTime = 0;
    int8_t newbit=0;
    // Read the interrupt count and time
    portENTER_CRITICAL(&timerMux);
    isrCount = isrCounter;
    isrTime = lastIsrAt;
    newbit =newBit;
    portEXIT_CRITICAL(&timerMux);
    // Print it
*/
/*
    Serial.print("onTimer no. ");
    Serial.print(isrCount);
    Serial.print(" at ");
    Serial.print(isrTime);

    Serial.print("ms ");
    Serial.print(" bit ");

    Serial.println(newbit);
  }
*/
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
     transmitFSK();
      ch='\0';
      message='\0';
    }
    else{
      message+=ch;
    }    
  }
}



