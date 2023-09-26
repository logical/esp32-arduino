#include <WiFi.h>
#include <WiFiAP.h>

#include <esp_http_server.h>
#include <EEPROM.h>
#define EEPROM_SIZE 128

String ssid;
String pass;


String HTML = R"***(
<html>
<head>
<script>
</script>
</head>
<body>
hello world
</body>
</html>
)***";
const int led = 13;

static esp_err_t index_handler(httpd_req_t *req)
{
    Serial.println("loading page");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "html");
    return httpd_resp_send(req,HTML.c_str(), HTML.length());
 
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
//save the login to eeprom
      EEPROM.writeString(0, ssid);
      EEPROM.writeString(32, pass);
      EEPROM.commit();

}

void setup(void) {
   
 
   EEPROM.begin(EEPROM_SIZE);
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  

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

  httpd_uri_t index_uri = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = index_handler,
      .user_ctx = NULL,
      .is_websocket = true,
      .handle_ws_control_frames = false,
      .supported_subprotocol = NULL
  };
  httpd_handle_t stream_httpd = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  if (httpd_start(&stream_httpd, &config) == ESP_OK)
  {
      httpd_register_uri_handler(stream_httpd, &index_uri);
  }
  Serial.println("HTTP server started");

}

void loop(void) {
 delay(10);
}
