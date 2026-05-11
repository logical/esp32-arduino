#include "esp_ota_ops.h"
#include "nvs_flash.h"

#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include "SD.h"

#include "esp_camera.h"
#include <vector>

#include "quirc.h"
#include "qrcode.h"

static const char *TAG = "camera_httpd";




#ifdef CONFIG_LED_ILLUMINATOR_ENABLED
int led_duty = 0;
bool isStreaming = false;
#ifdef CONFIG_LED_LEDC_LOW_SPEED_MODE
#define CONFIG_LED_LEDC_SPEED_MODE LEDC_LOW_SPEED_MODE
#else
#define CONFIG_LED_LEDC_SPEED_MODE LEDC_HIGH_SPEED_MODE
#endif
#endif

#define BUTTON_PIN A5

#define TFT_CS        15
#define TFT_RST        -1 // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC         32
#define TFT_MOSI 13 // Data out
#define TFT_SCLK 14  // Clock out

#define SD_MISO 12
#define SD_CS 2

TFT_eSPI tft = TFT_eSPI(128,160);  // Invoke library, pins defined in User_Setup.h
TFT_eSprite img = TFT_eSprite(&tft);  // Create Sprite object "img" with pointer to "tft" object


#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     21
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       19
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       5
#define Y2_GPIO_NUM        4
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define EEPROM_SIZE 128
#define debug false

  camera_config_t cameraConfig;
  TaskHandle_t scanTaskHandle = NULL;

//  uint16_t *image = NULL;

const char* codeFile="/codeFile.txt";
std::vector<String> codeArray;

void saveCodes(fs::FS &fs) {
  //remove duplicates
  std::sort(codeArray.begin(),codeArray.end());
  auto last =std::unique(codeArray.begin(),codeArray.end());
  codeArray.erase(last,codeArray.end());

  File file = fs.open(codeFile, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
  }  
  for(int i=0;i<codeArray.size();i++){
    file.println(codeArray[i].c_str());
  }
  file.close();
}


void readCodes(fs::FS &fs) {

  File file = fs.open(codeFile);
  if (!file){
    Serial.println("Failed to open file for reading");
    return;
  }
  codeArray.clear();

  while(file.available()){
    String code=file.readStringUntil('\n');
    if(code.length()>0){
      code.trim();
      codeArray.push_back(code);
    }
  }
  file.close();

}

void setup() {
//set the target boot partition to the bootloader first in case the power is shut off
  if (ESP_OK != esp_ota_set_boot_partition(esp_ota_get_next_update_partition(NULL))) {
    Serial.println("Bootloader error");
  } 

  Serial.begin(115200);

  ///test flash memory write
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
      ESP_LOGE("INIT", "ERASING NVS");
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK( ret );
  size_t len;
  char data[5];//exactly the right size
  nvs_handle_t my_handle;
  ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &my_handle)); 
  ESP_ERROR_CHECK(nvs_get_str(my_handle, "test", data,&len));
  nvs_close(my_handle);
  Serial.printf("read nvs: %s\n",data);

