///bluetooth speaker with fm transmitter lcd and buttons
#pragma GCC optimize("Ofast")

#include <Ticker.h>
#include <FMTX.h>
#include "arduinoFFT.h"

////check display driver
#include <SH1106Wire.h>        // legacy: #include "SSD1306.h"
#include <Preferences.h>

//taken from https://github.com/tierneytim/btAudio
// bluetooth, config, discover and audio
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"

// the audio DAC and amp configuration. 
#include "driver/i2s.h"



#define KEYPAD_INPUT1 25
#define KEYPAD_INPUT2 33
#define KEYPAD_INPUT3 32

#define BCK_PIN 27
#define WS_PIN 14
#define OUT_PIN 26

Preferences preferences;
///////////////////////////////////////
// INITIALIZE DISPLAY
///////////////////////////////////////

SH1106Wire display(0x3c,SDA,SCL);   // ADDRESS, SDA, SCL PINS 21 AND 22


////////////////////////////////////
//   MENU FUNCTIONS
////////////////////////////////////



float frequency=105;
//float track=1;
float onoff=1;
float power=4;
float visnum =0;

int16_t byteCount = 0;
bool displayScope=false;

const int16_t fftsamples =64;
double vReal[fftsamples];
double vImag[fftsamples];

ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, fftsamples, 44100);

void displayMenuItem(uint16_t x,uint16_t y, String s){
 if(!displayScope)display.drawString(x,y,s);
//display.display();
// Serial.println(c);
}

void bt_data_cb(const uint8_t *data, uint32_t len){
   // number of 16 bit samples
   int n = len/2;
   
   // point to a 16bit sample 
   int16_t* data16=(int16_t*)data;
   
   // create a variable (potentially processed) that we'll pass via I2S 
   int16_t fy; 
   
   // Records number of bytes written via I2S
   size_t i2s_bytes_write = 0;

   for(int i=0;i<n;i++){
    // put the current sample in fy
    fy=*data16;

    
      if(visnum==0){
        if(byteCount<display.width()){
          if(i%2 == 0)display.setPixel(byteCount++, fy/128+32);
        }
      }
      else if(visnum==1){
        if(byteCount<fftsamples){
          if(i%2 == 0){
            vReal[byteCount]=fy;
            vImag[byteCount]=0.0;
            byteCount++;
          }
        }
      }
        
    //making this value larger will decrease the volume(Very simple DSP!). 
//    fy/=1;
    
    // write data to I2S buffer 
    i2s_write(I2S_NUM_0, &fy, 2, &i2s_bytes_write,  portMAX_DELAY );
    
    //move to next memory address housing 16 bit data 
    data16++;
   }

}
Ticker flipscreen;
void animation(){

  if(displayScope){
    if(visnum==1){
      FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);	
      FFT.compute(FFTDirection::Forward); 
      //FFT.complexToMagnitude();
      int w=display.width()/(fftsamples/2);
      for (int i=0 ;i<display.width()/w;i++){
        int m=map(vReal[i],0,32767,0,64);
        int x= i*w;
        display.drawLine(x, m,x+w,m);
      }
    }
    display.display();
    display.clear();
    byteCount=0;
  }

}


void restoreEEPROM(){
  preferences.begin("my-app", true); // Open for read
  frequency = preferences.getFloat("frequency", 0); // Get int, default to 0
  //onoff = preferences.getFloat("onoff", 0); // Get int, default to 0
  power = preferences.getFloat("power", 0); // Get int, default to 0
  preferences.end(); // Close the preferences object
}

/////////////////////////////////////////////
// THE FUNCTIONS CALLED FROM THE MENU
////////////////////////////////////////////
void visNumUp(float *v){
  if (visnum<1)visnum++;
}
void visNumDown(float *v){
  if(visnum>0)visnum--;
}

void txPowerUp(float *v){
  if(*v>0)*v-=1;
  fmtx_set_rfgain(*v);
}
void txPowerDown(float *v){
  if(*v<15)*v+=1;
  fmtx_set_rfgain(*v);
}

void txOn(float *v){
  onoff=1;
//  sleepModeOff();
}

void txOff(float *v){
  onoff=0;
//  sleepModeOn();
}

void freqUp(float* v){
  if(*v<108)*v+=0.1;
  else *v=70;
  fmtx_set_freq(*v);
}
void freqDown(float* v){
  if(*v>70)*v-=0.1;
  else *v = 108;
  fmtx_set_freq(*v);
}

