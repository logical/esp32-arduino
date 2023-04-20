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

(function() {

  function draw() {
    canvas = document.getElementById('canvas');

    var img = new Image();
    img.onload = function() {
        var context = canvas.getContext('2d');
        context.drawImage(img, 0, 0);
    }
    img.src = "graph.svg?random="+new Date().getTime();;

  }

function startup(){
  setInterval(draw,1000);
  
}
  window.addEventListener('load', startup, false);

})();
</script>
</head>
<body>
<canvas width="900" height="500" id="canvas" />  
<form>
</form>
</body>
</html>
)***";

#define IMAGE_WIDTH 800
const uint16_t dataSize= IMAGE_WIDTH;
uint16_t data[dataSize];
uint16_t dataIndex;

static esp_err_t index_handler(httpd_req_t *req)
{
    Serial.println("loading page");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "html");
    return httpd_resp_send(req,HTML.c_str(), HTML.length());
 
}
#define SENSORPIN A0
static esp_err_t draw_graph(httpd_req_t *req) {
  String out = "";
  char temp[512];
 // while(1){
    out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"800\" height=\"400\">\n";
    out += "<rect width=\"800\" height=\"400\" fill=\"rgb(255, 255, 255)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
    out += "<g stroke=\"black\">\n";
    int y = 0;
    int i =dataIndex;
    for (int x = 0; x < dataSize; x++) {
      uint16_t y2=data[i]/4;
      sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 390 - y, x + 1, 390 - y2);
      out += temp;
      y = y2;
//      y=rand()%400;
      i++ ;
      if(i>=dataSize)i=0;   
    }
    out += "</g>\n</svg>\n";

    httpd_resp_set_type(req, "image/svg+xml");
    httpd_resp_set_hdr(req, "Content-Encoding", "svg");
    return httpd_resp_send(req,out.c_str(), out.length());
//  }
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

  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_11db);
  adc1_config_width(ADC_WIDTH_12Bit);

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

  httpd_uri_t graph_uri = {
      .uri = "/graph.svg",
      .method = HTTP_GET,
      .handler = draw_graph,
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
      httpd_register_uri_handler(stream_httpd, &graph_uri);
  }
}
//WiFiClient client;

void loop() {
  uint16_t d[4];
  d[0]=analogRead(SENSORPIN);
  delay(4);
  d[1]=analogRead(SENSORPIN);
  delay(4);
  d[2]=analogRead(SENSORPIN);
  delay(4);
  d[3]=analogRead(SENSORPIN);
  data[dataIndex]=(d[0]+d[1]+d[2]+d[3])/4;
  dataIndex++;
  if(dataIndex >=dataSize)dataIndex=0;
  delay(20);  
}
