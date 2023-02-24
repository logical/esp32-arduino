/*
 To display audio prepare circuit found in following link or drafted as ASCII art
 https://forum.arduino.cc/index.php?topic=567581.0
 (!) Note that unlike in the link we are connecting the supply line to 3.3V (not 5V)
 because ADC can measure only up to around 3V. Anything above 3V will be very inaccurate.

                      ^ +3.3V
                      |
                      _
                     | |10k
                     |_|
                      |            | |10uF
   GPIO34-------------*------------| |----------- line in
(Default ADC pin)     |           +| |      
                      |
                      _
                     | |10k
                     |_|
                      |
                      |
                      V GND

Connect hot wire of your audio to Line in and GNd wire of audio cable to common ground (GND)
*/
#include <driver/i2s.h>

#define SAMPLE_BUFFER_SIZE 1024
#define SAMPLE_RATE 96000

i2s_config_t i2s_config = {
      // I2S with ADC
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
      .intr_alloc_flags = 0,
      .dma_buf_count = 4,
      .dma_buf_len = SAMPLE_BUFFER_SIZE, 
      .use_apll = true,  
};


void setup()
{
  // we need serial output for the plotter
  Serial.begin(115200);
  // start up the I2S peripheral
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_11db);
  adc1_config_width(ADC_WIDTH_12Bit);
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);

  i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_6);//gpio34

  i2s_adc_enable(I2S_NUM_0);  

  Serial.println("setup success");
}

uint16_t raw_samples[SAMPLE_BUFFER_SIZE];
void loop()
{
  
  // read from the I2S device
  uint16_t samples[SAMPLE_BUFFER_SIZE/4];
  size_t bytes_read = 0;
  esp_err_t result=i2s_read(I2S_NUM_0, raw_samples, sizeof(uint16_t) * SAMPLE_BUFFER_SIZE, &bytes_read, portMAX_DELAY);

  if (result == ESP_OK){
   int samples_read = bytes_read / sizeof(uint16_t);
  if(samples_read<SAMPLE_BUFFER_SIZE){
    Serial.println("read error");
    return;
  }
//process samples
  for (int i = 0,j=0; i < samples_read-4; i+=4)
  {//oversample and take the average of 4
   samples[j]= ((raw_samples[i] & 0x0FFF)+(raw_samples[i+1] & 0x0FFF)+
                (raw_samples[i+2] & 0x0FFF)+(raw_samples[i+3] & 0x0FFF))/4;
    Serial.println(samples[j]);
    j++;
    
  }
  }
}
