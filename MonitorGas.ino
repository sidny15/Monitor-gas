#define BLYNK_PRINT Serial

#define BLYNK_AUTH_TOKEN "289PB4GsVg786NIpbIW97SH7PWPmXODz"
#define BLYNK_TEMPLATE_ID "TMPL6FFDBavRj"
#define BLYNK_TEMPLATE_NAME "Monitor Gas"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// Define your Blynk authentication token
// Your WiFi credentials
char ssid[] = "GELOMBANG";
char pass[] = "";

// Analog pin definitions
#define TGS2600_PIN 34   // GPIO34 (Analog ADC1_CH6)
#define MQ136_PIN 35     // GPIO35 (Analog ADC1_CH7)
#define buz 4
// Calibration values - adjust based on your sensors
#define TGS2600_RLOAD 10.0  // Load resistance in kOhms
#define MQ136_RLOAD 10.0     // Load resistance in kOhms

// Variables for sensor readings
float tgs2600_ratio = 0;
float mq136_ratio = 0;
float tgs2600_ppm = 0;
float mq136_ppm = 0;

BlynkTimer timer;

void readSensors() {
  // Read TGS2600 sensor
  int tgs2600_raw = analogRead(TGS2600_PIN);
  float tgs2600_voltage = tgs2600_raw * (3.3 / 4095.0); // ESP32 ADC resolution is 12-bit (0-4095)
  float tgs2600_rs = ((5.0 - tgs2600_voltage) / tgs2600_voltage) * TGS2600_RLOAD;
  tgs2600_ratio = tgs2600_rs / TGS2600_RLOAD;
  
  // Read MQ136 sensor
  int mq136_raw = analogRead(MQ136_PIN);
  float mq136_voltage = mq136_raw * (3.3 / 4095.0);
  float mq136_rs = ((5.0 - mq136_voltage) / mq136_voltage) * MQ136_RLOAD;
  mq136_ratio = mq136_rs / MQ136_RLOAD;
  tgs2600_ppm = pow(10, (log10(tgs2600_ratio) - 0.8) / -0.5); // Example for air quality
  mq136_ppm = pow(10, (log10(mq136_ratio) - 0.6) / -0.8);     // Example for H2S
  
  // Send data to Blynk
  Blynk.virtualWrite(V0, tgs2600_ppm);  // Virtual pin V0 for TGS2600
  Blynk.virtualWrite(V1, mq136_ppm);    // Virtual pin V1 for MQ136
  Blynk.virtualWrite(V2, tgs2600_voltage); // Raw voltage for debugging
  Blynk.virtualWrite(V3, mq136_voltage);   // Raw voltage for debugging
  if(tgs2600_ppm > 100 || mq136_ppm > 100){
    digitalWrite(buz,HIGH);
    delay(300);
    digitalWrite(buz, LOW);
    delay(300);
  }else{
    digitalWrite(buz,LOW);
  }
  // Print to serial for debugging
  Serial.print("TGS2600 - PPM: ");
  Serial.print(tgs2600_ppm);
  Serial.print(" | Voltage: ");
  Serial.print(tgs2600_voltage);
  Serial.print(" | Ratio: ");
  Serial.println(tgs2600_ratio);
  
  Serial.print("MQ136 - PPM: ");
  Serial.print(mq136_ppm);
  Serial.print(" | Voltage: ");
  Serial.print(mq136_voltage);
  Serial.print(" | Ratio: ");
  Serial.println(mq136_ratio);
}

void setup() {
  Serial.begin(115200);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  timer.setInterval(2000L, readSensors);
  analogSetAttenuation(ADC_11db); 
  pinMode(buz, OUTPUT);
  Serial.println("System started");
}

void loop() {
  Blynk.run();
  timer.run();
}