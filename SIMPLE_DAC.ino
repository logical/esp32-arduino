#include "soc/dac_channel.h"
#include "driver/dac_common.h"
#include "trust_wav.h"

#define SAMPLE_RATE 8000  //nice to know. 8 bits unsigned data at 8000hz
#define DAC1 25

void setup()
{
  // we need serial output for the plotter
  Serial.begin(115200);
  pinMode(DAC1,OUTPUT);}
    size_t bytes = 0;

void loop() {
  dac_output_enable(DAC_CHANNEL_1);
  for(int16_t i=0;i< TRUST_WAV_LEN;i++){
    dac_output_voltage(DAC_CHANNEL_1, trust_wav[i]);
    delayMicroseconds(125);

  }
  
  dac_output_disable(DAC_CHANNEL_1);
  delay(1000);

}
