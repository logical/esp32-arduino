////////////////////////////////////////////////////////////
//   MENU BEGINS
//  this is for each menu item
//////////////////////////////////////////////////////////////
// this uses tft_espi
// define your display first then include this header
// if actFunc is defined then a value is not defined
// if a value is defined then upFunc and downFunc must be defined to change value
//////////////////////////////////////////////////////////////////////////////////
#define tft_espi
//#define ssd1306
struct lcdMenuItem{
    bool active=false;
    char label[32];
    float *value;
    void (*actFunc)(char*)=NULL;
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
    int start=0;

    bool updateMenu=true;
    lcdMenuItem *menu[16];

    lcdMenu(){
      len=0;
      select=0;
     }
#ifdef tft_espi
    void displayMenuItem(uint16_t x,uint16_t y, String s,bool sel){
      img.setTextSize(1);
      img.setTextColor(TFT_GREEN, TFT_BLACK); // Set text color and background (to overwrite old text)

      if(sel)img.setTextColor(TFT_BLACK,TFT_GREEN); // Set text color and background (to overwrite old text)

      img.setCursor(x,y);
      img.print(s);
    }
    void displayItemValue(float v){
      String t=String(v);
      tft.setTextSize(3);
      img.setTextColor(TFT_GREEN, TFT_BLACK); // Set text color and background (to overwrite old text)
      img.setCursor(10,10);
      img.print(t);
    }
    #define LINE_SIZE 16
    #define PAGE_SIZE (TFT_HEIGHT/LINE_SIZE)
#elif defined ssd1306


#endif

    void lcdDisplay(){
      int x=8;
      int y=0;
      if(menu[select]->actFunc==NULL && menu[select]->active){
        displayItemValue(*menu[select]->value);
      }
      else{
        for(uint8_t i=start;i<len;i++){
            if(y>TFT_HEIGHT)break;
            if(i==select){
              displayMenuItem(0,y,menu[i]->label,true);
            }
            else displayMenuItem(0,y,menu[i]->label,false);
            y+=LINE_SIZE;
        }
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
    void lcdAddMenu(const char *t,void (*act)(char* name),void (*up)(float*),void (*down)(float*),float *v){
      menu[len]=new lcdMenuItem;
      snprintf(menu[len]->label,32,"%s",t);
      menu[len]->actFunc=act;
      menu[len]->upFunc=up;
      menu[len]->downFunc=down;
      menu[len]->value=v;
      len++;
      Serial.print("menuitems ");
      Serial.println(len);
    }
    void lcdDown(){

       if(menu[select]->active==true){
        if(menu[select]->downFunc!=NULL)menu[select]->downFunc(menu[select]->value);
       }
       else {
        if(select<len-1)select++;
        if(select>=start+PAGE_SIZE)start++;

       }

      updateMenu=true;
    }
    void lcdUp(){
       if(menu[select]->active==true){
        if(menu[select]->upFunc!=NULL)menu[select]->upFunc(menu[select]->value);
       }
       else{
        if(select>0) select--;
        if(select<start)start--;
       }
      updateMenu=true;
    }
    void lcdSelect(){
      if(menu[select]->active)menu[select]->active=false;
      else menu[select]->active=true;

      if(menu[select]->actFunc!=NULL){
        menu[select]->actFunc((char*)menu[select]->label);
        return;
      }
      updateMenu=true;
    }
};