//setup sd card first
  SPI.begin(TFT_SCLK, SD_MISO, TFT_MOSI, SD_CS);
  if (!SD.begin(SD_CS) ){
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

 // randomSeed(analogRead(A0));
  // Setup the LCD
  tft.init();
  img.createSprite(160, 128);

  tft.initDMA(); // To use SPI DMA you must call initDMA() to setup the DMA engine

  tft.setSwapBytes(true);
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(5);
  tft.setTextSize(1);        // Make it readable
  tft.setTextWrap(true);     // Turn on the magic

  img.createSprite(160, 128);



  Serial.println("serial ready");
  
  cameraConfig.ledc_channel = LEDC_CHANNEL_0;
  cameraConfig.ledc_timer = LEDC_TIMER_0;
  cameraConfig.pin_d0 = Y2_GPIO_NUM;
  cameraConfig.pin_d1 = Y3_GPIO_NUM;
  cameraConfig.pin_d2 = Y4_GPIO_NUM;
  cameraConfig.pin_d3 = Y5_GPIO_NUM;
  cameraConfig.pin_d4 = Y6_GPIO_NUM;
  cameraConfig.pin_d5 = Y7_GPIO_NUM;
  cameraConfig.pin_d6 = Y8_GPIO_NUM;
  cameraConfig.pin_d7 = Y9_GPIO_NUM;
  cameraConfig.pin_xclk = XCLK_GPIO_NUM;
  cameraConfig.pin_pclk = PCLK_GPIO_NUM;
  cameraConfig.pin_vsync = VSYNC_GPIO_NUM;
  cameraConfig.pin_href = HREF_GPIO_NUM;
  cameraConfig.pin_sscb_sda = SIOD_GPIO_NUM;
  cameraConfig.pin_sscb_scl = SIOC_GPIO_NUM;
  cameraConfig.pin_pwdn = PWDN_GPIO_NUM;
  cameraConfig.pin_reset = RESET_GPIO_NUM;
  cameraConfig.xclk_freq_hz = 40000000;

//  cameraConfig.frame_size = FRAMESIZE_QQVGA; //QQVGA=160x120 
  cameraConfig.frame_size = FRAMESIZE_VGA;
//  cameraConfig.frame_size = FRAMESIZE_QCIF; //QCIF=176x144
  
//  cameraConfig.pixel_format = PIXFORMAT_JPEG; // for streaming
  cameraConfig.pixel_format = PIXFORMAT_GRAYSCALE; // for qr scannerface detection/recognition
//  cameraConfig.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
//  cameraConfig.jpeg_quality=4;
  cameraConfig.grab_mode = CAMERA_GRAB_LATEST;
  cameraConfig.fb_location = CAMERA_FB_IN_PSRAM;
  cameraConfig.fb_count = 1;
  
  // camera init
  esp_err_t err = esp_camera_init(&cameraConfig);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  // Example: Setting up the sensor in the code
  sensor_t * s = esp_camera_sensor_get();
//  s->set_quality(s, 10);              // only jpeg
  s->set_denoise(s,8);
  s->set_brightness(s, 2);            // Slightly brighter
  s->set_contrast(s,2);
  s->set_hmirror(s, 1);               // Mirror image
  s->set_vflip(s, 1);                 // Flip image
  s->set_saturation(s,2);
//  s->set_whitebal(s, 1);  // Auto White Balance On
//  s->set_awb_gain(s, 1);  // AWB Gain On
//  s->set_wb_mode(s, 0);   // Mode: Auto

//buffer for image
//  image = (uint16_t*)malloc(160*128*2); //st7735 160x128
img.setTextColor(TFT_GREEN, TFT_BLACK); // Set text color and background (to overwrite old text)

readCodes(SD);

xTaskCreatePinnedToCore(qrCodeDetectTask, "qrCodeDetectTask", 65535, NULL, 1, &scanTaskHandle, 1);


}

void returnToBootloader(){
 // Read the uint32_t value from the specified address
//  EEPROM.get(0, bootAddress);
    esp_restart();


}

void adjustImage(camera_fb_t *fb){
 /* 
  uint8_t bot=255,mid=0,top=0;
  float avg;
  for (int i=0;i<fb->len;i++){
    if(bot>fb->buf[i])bot=fb->buf[i];
    if(top<fb->buf[i])top=fb->buf[i];
  }
//  for(int i=0;i<fb->len;i++)avg+=fb->buf[i];
//  mid=avg/fb->len;
  mid=top/2;
  for (int i=0;i<fb->len;i++){
    if(fb->buf[i]<mid)fb->buf[i]-=bot;
    else if(fb->buf[i]>mid)fb->buf[i]+=(255-top);    
  }
*/
}

void findRectangle(  camera_fb_t *fb){

  int i=0;
  for(int y=0;y<fb->height;y++){
    for (int x=0;x<fb->height;x++){
      int index=(y*fb->width+x);
      fb->buf[i++]=fb->buf[index];
    }
  }
  fb->width=fb->height;
  fb->len=i;
}



int displayCode=0;
int buttonStatus=-1;
int mode=0;
char payload[1024];

const char* help="\n\nUP   for scanner\n\nMIDDLE to exit\n\nDOWN for printer";
const char* scanhelp="\n\nRIGHT to save\n\nLEFT to exit";