void saveMenu(){
  /////ESP32 ONLY
  preferences.begin("my-app", false); // Open for read and write

  preferences.putFloat("frequency", frequency );
  //preferences.putFloat("onoff", onoff);
  preferences.putFloat("power", power);

  preferences.end(); // Close the preferences object 
}
void visualize(){

  displayScope=!displayScope;

}
void trackUp(float *v){

}

void trackDown(float *v){

}



////////////////////////////////////////////////////////////
//   MENU BEGINS
//  this is for each menu item
//////////////////////////////////////////////////////////////

struct lcdMenuItem{
    bool active=false;
    const char *label;
    float *value;
    void (*actFunc)(void)=NULL;
    void (*downFunc)(float*)=NULL;
    void (*upFunc)(float*)=NULL;
};
////////////////////////////////////
// ROOT OF MENU
///////////////////////////////////

class lcdMenu{
  public:
    int len=0;
    int select=0;
    void (*displayFunc)(uint16_t, uint16_t,String)=NULL;
    bool updateMenu=true,lcdUpSelected=false,lcdDownSelected=false,lcdSwitchSelected=false;
    lcdMenuItem *menu[16];

    lcdMenu(void (*displaytextfunction)(uint16_t, uint16_t,String)){
      len=0;
      select=0;  
      displayFunc=displaytextfunction;
     }

    void lcdDisplay(){
      display.clear();
      int x=16;
      int y=0;
      for(uint8_t i=select;i<len;i++){
        if(y>64)break;
        if(i==select)displayFunc(0,y,">");  
        displayFunc(x,y,menu[i]->label);

///display text to 4 decimal places

        if(menu[i]->actFunc==NULL && menu[i]->active){
          String t=String(*menu[i]->value);
          displayFunc(80,y,t);
        } 
        y+=16;
      }
      display.display();
      updateMenu=false;
    }
    void serialDisplay(){
      
      for(uint8_t i=0;i<len;i++){
        if(i==select){
          if(menu[select]->active)Serial.print(">");
          else Serial.print("#");
        }
        else Serial.print(" ");  
        Serial.print(menu[i]->label);
        if(menu[i]->active){
          Serial.print("   ");
          Serial.print(*menu[i]->value);
        } 
        Serial.println();
      }
      Serial.println("\n\n");
      updateMenu=false;
    }
    void lcdAddMenu(const char *t,void (*act)(void),void (*up)(float*),void (*down)(float*),float *v){
      menu[len]=new lcdMenuItem;
      menu[len]->label=t;
      menu[len]->actFunc=act;
      menu[len]->upFunc=up;
      menu[len]->downFunc=down;
      menu[len]->value=v;
      len++;  
    }
    void lcdUp(){
    
       if(menu[select]->active==true){
        if(menu[select]->upFunc!=NULL)menu[select]->upFunc(menu[select]->value);
       }
       else {
        if(select<len-1)select++;
       }

      updateMenu=true;
      lcdUpSelected=false;
    }
    void lcdDown(){
       if(menu[select]->active==true){
        if(menu[select]->downFunc!=NULL)menu[select]->downFunc(menu[select]->value);
       }
       else{
        if(select>0) select--;
       } 
      updateMenu=true;
      lcdDownSelected=false;
    }
    void lcdSelect(){
      if(menu[select]->active){
        menu[select]->active=false;
//        lcdSave();
      }
      else menu[select]->active=true;

      if(menu[select]->actFunc!=NULL){
        menu[select]->actFunc();
      }
      updateMenu=true;
      lcdSwitchSelected=false;
    }
};

////////////////////////////////////////
//  DECLARE MENU
////////////////////////////////////////

lcdMenu mainMenu(displayMenuItem);

long lastSwitchtime;
#define SwitchDelay 200


void switchplus(){
  if(millis()-lastSwitchtime > SwitchDelay){
    mainMenu.lcdUpSelected=true;
    lastSwitchtime=millis();
  }
}

void switchminus(){
  if(millis()-lastSwitchtime > SwitchDelay){
    mainMenu.lcdDownSelected=true;
    lastSwitchtime=millis();
  }
}

void switchPress(){
  if(millis()-lastSwitchtime > SwitchDelay){
    mainMenu.lcdSwitchSelected=true;
    lastSwitchtime=millis();
  }
}
void fmtx_off(){
  uint8_t reg=fmtx_read_reg(0x0b);
  reg |= 0b00100000;
  fmtx_write_reg(0x0b, reg);
}

