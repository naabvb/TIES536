/*********
  TIES536 Harjoitus 4
  Lauri Pimiä
  Sensorit päivittyvät tasan 28,8 minuutin välein, paitsi PIR,
  joka päivittyy tarpeen mukaan tai kun 60 sekuntia edellisestä liikkeestä on kulunut.
  Kaikkien arvosana 5 ominaisuuksien pitäisi toimia niin kuin pitää.
*********/

#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SEALEVELPRESSURE_HPA (1013.25)
#define LIGHTSENSORPIN 34 // Valosensori pinnissä 34
#define pirCooldownSeconds 60 // Seconds of no movement for PIR to reset
#define sensorsCooldownSeconds 1728 // Other sensors will update every 1728 seconds or 28.8 minutes because:
// 50 times a day = 24h / 50 = 0.48h * 60min = 28.8min * 60s = 1728s

// Timer variables
unsigned long now = millis(); // Time since startup
unsigned long pirLastTrigger = 0; // Time since last PIR trigger
unsigned long lastSensorUpdate = 0; // Time since last sensor update
boolean startPirTimer = false; // Is PIR timer running?
boolean pirAlarmSent = false; // Is PIR alarm already sent?
boolean startSensorTimer = false; // Is sensor timer running?
boolean systemStartup = true; // Used to run first update before timer

// PIR-pins
const int sensorPin = 26;
byte redPin = 16;
byte bluePin = 5;

Adafruit_BME280 bme; // I2C

WiFiMulti WiFiMulti;
HTTPClient ask;
const char* ssid     = "WLAN_NAME"; //Wifi SSID
const char* password = "WLAN_PASSWORD"; //Wifi Password
const char* apiKey1 = "API_KEY_ONE"; // API KEY for temperature, humidity and pressure
const char* apiKey2 = "API_KEY_TWO"; // API KEY for lux and PIR

// ASKSENSORS API host config
const char* host = "api.asksensors.com";  // API host name
const int httpPort = 80;      // port

void setupPir() {
  pinMode(sensorPin, INPUT_PULLUP);
  pinMode(redPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  digitalWrite(bluePin, 0);
}

// Gets run by interrupt when PIR is triggered.
// IRAM_ATTR runs the function on RAM, avoiding random core panic (https://randomnerdtutorials.com/esp32-pir-motion-sensor-interrupts-timers/)
void IRAM_ATTR movementDetected() {
  startPirTimer = true;
  pirLastTrigger = millis();
}

void setup() {

  Serial.begin(115200);
  pinMode(LIGHTSENSORPIN, INPUT); // Setup lux-sensor
  setupPir(); // Setup PIR-sensor pins
  bme.begin(0x76); // Select default I2C address
  attachInterrupt(digitalPinToInterrupt(sensorPin), movementDetected, RISING); // Add interrupt to PIR sensor

  Serial.println("*****************************************************");
  Serial.println("********** Program Start : Connect ESP32 to AskSensors.");
  Serial.println("Wait for WiFi... ");

  // connecting to the WiFi network
  WiFiMulti.addAP(ssid, password);
  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  // connected
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void loop() {

  now = millis();

  // Run startup loop to update sensors and start their timer
  if (systemStartup) {
    updateSensors();
    systemStartup = false;
    startSensorTimer = true;
    lastSensorUpdate = millis();
  }

  // Check if sensors should be updated and update if necessary
  if (startSensorTimer && (now > lastSensorUpdate) && ((now - lastSensorUpdate) > (sensorsCooldownSeconds * 1000))) {
    updateSensors();
    lastSensorUpdate = millis();
  }

  // Sends the actual motion alert if interrupt is triggered
  if (startPirTimer && pirAlarmSent == false) {
    Serial.println("Motion start");
    updatePIR(1);
    pirAlarmSent = true;
  }

  // Check if PIR cooldown has elapsed without additional movement and update if necessary
  if (startPirTimer && (now > pirLastTrigger) && ((now - pirLastTrigger) > (pirCooldownSeconds * 1000))) {
    Serial.println("Motion stop");
    startPirTimer = false;
    updatePIR(0);
    pirAlarmSent = false;
  }
}

// Updates PIR status
void updatePIR(int state) {

  WiFiClient client;

  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  } else {

    String url = "http://api.asksensors.com/write/";
    url += apiKey2;
    url += "?module3=";
    url += state;
    url += "&module4=";
    url += state;
    Serial.print("********** requesting URL: ");
    Serial.println(url);

    // send data
    ask.begin(url); //Specify the URL
    int httpCode = ask.GET();

    if (httpCode > 0) {
      String payload = ask.getString();
      Serial.println(httpCode);
      Serial.println(payload);
    } else {
      Serial.println("Error on HTTP request");
    }

    ask.end(); //End
    Serial.println("********** End ");
    Serial.println("*****************************************************");
    client.stop();  // stop client
  }
}

// Updates BME280 and Lux-sensor data
void updateSensors() {

  WiFiClient client;

  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  } else {
    updateBME(client);
    updateLux(client);
    client.stop();
  }
}

// Uploads results from lux-sensor to cloud
void updateLux(WiFiClient client) {

  String url = "http://api.asksensors.com/write/";
  url += apiKey2;
  url += "?module1=";
  url += analogRead(LIGHTSENSORPIN);
  url += "&module2=";
  url += analogRead(LIGHTSENSORPIN);
  Serial.print("********** requesting URL: ");
  Serial.println(url);
  // send data
  ask.begin(url); //Specify the URL

  //Check for the returning code
  int httpCode = ask.GET();

  if (httpCode > 0) {

    String payload = ask.getString();
    Serial.println(httpCode);
    Serial.println(payload);
  } else {
    Serial.println("Error on HTTP request");
  }

  ask.end(); //End
  Serial.println("********** End ");
  Serial.println("*****************************************************");
  return;
}

// Uploads results from BME280 to cloud
void updateBME(WiFiClient client) {

  String url = "http://api.asksensors.com/write/";
  url += apiKey1;
  url += "?module1=";
  url += bme.readTemperature();
  url += "&module2=";
  url += bme.readTemperature();
  url += "&module3=";
  url += bme.readHumidity();
  url += "&module4=";
  url += bme.readPressure() / 100.0F;

  Serial.print("********** requesting URL: ");
  Serial.println(url);
  // send data
  ask.begin(url); //Specify the URL

  //Check for the returning code
  int httpCode = ask.GET();

  if (httpCode > 0) {

    String payload = ask.getString();
    Serial.println(httpCode);
    Serial.println(payload);
  } else {
    Serial.println("Error on HTTP request");
  }

  ask.end(); //End
  Serial.println("********** End ");
  Serial.println("*****************************************************");
  return;
}