void loop() {

    buttonStatus = checkButtons(); 
    if(mode==0){
      //show start screen
      img.fillSprite(TFT_BLACK);
      img.setCursor(16,16);
      img.print(help);
      img.pushSprite(0,0);      
      payload[0] = '\0';//erase last code
      if(buttonStatus==1)mode=1;
      else if(buttonStatus==2)returnToBootloader();
      else if(buttonStatus==3)mode=2;
    }
    else if (mode==1){
      if(buttonStatus==1){}
      else if(buttonStatus==2)mode=0;
      else if(buttonStatus==3){}
      else if(buttonStatus==4){
        if(payload[0]!='\0'){
          codeArray.push_back(payload);
          saveCodes(SD);
          payload[0]='\0';
        }
      }//right
      else if(buttonStatus==5)payload[0]='\0';
    }    ///mode 1 handled in qrCodeDetectTask
    else if(mode==2){
      if(buttonStatus==1);
      else if(buttonStatus==2)mode=0;
      else if(buttonStatus==3){
        codeArray.erase(codeArray.begin()+displayCode);
        saveCodes(SD);
        displayCode=0;        
      }//delete code
      else if(buttonStatus==4){if(displayCode<codeArray.size()-1)displayCode++;}
      else if(buttonStatus==5){if(displayCode>0)displayCode--;}
      drawQRCode(codeArray[displayCode].c_str());
    }

    vTaskDelay(200 / portTICK_PERIOD_MS);

}

void drawQRCode(const char* qrc){
    int blk=2;
    uint8_t border=4*blk;

    QRCode qrcode;
//    Serial.println(qrc);

    uint8_t qrcodeData[qrcode_getBufferSize(3)];
    qrcode_initText(&qrcode, qrcodeData, 3, 0, qrc);
    // Top quiet zone

    img.fillSprite(TFT_WHITE);
    for (uint8_t y = 0; y < qrcode.size; y++) {    
        for (uint8_t x = 0; x < qrcode.size; x++) {

            if(qrcode_getModule(&qrcode, x, y))img.fillRect(x*blk+border, y*blk+border, blk, blk, TFT_BLACK);
        }

    }
    img.setCursor(0, 100);
    img.setTextColor(TFT_GREEN, TFT_BLACK); // Set text color and background (to overwrite old text)
    img.print(qrc);

    img.pushSprite(0,0);
}

int8_t checkButtons(){
  int sensorValue = 0;
  for(int i=0;i<32;i++){
    sensorValue += analogReadMilliVolts(BUTTON_PIN);
  }
  sensorValue /=32;
//  Serial.print("button value :");
//  Serial.println(sensorValue);

  if(sensorValue <125)return 1;//up
  if(sensorValue >125 && sensorValue < 140)return 2;//middle
  if(sensorValue >300 && sensorValue < 350)return 3;//down
  if(sensorValue >500 && sensorValue < 600)return 4;//right
  if(sensorValue >700 && sensorValue < 825)return 5;//left
  return -1;
}

void dumpData(const struct quirc_data *data)
{
  Serial.printf("Version: %d\n", data->version);
  Serial.printf("ECC level: %c\n", "MLHQ"[data->ecc_level]);
  Serial.printf("Mask: %d\n", data->mask);
  Serial.printf("Length: %d\n", data->payload_len);
  Serial.printf("Payload: %s\n", data->payload);
}

