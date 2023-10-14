#include <WiFi.h>
#include <WiFiAP.h>

#include <esp_http_server.h>
#include <EEPROM.h>
#include "FS.h"
#include "SD.h"

#define EEPROM_SIZE 128

String ssid;
String pass;

const int led = 13;
String getExtension(const char * n){
  String filename=n;
  int index = filename.indexOf(".");
  String extension = filename.substring(index+1);
  return extension;
}

//this index handler can set the header to almost any filetype
//I could not get mp3 to work
static esp_err_t index_handler(httpd_req_t *req)
{
  Serial.print("loading page ");
  Serial.println(req->uri);
  String extension = getExtension(req->uri);
  if (extension=="txt" || extension== "html"){
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "html");
  }
  if (extension=="png"){
    httpd_resp_set_type(req, "image/png");
    httpd_resp_set_hdr(req, "Content-Encoding", "png");
  }
  if (extension== "jpg"){
    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Encoding", "jpg");
  }
  
  File fs = SD.open(req->uri);
  String html=fs.readString();

  return httpd_resp_send(req,html.c_str(),html.length());
 
}

void getCreds(){

    Serial.println("");
  //get the ssid and password from Serial
      char c;
      Serial.println("enter ssid");
      while(1){
        while(!Serial.available()){delay(1);}
        c=Serial.read();
        if(c=='\n')break;
        ssid =String(ssid +c);
        delay(10);
      } 
      c='\0';   
      Serial.println("enter password");
      while(1){
        while(!Serial.available()){delay(1);}
        c=Serial.read();
        if(c=='\n')break;
        pass =String(pass +c);
        delay(10);
      }    

      EEPROM.writeString(0, ssid);
      EEPROM.writeString(32, pass);
      EEPROM.commit();

}

//index all files in the flat directory, no subdirectories

void listDir(fs::FS &fs, const char * dirname){

  httpd_handle_t stream_httpd = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 64;

  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if(!root){
      Serial.println("Failed to open directory");
      return;
  }
  if(!root.isDirectory()){
      Serial.println("Not a directory");
      return;
  }

  File file = root.openNextFile();
  if (httpd_start(&stream_httpd, &config) == ESP_OK)

    while(file){
      if(file.isDirectory()){
      } 
      else {
        Serial.printf("  FILE: %s , SIZE %d \n",file.path(),file.size());

//check file extension
        String extension = getExtension(file.path()); 
        Serial.printf("extension :%s\n",extension.c_str());
        if(extension == "html" || extension == "txt" || extension =="png" || extension =="jpg" ){
          
          httpd_uri_t indexAllUri={
            //this should have a const char * filename 
            .uri = strcpy((char*)malloc(strlen(file.path())+1), file.path()),

            .method = HTTP_GET,
            .handler = index_handler,
            .user_ctx = NULL,
          };
          httpd_register_uri_handler(stream_httpd, &indexAllUri);
        }
      }
      file = root.openNextFile();
    }
}



void setup(void) {
   
 
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  
   EEPROM.begin(EEPROM_SIZE);
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);

  if(!SD.begin()){
      Serial.println("Card Mount Failed");
      return;
  }

  ssid = EEPROM.readString(0);
  pass = EEPROM.readString(32);

//get the ssid and password if they are not saved.

  if(!ssid.length()){
    getCreds();
  }

//use the ssid
  WiFi.begin(ssid.c_str(),pass.c_str());
  for(int i=0;i<8;i++){
      if(WiFi.status() == WL_CONNECTED) break;
      delay(500);
      Serial.print(".");
  }
//still not connecting

  if(WiFi.status() != WL_CONNECTED) {
    ssid="";
    pass="";
    getCreds();
    WiFi.begin(ssid.c_str(),pass.c_str());
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
  }
  
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  listDir(SD, "/");

}

void loop(void) {
 delay(100);
}
