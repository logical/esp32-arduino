#include "esp_ota_ops.h"
#include "nvs_flash.h"
#include "FS.h"
#include <SD.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <EEPROM.h>

#define BUTTON_PIN A5

#define TFT_MOSI 13 // Data out
#define TFT_SCLK 14  // Clock out
#define SD_MISO 12
#define SD_CS 2

int sck = TFT_SCLK;
int miso = SD_MISO;
int mosi = TFT_MOSI;
int cs = SD_CS;

TFT_eSPI tft = TFT_eSPI(128,160);  // Invoke library, pins defined in User_Setup.h
TFT_eSprite img = TFT_eSprite(&tft);  // Create Sprite object "img" with pointer to "tft" object

#include "menu.h"

#define EEPROM_SIZE 16

lcdMenu mainMenu;


/*uint32_t get_app_partition_address() {
    const esp_partition_t* app_partition=esp_ota_get_boot_partition();
 
     if (app_partition != NULL) {
        Serial.printf("App partition label: %s\n", app_partition->label);
        Serial.printf("App partition start address: 0x%08x\n", app_partition->address);
        Serial.printf("App partition size: 0x%08x (%d bytes)\n", app_partition->size, app_partition->size);
    } else {
        Serial.printf("Application partition not found!\n");
    }
  return app_partition->address;
}

void get_partition_info(void) {
    const esp_partition_t *data_partition = NULL;

    // Find the first partition that matches the type and subtype
    data_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA,      // Type
        ESP_PARTITION_SUBTYPE_DATA_NVS, // Subtype (e.g., NVS, SPIFFS, FAT)
        NULL                          // Label (NULL for any label)
    );

    if (data_partition != NULL) {
        Serial.printf("Found partition: ");
        Serial.printf("  Label: %s", data_partition->label);
        Serial.printf("  Type: %d, Subtype: %d", data_partition->type, data_partition->subtype);
        Serial.printf("  Start Address: 0x%08x", data_partition->address); // The start address
        Serial.printf("  Size: 0x%08x (%d KB)\n", data_partition->size, data_partition->size / 1024);
    } else {
        Serial.printf("Data partition not found!");
    }
}
void writeFile(fs::FS &fs, const char *path, uint8_t* data,int len) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.write(data, len)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

///GET SETTINGS SAVED IN MEMORY
  nvs_handle_t my_handle;
  ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &my_handle)); 
  ESP_ERROR_CHECK(nvs_set_u8(my_handle, name, &data[i]));
  nvs_close(my_handle);
*/

/////////////////////////////////////////////
// for my button switches i used one analog pin with 10k pullup.
// the pulldown resistors on the switches where:
// 1 = 100
// 2 = 330
// 3 = 1k
// 4 = 1.8k
// 5 = 3k
// the numbers dont work like I wanted somehow the 100 is too low.
//4095*(100/10100) is 40 , not zero.
// I had room for more buttons.
///////////////////////////////////////////////
int8_t checkButtons(){
  int sensorValue = 0;
  for(int i=0;i<32;i++){
    sensorValue += analogRead(BUTTON_PIN);
  }
  sensorValue /=32;
//  Serial.print("button value :");
//  Serial.println(sensorValue);

  if(sensorValue ==0)return 1;//up
  if(sensorValue >0 && sensorValue < 6)return 2;//middle
  if(sensorValue >240 && sensorValue < 250)return 3;//down
  if(sensorValue >495 && sensorValue < 500)return 4;//right
  if(sensorValue >820 && sensorValue < 825)return 5;//left
  return -1;
}


 

//this is the main function. IT takes the bin file from the sd card and loads it into memory.
// Then it set the pointer to the begining and reboots.
//you have to set the boot partition back to the bootloader in your program code or it will be stuck.
/*
  if (ESP_OK != esp_ota_set_boot_partition(esp_ota_get_next_update_partition(NULL))) {
    Serial.println("Bootloader error");
  } 
*/
//then when you restart it returns to the bootloader.
/*
    esp_restart();
*/ 
#define BUFFER_SIZE 4096
  static uint8_t buf[BUFFER_SIZE];

void runProgram(char* name){
  uint32_t len=0;

  img.fillRect(0,0,TFT_WIDTH,TFT_HEIGHT,TFT_BLACK);
  img.setCursor(32,32);
  img.print("LOADING...");
  img.pushSprite(0,0);  

  Serial.print("running ");
  Serial.println(name);
  File file = SD.open(name);
  
  esp_ota_handle_t update_handle;
  const esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);
  ESP_ERROR_CHECK(esp_ota_begin(partition,OTA_SIZE_UNKNOWN,&update_handle));
//  ESP_ERROR_CHECK(esp_ota_set_final_partition(update_handle,partition,true));
  len=file.size();
    while (len) {
      Serial.print("writing...");
      Serial.println(len);
      size_t toRead = len;
      if (toRead > BUFFER_SIZE) {
        toRead = BUFFER_SIZE;
      }
      file.read(buf, toRead);
      ESP_ERROR_CHECK(esp_ota_write(update_handle,buf,toRead));
      len -= toRead;
    }
//            delay(500);
  ESP_ERROR_CHECK(esp_ota_end(update_handle));
  if (ESP_OK == esp_ota_set_boot_partition(esp_ota_get_next_update_partition(NULL))) {
    esp_restart();
  } else {
    Serial.println("Upload Error");
  }
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      mainMenu.lcdAddMenu(file.path(), runProgram,NULL,NULL,NULL);
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
  file.close();
}


void setup() {
  Serial.begin(115200);


///test flash memory write
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
      ESP_LOGE("INIT", "ERASING NVS");
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK( ret );

  nvs_handle_t my_handle;
  ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &my_handle)); 
  ESP_ERROR_CHECK(nvs_set_str(my_handle, "test", "test"));
  nvs_close(my_handle);


  SPI.begin(sck, miso, mosi, cs);
  if (!SD.begin(cs) ){
//  if (!SD.begin()) {
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

 // Setup the LCD
  tft.init();
  img.createSprite(160, 128);

  tft.initDMA(); // To use SPI DMA you must call initDMA() to setup the DMA engine

  tft.setSwapBytes(true);
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  tft.setTextSize(1);
//  tft.setTextWrap(true);     // Turn on the magic


  listDir(SD,"/",0);

 // mainMenu.lcdDisplay();

}

void loop() {


  uint8_t button=checkButtons(); 
  if(mainMenu.updateMenu){
    img.fillSprite(TFT_BLACK);
    mainMenu.lcdDisplay();
    img.pushSprite(0,0);
  }
  if(button==1)mainMenu.lcdUp();
  if(button==3)mainMenu.lcdDown();
  if(button==2)mainMenu.lcdSelect();
  delay(200);
}
