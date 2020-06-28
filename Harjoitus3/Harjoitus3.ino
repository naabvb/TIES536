/*********
  TIES536 Harjoitus 3
  Lauri Pimiä
  Kaikkien arvosanaan 5 kuuluvien ominaisuuksien pitäisi toimia.
*********/

// Load Wi-Fi library
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SEALEVELPRESSURE_HPA (1013.25)
#define LIGHTSENSORPIN 34 //Valosensori pinnissä 34

// PIR-pins
byte sensorPin = 26;
byte redPin = 16;
byte bluePin = 5;

Adafruit_BME280 bme; // I2C

// ESP-tukiaseman ssid ja salasana
const char* ssid     = "ESPAP";
const char* password = "espsalasana321";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Sets PIR-sensor related pins
void setupPir() {
  pinMode(sensorPin, INPUT);
  pinMode(redPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  digitalWrite(bluePin, 0);
}

void setup() {
  Serial.begin(115200);
  pinMode(LIGHTSENSORPIN, INPUT); // Setup lux-sensor
  setupPir(); // Setup PIR-sensor pins
  bme.begin(0x76); // Select default I2C address

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.softAP(ssid, password);

  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.softAPIP());
  server.begin();
}

void loop() {
  byte state = digitalRead(sensorPin); // Read PIR state
  digitalWrite(redPin, state); // LED on if 1
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta charset='utf-8'/><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"); // UTF-8 for äö
            client.println("<link rel=\"icon\" href=\"data:,\">");

            // Write heading and table + center both
            client.println("</head><body><h1 style='text-align:center'>ESP32 Mittaukset</h1>");
            client.println("<table style='margin-left:auto;margin-right:auto;padding-top:0.5em;'>");

            // Write temperature
            client.println("<tr><td>Lämpötila</td><td>");
            client.println(bme.readTemperature(), 1); // 1 = How many decimals to print
            client.println(" °C</td></tr>");

            // Write humidity
            client.println("<tr><td>Kosteus</td><td>");
            client.println(bme.readHumidity(), 0);
            client.println(" %</td></tr>");

            // Write pressure
            client.println("<tr><td>Paine</td><td>");
            client.println(bme.readPressure() / 100.0F, 0);
            client.println(" mbar</td></tr>");

            // Write lux
            client.println("<tr><td>Valoisuus</td><td>");
            client.println(analogRead(LIGHTSENSORPIN));
            client.println(" lux</td></tr>");

            // Write PIR depending on current status
            client.println("<tr><td>Paikalla?</td><td>");
            if (state == 1) client.println("KYLLÄ");
            else if (state == 0) client.println("EI");
            client.println("</td></tr></table>");

            // Add button and containers for centering
            client.println("<div style='position:relative;height:200px;'><div class='centered'><button onClick='window.location.reload();'>UPDATE</button></div></div>");
            client.println("</body></html>");

            // Add CSS
            client.println("<style type='text/css'>");
            client.println(".centered { margin: 0; position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); }");
            client.println("button { background-color: transparent; border-radius: 10px; font-size: larger; padding: 14px 15px; font-weight: bold; border-color: black; }");
            client.println("td { padding: 0 20px; text-align: left; font-weight: bold; }");
            client.println("</style>");

            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
