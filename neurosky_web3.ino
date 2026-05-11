/*
join the access point then go to 192.168,4,1
*/
#include <Brain.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include <ArduinoJson.h>
#include <esp_http_server.h>
//#include <vector>
//#include "FS.h"
//#include "SPIFFS.h"

//#define FORMAT_SPIFFS_IF_FAILED true

//#define BUTTON_PIN 0 //boot button can be reused in code, 0 when pressed
//#define LED_PIN 17
#define BUZZ_PIN 27
#define RX_PIN 25
#define TX_PIN 26

JsonDocument doc;

//esp32c3 super mini uart0 is on pins 20 and 21
Brain brain(Serial2);


 String HTML = R"***(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Brainwave Monitor</title>
    <style>
        body { font-family: sans-serif; background: #222; color: #eee; padding: 20px; }
        .container { width: 900px; margin: auto; }
        .controls { 
            display: grid; 
            grid-template-columns: repeat(4, 1fr); 
            gap: 10px; 
            background: #333; 
            padding: 15px; 
            border-radius: 8px; 
            margin-bottom: 10px;
        }
        label { font-size: 0.9em; cursor: pointer; }
        svg { width: 100%; height: 400px; border: 1px solid #444; background: #111; border-radius: 8px; }
    </style>
</head>
<body>

<div class="container">
    <div class="controls" id="checkbox-panel">
        </div>

    <svg id="chart" viewBox="0 0 800 300">
        <line x1="0" y1="150" x2="800" y2="150" stroke="#333" />
        <g id="lines-container"></g> </svg>

    <div style="margin-top: 15px;">
        <input type="range" id="scroll" min="0" max="0" value="0" style="width: 80%;">
        <span id="pos">0</span> / <span id="total">0</span>
    </div>
</div>

<script>
const SAMPLES = 36000;
const WINDOW_SIZE = 400;

// Mapping our sensors to metadata for easy looping
const sensors = [
    { id: 'sensorA', key: 'signal',     color: '#00ffcc', data: [], scale: 1 },
    { id: 'sensorB', key: 'attention',  color: '#ff4444', data: [], scale: 3 },
    { id: 'sensorC', key: 'meditation', color: '#ff44ff', data: [], scale: 3 },
    { id: 'sensorD', key: 'delta',      color: '#ffff00', data: [], scale: 0.0001 },
    { id: 'sensorE', key: 'theta',      color: '#00ccff', data: [], scale: 0.0001 },
    { id: 'sensorF', key: 'lowalpha',   color: '#77ff77', data: [], scale: 0.0001 },
    { id: 'sensorG', key: 'highalpha',  color: '#22aa22', data: [], scale: 0.0001 },
    { id: 'sensorH', key: 'lowbeta',    color: '#ffaa00', data: [], scale: 0.0001 },
    { id: 'sensorI', key: 'highbeta',   color: '#cc6600', data: [], scale: 0.0001 },
    { id: 'sensorJ', key: 'lowgamma',   color: '#bbbbbb', data: [], scale: 0.0001 },
    { id: 'sensorK', key: 'midgamma',   color: '#666666', data: [], scale: 0.0001 }
];

const panel = document.getElementById('checkbox-panel');
const linesContainer = document.getElementById('lines-container');
const scrollBar = document.getElementById('scroll');
const posLabel = document.getElementById('pos');
const totalLabel = document.getElementById('total');

// Setup UI: Create checkboxes and SVG lines dynamically
sensors.forEach(s => {
    // Add Checkbox
    panel.innerHTML += `
        <label><input type="checkbox" id="check_${s.id}" checked onchange="updateGraph()"> ${s.key}</label>
    `;
    // Add SVG Line
    const poly = document.createElementNS("http://www.w3.org/2000/svg", "polyline");
    poly.setAttribute("id", `line_${s.id}`);
    poly.setAttribute("fill", "none");
    poly.setAttribute("stroke", s.color);
    poly.setAttribute("stroke-width", "2");
    linesContainer.appendChild(poly);
});

function updateGraph() {
    const start = parseInt(scrollBar.value);
    posLabel.innerText = start;
    totalLabel.innerText = sensors[0].data.length;

    sensors.forEach(s => {
        const lineElement = document.getElementById(`line_${s.id}`);
        const isChecked = document.getElementById(`check_${s.id}`).checked;

        if (!isChecked || s.data.length === 0) {
            lineElement.setAttribute('points', ""); // Hide line
            return;
        }

        const slice = s.data.slice(start, start + WINDOW_SIZE);
        const points = slice.map((val, i) => {
            const x = i * (800 / WINDOW_SIZE);
            // Apply scale so data stays within 0-300px height
            const y = 300 - (val * s.scale);
            return `${x},${y}`;
        }).join(' ');

        lineElement.setAttribute('points', points);
    });
}

async function startStreaming() {
    try {
        const response = await fetch('/stream');
        if (!response.ok) throw new Error("Network error");
        
        const data = await response.json(); // AWAIT IS KEY!

        sensors.forEach(s => {
            s.data.push(data[s.key] || 0);
            if (s.data.length > SAMPLES) s.data.shift();
        });

        // Update scrollbar bounds
        scrollBar.max = Math.max(0, sensors[0].data.length - WINDOW_SIZE);
        
        // Auto-scroll if already at the end
        if (parseInt(scrollBar.value) >= scrollBar.max - 5) {
            scrollBar.value = scrollBar.max;
        }

        updateGraph();
    } catch (error) {
        console.error("Fetch failed:", error);
    }
}

// Start polling
setInterval(startStreaming, 1000);
scrollBar.addEventListener('input', updateGraph);
</script>
</body>
</html>

)***";

#define CHANNELS 11
/*
std::vector<uint16_t> signalQ;
std::vector<uint16_t> attention;
std::vector<uint16_t> meditation;
std::vector<uint16_t> sensorA;
std::vector<uint16_t> sensorB;
std::vector<uint16_t> sensorC;
std::vector<uint16_t> sensorD;
std::vector<uint16_t> sensorE;
std::vector<uint16_t> sensorF;
std::vector<uint16_t> sensorG;
std::vector<uint16_t> sensorH;
*/

char temp[64];
uint8_t radio=1;
uint8_t sliderValue=1;
String messageBuffer="ready...\n";


static esp_err_t index_handler(httpd_req_t *req)
{
  Serial.println("loading page");
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Content-Encoding", "html");
  return httpd_resp_send(req,HTML.c_str(), HTML.length());
 
}

static esp_err_t message_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Content-Encoding", "text");
  return httpd_resp_send(req,messageBuffer.c_str(), messageBuffer.length());
 
}

