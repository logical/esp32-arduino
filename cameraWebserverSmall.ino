
// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <esp_http_server.h>
#include <esp_camera.h>
#include <fb_gfx.h>
#include <EEPROM.h>
#include <WiFi.h>

#include "esp_log.h"
static const char *TAG = "camera_httpd";



#define QUANT_TYPE 0 //if set to 1 => very large firmware, very slow, reboots when streaming...

#ifdef CONFIG_LED_ILLUMINATOR_ENABLED
int led_duty = 0;
bool isStreaming = false;
#ifdef CONFIG_LED_LEDC_LOW_SPEED_MODE
#define CONFIG_LED_LEDC_SPEED_MODE LEDC_LOW_SPEED_MODE
#else
#define CONFIG_LED_LEDC_SPEED_MODE LEDC_HIGH_SPEED_MODE
#endif
#endif




#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define EEPROM_SIZE 128
// ===========================
// Enter your WiFi credentials
// ===========================
//const char ssid[64] = "**********";
//const char password[64] = "**********";
String ssid;
String pass;


 String HTML = R"***(
<!doctype html>
<html>
    <head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width,initial-scale=1">
        <title>ESP32 OV2460</title>
    </head>
    <body>
<img id="motion" src="document.location.origin +:81" crossorigin>
<img id="stream" src="" crossorigin>
<button id="toggle-stream">Start Stream</button>
        <script>
document.addEventListener('DOMContentLoaded', function (event) {
  var baseHost = document.location.origin;
  var streamUrl = baseHost + ':81';
  const view = document.getElementById('stream');
  const mview = document.getElementById('motion');
  mview.src = `${streamUrl}/stream`;

  const streamButton = document.getElementById('toggle-stream')
const stopStream = () => {
    window.stop();
    streamButton.innerHTML = 'Start Stream'
  }

  const startStream = () => {
    view.src = `${streamUrl}/stream`;
//    show(viewContainer)
    streamButton.innerHTML = 'Start Motion';
  }
    streamButton.onclick = () => {
    const streamEnabled = streamButton.innerHTML === 'Start Motion'
    if (streamEnabled) {
      stopStream();
    } else {
      startStream();
    }
  }

});
        </script>
    </body>
</html>
)***";

  camera_config_t cameraConfig;


