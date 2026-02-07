////////////////////////////////////////////////////////////
//   MENU BEGINS
//  this is for each menu item
//////////////////////////////////////////////////////////////
//this uses tft_espi

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

    bool updateMenu=true;
    lcdMenuItem *menu[16];

    lcdMenu(){
      len=0;
      select=0;
     }

    void displayMenuItem(uint16_t x,uint16_t y, String s){
      img.setCursor(x,y);
      img.print(s);
    //display.display();
    // Serial.println(c);
    }


    void lcdDisplay(){
      int x=8;
      int y=0;
      img.fillScreen(TFT_BLACK); // Clear the screen with black color
      for(uint8_t i=select;i<len;i++){
        if(y>128)break;
        if(i==select)displayMenuItem(0,y,">");
        displayMenuItem(x,y,menu[i]->label);

///display text to 4 decimal places
/*
        if(menu[i]->actFunc==NULL && menu[i]->active){
          String t=String(*menu[i]->value);
          displayMenuItem(80,y,t);
        }
*/
        y+=16;

      }
      img.pushSprite(0,0);
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
    void lcdUp(){

       if(menu[select]->active==true){
        if(menu[select]->upFunc!=NULL)menu[select]->upFunc(menu[select]->value);
       }
       else {
        if(select<len-1)select++;
       }

      updateMenu=true;
    }
    void lcdDown(){
       if(menu[select]->active==true){
        if(menu[select]->downFunc!=NULL)menu[select]->downFunc(menu[select]->value);
       }
       else{
        if(select>0) select--;
       }
      updateMenu=true;
    }
    void lcdSelect(){
      if(menu[select]->active){
        menu[select]->active=false;
//        lcdSave();
      }
      else menu[select]->active=true;

      if(menu[select]->actFunc!=NULL){
        menu[select]->actFunc((char*)menu[select]->label);
      }
      updateMenu=true;
    }
};

