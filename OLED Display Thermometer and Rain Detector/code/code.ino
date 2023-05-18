#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <Wire.h>
#include "dht_nonblocking.h"
#include <Fonts/FreeSans12pt7b.h>
#define DHT_SENSOR_TYPE DHT_TYPE_11
#define DHT_SENSOR_PIN 10
#define WLS_POWER 12
#define RED 7
#define GREEN 6
#define BLUE 5
#define ADC 0
#define BUTTON 2
#define THRESHOLD 1      // Max accuracy for device is 1.8 degrees F
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int redValue;
int greenValue;
int blueValue;
enum state{Temperature, Humidity};
enum state mode = Temperature;
char printBuffer[128];
DHT_nonblocking dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

void setup() {
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  pinMode(WLS_POWER, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP); 
  digitalWrite(RED, LOW);
  digitalWrite(GREEN, LOW);
  digitalWrite(BLUE, LOW);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  Serial.begin(9600);
}

void loop() {
  float temperature;
  float tempF;
  float temp_diff;
  float humidity;

  /* Measure temperature and humidity.  If the functions returns
    true, then a measurement is available. */
  if (measure_environment(&temperature, &humidity) == true) {
    tempF = temperature * 9/5 + 32;
    temp_diff = delta_temp(tempF);

    /* Light up LED according to if a front is occurring
    or if there are extreme temperature values*/
    if (fabs(temp_diff) >= THRESHOLD) {                                         // front is occurring
      if (temp_diff <= -THRESHOLD) {blueValue = 255; redValue = 0; blink();}    // coldfront
      else {blueValue = 0; redValue = 255; blink();}                            // warmfront
    } else {                                                                    // no front, check for extreme values
      if (temperature <= 0) {blueValue = 255; redValue = 0;}                    // freezing
      else if (tempF >= 100) {blueValue = 0; redValue = 255;}                   // really hot
      else {blueValue = 0; redValue = 0;}                                       // turn off
    }

    if (rainfall()) greenValue = 255; else greenValue = 0;       // green LED for rain
  }

  // Switch display modes
  if (digitalRead(BUTTON) == LOW) {
    switch(mode) {
      case Humidity: mode = Temperature; break;
      default: mode = Humidity;
    }
    delay(300);
  }

  // Update LED colors
  analogWrite(RED, redValue);
  analogWrite(GREEN, greenValue);
  analogWrite(BLUE, blueValue);

  // Update display
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setFont(&FreeSans12pt7b);
  display.setTextSize(2);

  switch (mode) {
    case Humidity:
      display.setCursor(10, 50);
      display.print(humidity, 0);
      display.print(" %");
      break; 
    default: 
      display.print(tempF, 1); 
      if (tempF >= 100) { 
        display.setCursor(0, 50);  
        display.drawCircle(120, 10, 5, WHITE);   
      } else {
        display.setCursor(15, 50);
        display.drawCircle(110, 10, 5, WHITE); 
      }
  }

  display.display();
  delay(100);
}

static bool measure_environment(float *temperature, float *humidity) {
  static unsigned long measurement_timestamp = millis();

  /* Measure once every four seconds. */
  if (millis() - measurement_timestamp > 3000ul) {
    if (dht_sensor.measure(temperature, humidity) == true) {
      measurement_timestamp = millis();
      return(true);
    }
  }

  return(false);
}

static bool rainfall() {
  static unsigned long measurement_timestamp = millis();
  static float prev_value = 1;
  static bool raining = false;
  Serial.println(prev_value);

  /* Measure once every 60 seconds. */
  if (millis() - measurement_timestamp > 59000ul) {
    int value = analogRead(ADC);
    if (value - prev_value >= 0) raining = true; else raining = false;
    digitalWrite(WLS_POWER, LOW);
    measurement_timestamp = millis();
    prev_value = value;
  } else if (millis() - measurement_timestamp > 49000ul) digitalWrite(WLS_POWER, HIGH); // Turn sensor on 10 seconds early

  return raining;
}

static float delta_temp(float temp) {
  static unsigned long measurement_timestamp = millis();
  static float prev_temp = temp;

  /* Measure once every 60 seconds. */
  if (millis() - measurement_timestamp > 59000ul) {
    prev_temp = temp;
    measurement_timestamp = millis();
  }

  return temp - prev_temp;
}

void blink() {
  analogWrite(RED, redValue);      // turn the LED on
  analogWrite(GREEN, greenValue);
  analogWrite(BLUE, blueValue);     
  delay(500);                      // wait for a second
  analogWrite(RED, 0);             // turn the LED off by making the voltage LOW
  analogWrite(GREEN, 0);
  analogWrite(BLUE, 0);      
  delay(500);                      // wait for a second
}