void qrCodeDetectTask(void *taskData)
{
  struct quirc *q = NULL;
  uint8_t *image = NULL;
  camera_fb_t *fb = NULL;

  uint16_t old_width = 0;
  uint16_t old_height = 0;

  if (debug)
  {
    Serial.printf("begin to qr_recoginze\r\n");
  }
  q = quirc_new();
  if (q == NULL)
  {
    if (debug)
    {
      Serial.print("can't create quirc object\r\n");
    }
    vTaskDelete(NULL);
    return;
  }

  while (true)
  {
    if (debug)
    {
      Serial.printf("alloc qr heap: %u\r\n", xPortGetFreeHeapSize());
      Serial.printf("uxHighWaterMark = %d\r\n", uxTaskGetStackHighWaterMark(NULL));
    }
    if(mode==1){

    fb = esp_camera_fb_get();
    if (!fb)
    {
      if (debug)
      {
        Serial.println("Camera capture failed");
      }

      continue;
    }
//    adjustImage(fb);
    showGrayscale(fb,payload);  //draw the image to lcd
    findRectangle(fb); //this will clip a square from the image

      if (quirc_resize(q, fb->height, fb->height) < 0)
      {
        if (debug)
        {
          Serial.println("Resize the QR-code recognizer err (cannot allocate memory).");
        }
        esp_camera_fb_return(fb);
        fb = NULL;
        image = NULL;
        continue;
      }

    int w,h;
    image = quirc_begin(q, NULL,NULL);
    if (debug)
    {
      Serial.printf("Frame w h len: %d, %d, %d \r\n", fb->width, fb->height, fb->len);
    }

    memcpy(image, fb->buf, fb->len);
    quirc_end(q);

    if (debug)
    {
      Serial.printf("quirc_end\r\n");
    }
    int count = quirc_count(q);
    if (count == 0)
    {
      if (debug)
      {
        Serial.printf("Error: not a valid qrcode\n");
      }
      esp_camera_fb_return(fb);
      fb = NULL;
      image = NULL;
      continue;
    }

    for (int i = 0; i < count; i++)
    {
      struct quirc_code code;
      struct quirc_data data;
      quirc_decode_error_t err;

      quirc_extract(q, i, &code);

      err = quirc_decode(&code, &data);


      if (err)
      {
        const char *error = quirc_strerror(err);
        int len = strlen(error);
        if (debug)
        {
          Serial.printf("Decoding FAILED: %s\n", error);
        }
        esp_camera_fb_return(fb);
        fb = NULL;
        image = NULL;
//        payload[0] = '\0';
        //break;
      }
      else
      {
        if (debug)
        {
          Serial.printf("Decoding successful:\n");
          dumpData(&data);
        }
        snprintf(payload,1024,"%s",(char*)data.payload);
        //break;

      }
    }
    esp_camera_fb_return(fb);
    fb = NULL;
    image = NULL;
  }else vTaskDelay(200 / portTICK_PERIOD_MS);

  }
  quirc_destroy(q);
  vTaskDelete(NULL);
}


uint16_t convertGrayscaleToRGB565(uint8_t gray) {
    uint16_t r = (gray >> 3) & 0x1F;
    uint16_t g = (gray >> 2) & 0x3F;
    uint16_t b = (gray >> 3) & 0x1F;
    return (r << 11) | (g << 5) | b;
}

#define VGAW 640
#define VGAH 480
#define SCALEX VGAW/TFT_WIDTH
#define SCALEY VGAH/TFT_HEIGHT

void showGrayscale(camera_fb_t * fb, char* text) {
    if (!fb){
      Serial.println("crash");
      return;
    }

    int srcW = fb->width;  
    int srcH = fb->height;
    int destH=tft.getViewportHeight();
    int destW=tft.getViewportWidth();
    uint8_t* src = (uint8_t*)fb->buf;



    for (int y = 0; y < destH; y++) {
      for (int x = 0; x < destW; x++) {
        int srcX = (int)(x * SCALEX); 
        int srcY = (int)(y * SCALEY);
        uint8_t g = src[srcY * srcW + srcX];
    // Convert 8-bit gray to 16-bit RGB565
        uint16_t color = img.color565(g, g, g); 
//        image[i++] = color; 
          img.drawPixel(x, y, color);
      }
    }
img.drawRect(1,1,TFT_HEIGHT-1, TFT_HEIGHT-1, TFT_GREEN);

if(text[0]!='\0'){
  img.setCursor(0, 0);       // Start at top-left
  img.setTextColor(TFT_GREEN, TFT_BLACK); // Set text color and background (to overwrite old text)
  img.print(text);
  img.print(scanhelp);
}
/*
  tft.dmaWait();
  tft.startWrite();
tft.pushImageDMA(0,0,destW,destH,image);

tft.endWrite();
*/

//img.pushImage(0,0,destW,destH,(uint16_t*)fb->buf,1);
img.pushSprite(0,0);

}

 