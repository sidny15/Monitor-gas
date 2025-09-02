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

// Variables for buzzer control
unsigned long previousBuzzerMillis = 0;
int buzzerState = LOW;
int beepCount = 0;
int maxBeeps = 0;
int beepInterval = 0;

BlynkTimer timer;

void controlBuzzer() {
  // Determine which sensor has higher ppm
  float max_ppm = max(tgs2600_ppm, mq136_ppm);
  
  // Reset buzzer logic if ppm is below 20
  if (max_ppm <= 20) {
    digitalWrite(buz, LOW);
    beepCount = 0;
    return;
  }
  
  // Determine buzzer pattern based on ppm level
  if (max_ppm > 20 && max_ppm <= 50) {
    // 1-3 beeps with 300ms interval
    maxBeeps = map(max_ppm, 21, 50, 1, 3);
    beepInterval = 300;
  } 
  else if (max_ppm > 50 && max_ppm <= 100) {
    // Long beep with 1 second interval
    maxBeeps = 1;
    beepInterval = 1000;
  }
  else if (max_ppm > 100 && max_ppm <= 400) {
    // Continuous beep
    maxBeeps = 1;
    beepInterval = 300;
  }
  else if (max_ppm > 400) {
    // For ppm above 400, turn off buzzer (or implement different behavior)
    digitalWrite(buz, LOW);
    return;
  }
  
  // Handle buzzer patterns
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousBuzzerMillis >= beepInterval) {
    previousBuzzerMillis = currentMillis;
    
    if (buzzerState == LOW && beepCount < maxBeeps) {
      buzzerState = HIGH;
      digitalWrite(buz, buzzerState);
      beepCount++;
    } 
    else {
      buzzerState = LOW;
      digitalWrite(buz, buzzerState);
      
      // If we've completed all beeps, reset for next cycle
      if (beepCount >= maxBeeps) {
        beepCount = 0;
      }
    }
  }
}

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
  tgs2600_ppm = map(tgs2600_ppm, 0, 1000, 0,3000);
  // Send data to Blynk
  Blynk.virtualWrite(V0, tgs2600_ppm);  // Virtual pin V0 for TGS2600
  Blynk.virtualWrite(V1, mq136_ppm);    // Virtual pin V1 for MQ136
  Blynk.virtualWrite(V2, tgs2600_voltage); // Raw voltage for debugging
  Blynk.virtualWrite(V3, mq136_voltage);   // Raw voltage for debugging
  
  // Control buzzer based on sensor readings
  controlBuzzer();
  
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
  
  // Print buzzer status
  float max_ppm = max(tgs2600_ppm, mq136_ppm);
  Serial.print("Max PPM: ");
  Serial.print(max_ppm);
  Serial.print(" | Buzzer: ");
  Serial.println(digitalRead(buz) ? "ON" : "OFF");
}

void setup() {
  Serial.begin(115200);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  timer.setInterval(2000L, readSensors);
  analogSetAttenuation(ADC_11db); 
  pinMode(buz, OUTPUT);
  digitalWrite(buz, LOW);
  Serial.println("System started");
}

void loop() {
  Blynk.run();
  timer.run();
}