static esp_err_t radio_handler(httpd_req_t *req){
  char buf[32];
//  Serial.println("Radio handler");
  httpd_req_get_url_query_str(req,buf,32);
//  Serial.println(buf);
  if(strcmp(buf,"frequencies")==0){radio=3;}
  else if(strcmp(buf,"storage")==0){radio=2;}
  else{radio=1;}
//  clearData();  

  return ESP_OK;

}
static esp_err_t slider_handler(httpd_req_t *req){
  char buf[32];
//  Serial.println("Radio handler");
  httpd_req_get_url_query_str(req,buf,32);
//  Serial.println(buf);
  sliderValue=atoi(buf);
  Serial.print("slider = ");
  Serial.println(sliderValue);
  return ESP_OK;

}

const char *color[11]={"#ff0000","#00ff00","#0000ff","#ffff00","#ff00ff","#00ffff","#7f0000","#007f00","#00007f","#7f7f00","#7f007f"};

esp_err_t stream_handler(httpd_req_t *req) {
    // Set headers for binary stream
//    uint16_t sensorData[3];
 // 2. Serialize JSON to a string
    String response;
    serializeJson(doc, response);
// 3. Set the content type and send the response
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, response.c_str(), HTTPD_RESP_USE_STRLEN);
}

 bool connectionStatus = false;
static void http_server_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data) {
    if (id == HTTP_SERVER_EVENT_ON_CONNECTED) {
        ESP_LOGI("HTTP_SERVER", "A client has connected");
        connectionStatus = true;
    } else if (id == HTTP_SERVER_EVENT_DISCONNECTED) {
        ESP_LOGI("HTTP_SERVER", "A client has disconnected");
        connectionStatus = false;
    }
}


