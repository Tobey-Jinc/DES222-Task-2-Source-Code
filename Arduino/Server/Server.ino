#include <SPI.h>
#include <WiFi101.h>
#include <MPU9250_WE.h>
#include <Adafruit_NeoPixel.h>

#define PI 3.1415926535897932384626433832795

#define MPU9250_ADDR 0x68
MPU9250_WE myMPU9250 = MPU9250_WE(MPU9250_ADDR);

#define RAINBOW_PIN 0
#define NUMPIXELS 13
Adafruit_NeoPixel pixels(NUMPIXELS, RAINBOW_PIN, NEO_GRB + NEO_KHZ800);

#define SECRET_SSID "FriendFinder"
#define SECRET_PASS "ABBADEAF01"
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                // your network key Index number (needed only for WEP)

char packetBuffer[255]; //buffer to hold incoming packet
char WelcomeBuffer[] = "welcome";  // message to send to new connections

int led =  LED_BUILTIN;
int status = WL_IDLE_STATUS;
WiFiServer server(80);

// Store both our location, and the clients location (once we find it out)
float myLatitude = -26.6743;
float myLongitude = 153.0561;
float clientLatitude = -26.6742;
float clientLongitude = 153.0560;

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  Wire.begin();
  pixels.begin();

  pixels.clear();
  pixels.setPixelColor(3, pixels.Color(0, 0, 20));
  pixels.setPixelColor(6, pixels.Color(0, 0, 20));
  pixels.setPixelColor(9, pixels.Color(0, 0, 20));
  pixels.show();

  if(!myMPU9250.init()){
    Serial.println("MPU9250 does not respond");
  }
  else{
    Serial.println("MPU9250 is connected");
  }
  if(!myMPU9250.initMagnetometer()){
    Serial.println("Magnetometer does not respond");
  }
  else{
    Serial.println("Magnetometer is connected");
  }

  myMPU9250.setMagOpMode(AK8963_CONT_MODE_100HZ);

  delay(200);

  // while (!Serial) {
  //   ; // wait for serial port to connect. Needed for native USB port only
  // }

  Serial.println("Access Point Web Server");

  pinMode(led, OUTPUT);      // set the LED pin mode

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }

  // by default the local IP address of will be 192.168.1.1
  // you can override it with the following:
  // WiFi.config(IPAddress(10, 0, 0, 1));

  // print the network name (SSID);
  Serial.print("Creating access point named: ");
  Serial.println(ssid);

  // Create open network. Change this line if you want to create an WEP network:
  status = WiFi.beginAP(ssid, 0, pass);
  if (status != WL_AP_LISTENING) {
    Serial.println("Creating access point failed");
    // don't continue
    while (true);
  }

  // wait 10 seconds for connection:
  delay(10000);

  // start the web server on port 80
  server.begin();

  // you're connected now, so print out the status
  printWiFiStatus();

}

int heading() {
  xyzFloat magValue = myMPU9250.getMagValues(); // returns magnetic flux density [ÂµT] 

  int heading = round(atan2(magValue.y, magValue.x) * 180 / M_PI);
  return heading;
}

float angleBetween(float x1, float x2, float y1, float y2) {
    double radians = atan2((y1 - y2), (x1 - x2));

    double compassReading = radians * (180 / PI);

    return heading() - compassReading;
}

void displayAngle(float angle) {
  int pixel = 0 - int(angle / 15) + 6;
  if (pixel < 0) {
    pixel = 0;
  }
  else if (pixel > 12) {
    pixel = 12;
  }

  int red[12] =   {20, 15, 10, 5, 0, 0, 0, 0, 5, 10, 15, 20};
  int green[12] = {0, 0, 5, 10, 15, 20, 20, 15, 10, 5, 0, 0};

  pixels.clear();
  pixels.setPixelColor(pixel, pixels.Color(red[pixel], green[pixel], 0));
  pixels.show();
}

void loop() {
  // compare the previous status to the current status
  if (status != WiFi.status()) {
    // it has changed update the variable
    status = WiFi.status();

    if (status == WL_AP_CONNECTED) {
      byte remoteMac[6];

      // a device has connected to the AP
      Serial.print("Device connected to AP, MAC address: ");
      WiFi.APClientMacAddress(remoteMac);
      printMacAddress(remoteMac);

    } else {
      // a device has disconnected from the AP, and we are back in listening mode
      Serial.println("Device disconnected from AP");
    }
  }
  
  
  WiFiClient client = server.available();   // listen for incoming clients
  bool ignore = false;

  if (client) {                             // if you get a client,
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:application/json");
            client.println();

            // the content of the HTTP response follows the header:
            // client.print("Click <a href=\"/H\">here</a> turn the LED on<br>");
            // client.print("Click <a href=\"/L\">here</a> turn the LED off<br>");
            char responseBuffer[64];
            sprintf(responseBuffer, "{ \"lat\": %.4f, \"lon\": %.4f }", myLatitude, myLongitude);
            client.print(responseBuffer);

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          }
          else {      // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        }
        else if (c != '\r') {    // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        } else {
          ignore = true;
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H")) {
          digitalWrite(led, HIGH);               // GET /H turns the LED on
        }
        if (currentLine.endsWith("GET /L")) {
          digitalWrite(led, LOW);                // GET /L turns the LED off
        }

        // check if we have been sent a location
        if (currentLine.startsWith("GET /location?") && currentLine.endsWith("HTTP/1.1")) {
          if (!ignore) {
            int startOfLat = currentLine.indexOf('=') + 1;
            int endOfLat = currentLine.indexOf('&') - 1;
            int startOfLon = currentLine.indexOf('=', endOfLat) + 1;
            int endOfLon = currentLine.indexOf(' ', startOfLon) - 1;
            clientLatitude = currentLine.substring(startOfLat, endOfLat).toFloat();
            clientLongitude = currentLine.substring(startOfLon, endOfLon).toFloat();
            Serial.println("");
            Serial.print("Client latitude: "); Serial.println(clientLatitude);
            Serial.print("Client longitude: "); Serial.println(clientLongitude);

            // angleBetween(clientLatitude, clientLongitude, myLatitude, myLongitude);
            // float angle = angleBetween(-26.468718, 153.093456, -26.468478, 153.093402);
            float angle = angleBetween(clientLatitude, clientLongitude, myLatitude, myLongitude);
            displayAngle(angle);
          } else {
            ignore = false;
          }
        }

      }
    }
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }

}

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);

}

void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0) {
      Serial.print(":");
    }
  }
  Serial.println();
}