void fmtx_on(){
  uint8_t reg=fmtx_read_reg(0x0b);
  reg &= 0b11011111;//zero is on
  fmtx_write_reg(0x0b, reg);
}

void sleepModeOn(){
  onoff=0;
  Serial.println("going to sleep");
//  display.displayOff();
  fmtx_off();
//  dfmp3.sleep();
//  dfmp3.stop();

//  esp_sleep_enable_timer_wakeup((uint64_t) * 1000000);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, 0);
  esp_deep_sleep_start();
}

void setup() {

  Serial.begin(115200);

  // i2s configuration
  static const i2s_config_t i2s_config = {
    .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 48000,//44100
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = static_cast<i2s_comm_format_t>(I2S_COMM_FORMAT_I2S|I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // default interrupt priority
    .dma_buf_count = 8,
    .dma_buf_len = 1000,
    .use_apll = false,
    .tx_desc_auto_clear = true
  };
  // i2s pinout
  static const i2s_pin_config_t pin_config = {
    .bck_io_num = BCK_PIN,//27
    .ws_io_num = WS_PIN,//14
    .data_out_num = OUT_PIN, //26
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  
  // now configure i2s with constructed pinout and config
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
 // i2s_set_clk(I2S_NUM_0, 44100, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
 // i2s_set_sample_rates(I2S_NUM_0, 44100);
  
 // set up bluetooth classic via bluedroid
  btStart();
  esp_bluedroid_init();
  esp_bluedroid_enable();

 
  // set up device name
  const char *dev_name = "ESP_SPEAKER";
  esp_bt_dev_set_device_name(dev_name);

  // initialize A2DP sink and set the data callback(A2DP is bluetooth audio)
  esp_a2d_sink_register_data_callback(bt_data_cb);
  esp_a2d_sink_init();
  
  // set discoverable and connectable mode, wait to be connected
  esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE,ESP_BT_GENERAL_DISCOVERABLE); 

  flipscreen.attach(0.1,animation);
  restoreEEPROM();
///////////////////////////////////
//  ADD MENUITEMS TO MENU
////////////////////////////////////
  mainMenu.lcdAddMenu("frequency", NULL,freqUp,freqDown,&frequency);
  mainMenu.lcdAddMenu("TX power", NULL, txPowerUp, txPowerDown, &power);
  mainMenu.lcdAddMenu("visualize",visualize,NULL,NULL,NULL);
  mainMenu.lcdAddMenu("visualization",NULL,visNumUp,visNumDown,&visnum);
  mainMenu.lcdAddMenu("save",saveMenu,NULL,NULL,NULL);
  mainMenu.lcdAddMenu("on-off", NULL,txOn,txOff,&onoff);

//////////////////////////////////////
//menu buttons
/////////////////////////////////////////

  pinMode(KEYPAD_INPUT1,INPUT_PULLUP);
  pinMode(KEYPAD_INPUT2,INPUT_PULLUP);
  pinMode(KEYPAD_INPUT3, INPUT_PULLUP);
  attachInterrupt(KEYPAD_INPUT1, switchplus, FALLING);
  attachInterrupt(KEYPAD_INPUT2, switchminus, FALLING);
  attachInterrupt(KEYPAD_INPUT3, switchPress,FALLING);


//////////////////////
//  DISPLAY

  display.init();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.flipScreenVertically();
  mainMenu.lcdDisplay();
//////////////////////
// FM TRANSMITTER
//////////////////////////

  fmtx_init(frequency, USA); 
//  fmtx_set_au_enhance();
////////////////////////////////////
// WAKE PERIPHERALS UP FROM SLEEP
///////////////////////////////////
//  randomSeed(analogRead(36));
  //track=random(0,trackCount);

// display.displayOn();
//      display.clear();
//      display.display();
  fmtx_on();


}

void waitMilliseconds(uint16_t msWait)
{
  uint32_t start = millis();
  
  while ((millis() - start) < msWait)
  {
    // if you have loops with delays, its important to 
    // call dfmp3.loop() periodically so it allows for notifications 
    // to be handled without interrupts
//    dfmp3.loop(); 
    delay(1);
  }
}

void loop() {


   if(mainMenu.updateMenu)mainMenu.lcdDisplay();
  if(mainMenu.lcdUpSelected)mainMenu.lcdUp();
  if(mainMenu.lcdDownSelected)mainMenu.lcdDown();
  if(mainMenu.lcdSwitchSelected)mainMenu.lcdSelect();
  waitMilliseconds(100);
//  delay(10);
}