void setBaudRate(){
/*
    0x00: Switch to 9600 Baud, Normal Output
    0x01: Switch to 1200 Baud, Normal Output
    0x02: Switch to 57.6k Baud, Normal + Raw Output
    Note: Upon reset, the module reverts to the hardware pad (BR0/BR1) settings. 
*/
  delay(500);

  Serial2.write(0x01);

  delay(500);
  Serial2.end();

  Serial2.begin(57600,SERIAL_8N1,1,0);//data rate
//  Serial1.updateBaudRate(57600);



}
void setup() {


  Serial.begin(115200);

  Serial.setDebugOutput(true);
  Serial.setDebugOutput(true);

    //workaround
  dacDisable(RX_PIN);
  dacDisable(TX_PIN);

  delay(1000);
  Serial2.begin(9600,SERIAL_8N1,RX_PIN,TX_PIN);//data rate
  //setBaudRate();
  ledcAttach(BUZZ_PIN, 5000, 10); 
//  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_0db);
//  adc1_config_width(ADC_WIDTH_12Bit);
 String newHostname = "ESP32Node";
  WiFi.hostname(newHostname.c_str());

  
  WiFi.mode(WIFI_AP);
  WiFi.softAP("test");
//  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.setTxPower(WIFI_POWER_2dBm);

  //workaround
  dacDisable(RX_PIN);
  dacDisable(TX_PIN);

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
  
  httpd_uri_t message_uri = {
      .uri = "/message",
      .method = HTTP_GET,
      .handler = message_handler,
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

  httpd_uri_t radio_uri = {
      .uri = "/radio",
      .method = HTTP_GET,
      .handler = radio_handler,
      .user_ctx = NULL,
      .is_websocket = true,
      .handle_ws_control_frames = false,
      .supported_subprotocol = NULL
  };
  httpd_uri_t slider_uri = {
      .uri = "/slider",
      .method = HTTP_GET,
      .handler = slider_handler,
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
      httpd_register_uri_handler(stream_httpd, &message_uri);
      httpd_register_uri_handler(stream_httpd, &stream_uri);
      httpd_register_uri_handler(stream_httpd, &radio_uri);
      httpd_register_uri_handler(stream_httpd, &slider_uri);
      // Register in your app_main or server start function
    esp_event_handler_register(ESP_HTTP_SERVER_EVENT, ESP_EVENT_ANY_ID, &http_server_event_handler, NULL);

  }

 delay(1000);
  Serial2.begin(9600,SERIAL_8N1,RX_PIN,TX_PIN);//data rate

  Serial.println("ready");

}
unsigned long microTime,waitTime;
unsigned long avgmed;
unsigned long avgatt;
unsigned int count;

void loop() {
//  Serial.println("inside loop");
  
  if(brain.update()){
    Serial.println("getting reading");

    doc["signal"]=brain.readSignalQuality();
    doc["attention"]=brain.readAttention();
    doc["meditation"]=brain.readMeditation();
    doc["delta"]=brain.readDelta();
    doc["theta"]=brain.readTheta();
    doc["lowalpha"]=brain.readLowAlpha();
    doc["highalpha"]=brain.readHighAlpha();
    doc["lowbeta"]=brain.readLowBeta();
    doc["highbeta"]=brain.readHighBeta();
    doc["lowgamma"]=brain.readLowGamma();
    doc["midgamma"]=brain.readMidGamma(); 

    uint8_t qual=doc["signal"];
    uint8_t att=doc["attention"];
    uint8_t med=doc["meditation"];
    if(qual==0){
      avgmed+=med;
      avgatt+=att;
      count++;
      Serial.println("signal good");
      if((micros()-microTime)>waitTime){
        med=avgmed/count;
        att=avgatt/count;
        ledcWriteTone(BUZZ_PIN, att*10); // Send tone 
        delay(30);
        ledcWriteTone(BUZZ_PIN, 0);     // Turn off
//        delay(1000);
        waitTime=400000*(unsigned long)med;//up to ten seconds
        microTime=micros();
        count=0;
        avgmed=0;
        avgatt=0;
      }
    }
  }
  else{
//    delay(10);
      vTaskDelay(10/portTICK_PERIOD_MS);
  }

}