void setup() {
  Serial.begin(115200);
  Serial.setTimeout(20000);
  Serial.setDebugOutput(true);
  Serial.println();
    EEPROM.begin(EEPROM_SIZE);

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
  cameraConfig.xclk_freq_hz = 20000000;
  cameraConfig.frame_size = FRAMESIZE_UXGA;
  //cameraConfig.pixel_format = PIXFORMAT_JPEG; // for streaming
  cameraConfig.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  cameraConfig.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  cameraConfig.fb_location = CAMERA_FB_IN_PSRAM;
  cameraConfig.jpeg_quality = 12;
  cameraConfig.fb_count = 1;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(cameraConfig.pixel_format == PIXFORMAT_JPEG){
    if(psramFound()){
      cameraConfig.jpeg_quality = 10;
      cameraConfig.fb_count = 2;
      cameraConfig.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      cameraConfig.frame_size = FRAMESIZE_SVGA;
      cameraConfig.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    cameraConfig.frame_size = FRAMESIZE_240X240;
  }

  // camera init
  esp_err_t err = esp_camera_init(&cameraConfig);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if(cameraConfig.pixel_format == PIXFORMAT_JPEG){
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

WiFi.softAP("test");
IPAddress myIP = WiFi.softAPIP();
  startCameraServer();

  Serial.println(WiFi.macAddress());
  Serial.print("Camera Ready! Use 'http://");
  Serial.print(myIP);
  Serial.println("' to connect");
}

void swapFormats(pixformat_t pixfmt){
  sensor_t * s = esp_camera_sensor_get();
  if(pixfmt == PIXFORMAT_JPEG){
    s->set_pixformat(s,PIXFORMAT_JPEG);
    s->set_quality(s,10);
    s->set_framesize(s, FRAMESIZE_QVGA);
  } else {
    // Best option for face detection/recognition
    s->set_pixformat(s,PIXFORMAT_RGB565);
    s->set_framesize(s,FRAMESIZE_240X240);    
  }

}
void loop() {
  // Do nothing. Everything is done in another task by the web server
  delay(10000);
}
typedef struct
{
    httpd_req_t *req;
    size_t len;
} jpg_chunking_t;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\nX-Timestamp: %d.%06d\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;



#ifdef CONFIG_LED_ILLUMINATOR_ENABLED
void enable_led(bool en)
{ // Turn LED On or Off
    int duty = en ? led_duty : 0;
    if (en && isStreaming && (led_duty > CONFIG_LED_MAX_INTENSITY))
    {
        duty = CONFIG_LED_MAX_INTENSITY;
    }
    ledc_set_duty(CONFIG_LED_LEDC_SPEED_MODE, CONFIG_LED_LEDC_CHANNEL, duty);
    ledc_update_duty(CONFIG_LED_LEDC_SPEED_MODE, CONFIG_LED_LEDC_CHANNEL);
    ESP_LOGI(TAG, "Set LED intensity to %d", duty);
}
#endif



void draw_box(fb_data_t *fbd,int x,int y,int w,int h,uint32_t color){

  if(fbd->bytes_per_pixel == 2){
      //color = ((color >> 8) & 0xF800) | ((color >> 3) & 0x07E0) | (color & 0x001F);
      color = ((color >> 16) & 0x001F) | ((color >> 3) & 0x07E0) | ((color << 8) & 0xF800);
  }
  fb_gfx_drawFastHLine(fbd, x, y, w, color);
  fb_gfx_drawFastHLine(fbd, x, y + h - 1, w, color);
  fb_gfx_drawFastVLine(fbd, x, y, h, color);
  fb_gfx_drawFastVLine(fbd, x + w - 1, y, h, color);

}

/*BROKEN*/
#define THRESH 64
bool motion_detect(camera_fb_t *fb ){
    static uint8_t lastrgb565[24][24];
    bool motion=false;

    fb_data_t rfb;
    rfb.width = fb->width;
    rfb.height = fb->height;
    rfb.data = fb->buf;
    rfb.bytes_per_pixel = 2;
    rfb.format = FB_RGB565;

    int rectx0=fb->width,recty0 =fb->height,rectx1=0,recty1=0;

  for(int y=0,yy=0;y < fb->height-10;yy++,y+=10){
    for(int x=0,xx=0;x < fb->width-10;xx++,x+=10){
      uint32_t i= (y * (fb->width * 2)) + (x * 2);
//      Serial.println(i);
//      uint8_t r1=fb->buf[x*y*2] & 0b11111000;//check red only
      uint8_t r1=fb->buf[i] & 0b11111000;//check red only
      uint8_t r2=lastrgb565[xx][yy] & 0b11111000;
      uint8_t diff=abs(r1-r2);
     
      lastrgb565[xx][yy]=fb->buf[i];//set last pixel

      if(diff>THRESH){
        motion=true;
//        return motion;
        if(rectx0>x)rectx0=x;
        if(rectx1<x)rectx1=x;
        if(recty0>y)recty0=y;
        if(recty1<y)recty1=y;
      }

    }
  }

  if(motion==true){
//     Serial.printf("Motion detected %d %d %d %d",rectx0,recty0,rectx1,recty1);
    draw_box(&rfb,rectx0,recty0,rectx1-rectx0,recty1-recty0,0x00ff00);              
//    enable_led(1);

  }      
  return motion;
}
bool MotionDetector=false;

static esp_err_t motion_handler(httpd_req_t *req){
 // MotionDetector=TRUE;
  stream_handler(req);
}



static esp_err_t stream_handler(httpd_req_t *req){
    camera_fb_t *fb = NULL;
    struct timeval _timestamp;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;
    char *part_buf[128];

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK)
    {
        return res;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "X-Framerate", "60");


    while (true)
    {


        fb = esp_camera_fb_get();
        if (!fb)
        {
            log_e("Camera capture failed");
            res = ESP_FAIL;
        }
        else
        {
            if (fb->format != PIXFORMAT_JPEG)
            {
                if(motion_detect(fb));
                bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                esp_camera_fb_return(fb);
                fb = NULL;
                if (!jpeg_converted)
                {
                    log_e("JPEG compression failed");
                    res = ESP_FAIL;
                }
            }
            else
            {
                _jpg_buf_len = fb->len;
                _jpg_buf = fb->buf;
            }
        }
        if (res == ESP_OK)
        {
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if (res == ESP_OK)
        {
            size_t hlen = snprintf((char *)part_buf, 128, _STREAM_PART, _jpg_buf_len, _timestamp.tv_sec, _timestamp.tv_usec);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if (res == ESP_OK)
        {
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if (fb)
        {
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        }
        else if (_jpg_buf)
        {
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        if (res != ESP_OK)
        {
            log_e( "send frame failed failed");
            break;
        }
        int64_t fr_end = esp_timer_get_time();


//        int64_t frame_time = fr_end - last_frame;
//        frame_time /= 1000;
    }
    return res;
}




static esp_err_t index_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "html");
    return httpd_resp_send(req, HTML.c_str(), HTML.length());
}

void startCameraServer(void)
{
    httpd_config_t httpConfig = HTTPD_DEFAULT_CONFIG();
    httpConfig.max_uri_handlers = 16;

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
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = NULL,
        .is_websocket = true,
        .handle_ws_control_frames = false,
        .supported_subprotocol = NULL
    };
    httpd_uri_t motion_uri = {
        .uri = "/motion",
        .method = HTTP_GET,
        .handler = motion_handler,
        .user_ctx = NULL,
        .is_websocket = true,
        .handle_ws_control_frames = false,
        .supported_subprotocol = NULL
    };
//    ra_filter_init(&ra_filter, 20);
    ESP_LOGI(TAG, "Starting web server on port: '%d'", httpConfig.server_port);
    if (httpd_start(&camera_httpd, &httpConfig) == ESP_OK)
    {
        httpd_register_uri_handler(camera_httpd, &index_uri);
    }

    httpConfig.server_port += 1;
    httpConfig.ctrl_port += 1;
    ESP_LOGI(TAG, "Starting stream server on port: '%d'", httpConfig.server_port);
    if (httpd_start(&stream_httpd, &httpConfig) == ESP_OK)
    {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
        httpd_register_uri_handler(stream_httpd, &motion_uri);
    }
}
