#include "SSD1306Wire.h"        // legacy: #include "SSD1306.h"

//rotary encoder

#define CLK 19
#define DT 18
#define SW 5


SSD1306Wire display(0x3c,SDA,SCL);   // ADDRESS, SDA, SCL  -  SDA and SCL usually populate automatically based on your board's pins_arduino.h e.g. https://github.com/esp8266/Arduino/blob/master/variants/nodemcu/pins_arduino.h

////////////////////////////////////
//   MENU FUNCTIONS
////////////////////////////////////

void displayMenuItem(uint16_t x,uint16_t y, String s){
 display.drawString(x,y,s);
display.display();
// Serial.println(c);
}


/////////////////////////////////////////////
// THE FUNCTIONS CALLED FROM THE MENU
////////////////////////////////////////////
float frequency=0;
float track=1;

void freqUp(float* v){
  if(*v<255)*v+=1;
  else *v=0;
}
void freqDown(float* v){
  if(*v>0)*v-=1;
  else *v = 255;
}

void trackUp(float *v){
  if(*v<255)*v+=10;
  else *v=0;
}

void trackDown(float *v){
  if(*v>0)*v-=10;
  else *v=255;
}

////////////////////////////////////////////////////////////
//   MENU BEGINS
//  these 2 classes are the whole menu
//  this is for each menu item
//////////////////////////////////////////////////////////////

struct lcdMenuItem{
    bool active=false;
    const char *label;
    float *value;
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
    bool updateMenu=true,lcdUpSelected=false,lcdDownSelected=false;
    lcdMenuItem *menu[16];

    lcdMenu(void (*displaytextfunction)(uint16_t, uint16_t,String)){
      len=0;
      select=0;  
      displayFunc=displaytextfunction;
     }

    void lcdDisplay(){
      display.clear();
      int x=20;
      int y=0;
      for(uint8_t i=0;i<len;i++){
        if(i==select)displayFunc(0,y,">");  
        displayFunc(x,y,menu[i]->label); 
        if(menu[i]->active)displayFunc(100,y,String(*menu[i]->value)); 
        y+=20;
      }
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
    void lcdAddMenu(const char *t,void (*up)(float*),void (*down)(float*),float *v){
      menu[len]=new lcdMenuItem;
      menu[len]->label=t;
      menu[len]->upFunc=up;
      menu[len]->downFunc=down;
      menu[len]->value=v;
      len++;  
    }
    void lcdUp(){
       if(menu[select]->active==true)menu[select]->upFunc(menu[select]->value);
       else {
        if(select<len-1)select++;
        else select=0;
       }
      updateMenu=true;
      lcdUpSelected=false;
    }
    void lcdDown(){
       if(menu[select]->active==true)menu[select]->downFunc(menu[select]->value);
       else{
        if(select>0) select--;
        else select=len-1;
       } 
      updateMenu=true;
      lcdDownSelected=false;
    }
    void lcdSelect(){
      if(menu[select]->active)menu[select]->active=false;
      else menu[select]->active=true;
      updateMenu=true;
    }
};

////////////////////////////////////////
//  DECLARE MENU
////////////////////////////////////////

lcdMenu mainMenu(displayMenuItem);


///////////////////////////////////////////
// CHANGE INTERRUPTS
///////////////////////////////////////////

 void IRAM_ATTR updateEncoder(){
 
  static uint8_t currentStateCLK;
  static uint8_t lastStateCLK;
	// Read the current state of CLK
	currentStateCLK = digitalRead(CLK);

	if (currentStateCLK != lastStateCLK  && currentStateCLK == 1){
		if (digitalRead(DT) != currentStateCLK) {
    mainMenu.lcdUpSelected=true;
		} else {
    mainMenu.lcdDownSelected=true;
		}
	}
	lastStateCLK = currentStateCLK;
};

void IRAM_ATTR switchPress(){

    mainMenu.lcdSelect();
}

void setup() {
frequency=0;
 track=0;

  Serial.begin(115200);

///////////////////////////////////
//  ADD MENUITEMS TO MENU
////////////////////////////////////
  mainMenu.lcdAddMenu("frequency",freqUp,freqDown,&frequency);
  mainMenu.lcdAddMenu("track",trackUp,trackDown,&track);

//////////////////////////////////////
//  ROTARY ENCODER
/////////////////////////////////////////

  pinMode(CLK,INPUT);
  pinMode(DT,INPUT);
  pinMode(SW, INPUT_PULLUP);
  attachInterrupt(CLK, updateEncoder, CHANGE);
  attachInterrupt(DT, updateEncoder, CHANGE);
  attachInterrupt(SW, switchPress,RISING);

//  DISPLAY

  display.init();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  mainMenu.lcdDisplay();

}

void loop() {
  
  if(mainMenu.updateMenu)mainMenu.lcdDisplay();
  if(mainMenu.lcdUpSelected)mainMenu.lcdUp();
  else if(mainMenu.lcdDownSelected)mainMenu.lcdDown();
  delay(10);
}
