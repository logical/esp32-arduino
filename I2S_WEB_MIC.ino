/*
join the access point then go to 192.168,4,1
*/
#include <WiFi.h>
#include <WiFiAP.h>

#include <esp_http_server.h>
#include <driver/i2s.h>

 String HTML = R"***(
<html>
<head>
<script>
</script>
</head>
<body>
<form>
<input type="text" id= "address">
<input type="submit" value="connect" onclick="connect()" >
</form>
<audio controls autoplay>
  <source src="http://192.168.4.1/audio.wav" type="audio/wav">
</audio>
</body>
</html>
)***";

uint8_t wavheader[44]={
0x52,0x49,0x46,0x46,
0xFF,0xFF,0xFF,0xFF,
0x57,0x41,0x56,0x45,
0x66,0x6D,0x74,0x20,
0x10,0x00,0x00,0x00,
0x01,0x00,
0x01,0x00,
0x80,0x3E,0x00,0x00,//16k
0x00,0x7D,0x00,0x00,//byte rate
0x02,0x00,//block
0x10,0x00,
0x64,0x61,0x74,0x61,//"data"
0xFF,0xFF,0xFF,0xFF};

#define SAMPLE_BUFFER_SIZE 1024
#define SAMPLE_RATE 8000
// either wire your microphone to the same pins or change these to match your wiring
#define I2S_SERIAL_CLOCK GPIO_NUM_32
#define I2S_LEFT_RIGHT_CLOCK GPIO_NUM_33
#define I2S_SERIAL_DATA GPIO_NUM_35

i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0};

i2s_pin_config_t i2s_mic_pins = {
    .mck_io_num = I2S_PIN_NO_CHANGE,
    .bck_io_num = I2S_SERIAL_CLOCK,
    .ws_io_num = I2S_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SERIAL_DATA};

static esp_err_t index_handler(httpd_req_t *req)
{
    Serial.println("loading page");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "html");
    return httpd_resp_send(req,HTML.c_str(), HTML.length());
 
}
int32_t raw_samples[SAMPLE_BUFFER_SIZE];

static esp_err_t stream_handler(httpd_req_t *req)
{
    Serial.println("streaming");

    httpd_resp_set_type(req, "audio/wav");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=audio.wav");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    
    httpd_resp_send_chunk(req, (const char *)wavheader, sizeof(wavheader));
   
 
    size_t bytes_read;

    while(1){
      esp_err_t result=i2s_read(I2S_NUM_0, raw_samples, sizeof(int32_t) * SAMPLE_BUFFER_SIZE, &bytes_read, portMAX_DELAY);
      if (result == ESP_OK){

        for (int i = 0; i < bytes_read/sizeof(int32_t); i++)raw_samples[i]>>=12;

        Serial.print(".");
        result=httpd_resp_send_chunk(req, (const char *)raw_samples, bytes_read);
        if(result!=ESP_OK)return result;
      }
    }      

}

const int led = 13;

void setup() {
  pinMode(led, OUTPUT);
  digitalWrite(led,LOW);
  Serial.begin(115200);

  Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();
  if(ESP_OK!=i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL)){
    Serial.println("setup fail");
    
  }
  if(ESP_OK!=i2s_set_pin(I2S_NUM_0, &i2s_mic_pins)){
    Serial.println("setup fail");

  } 
  WiFi.softAP("test");
  IPAddress myIP = WiFi.softAPIP();

  httpd_uri_t index_uri = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = index_handler,
      .user_ctx = NULL,
      .is_websocket = true,
      .handle_ws_control_frames = false,
      .supported_subprotocol = NULL
  };

  httpd_uri_t stream_uri = {
      .uri = "/audio.wav",
      .method = HTTP_GET,
      .handler = stream_handler,
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
      httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}

void loop() {
}
