#include <SPI.h>
#include <WiFi101.h>
#include <MPU9250_WE.h>
#include <Adafruit_NeoPixel.h>

#define MPU9250_ADDR 0x68
#define PI 3.1415926535897932384626433832795

#define RAINBOW_PIN 0
#define NUMPIXELS 13

MPU9250_WE myMPU9250 = MPU9250_WE(MPU9250_ADDR);

Adafruit_NeoPixel pixels(NUMPIXELS, RAINBOW_PIN, NEO_GRB + NEO_KHZ800);

//#include "arduino_secrets.h" 
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
#define SECRET_SSID "FriendFinder"
#define SECRET_PASS "ABBADEAF01"
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
//int keyIndex = 0;            // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;

// Initialize the WiFi client library
WiFiClient client;

// server address:
// char server[] = "example.org";
// IPAddress server(64,131,82,241);

unsigned long lastConnectionTime = 0;            // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 1L * 1000L; // delay between updates, in milliseconds

// Store both our location, and the servers location (once we find it out)
float myLatitude = -26.6744;
float myLongitude = 153.0560;
float serverLatitude = -26.6743;
float serverLongitude = 153.0561;

String data = "";

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  Wire.begin();
  pixels.begin();

  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(0, 20, 0));
  pixels.setPixelColor(6, pixels.Color(0, 20, 0));
  pixels.setPixelColor(12, pixels.Color(0, 20, 0));
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

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  // attempt to connect to WiFi network:
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, 0, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  // you're connected now, so print out the status:
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
  // if there's incoming data from the net connection.
  // send it out the serial port.  This is for debugging
  // purposes only:
  data = "";
  while (client.available()) {
    char c = client.read();
    // Serial.write(c);
    data += c;
  }
  if (data.length() > 0) {
    Serial.println(data);
    int startOfLat = data.indexOf(':') + 2;
    int endOfLat = data.indexOf(',') - 1;
    int startOfLon = data.indexOf(':', endOfLat) + 2;
    int endOfLon = data.indexOf(' ', startOfLon) - 1;
    serverLatitude = data.substring(startOfLat, endOfLat).toFloat();
    serverLongitude = data.substring(startOfLon, endOfLon).toFloat();

    // angleBetween(serverLatitude, serverLongitude, myLatitude, myLongitude);
    // float angle = angleBetween(-26.468478, 153.093402, -26.468718, 153.093456);
    float angle = angleBetween(serverLatitude, serverLongitude, myLatitude, myLongitude);
    displayAngle(angle);
  }

  // if ten seconds have passed since your last connection,
  // then connect again and send data:
  if (millis() - lastConnectionTime > postingInterval) {
    httpRequest();
  }

}

// this method makes a HTTP connection to the server:
void httpRequest() {
  // close any connection before send a new request.
  // This will free the socket on the WiFi shield
  client.stop();

  // if there's a successful connection:
  if (client.connect(WiFi.gatewayIP(), 80)) {
    Serial.println("connecting...");
    // send the HTTP PUT request:
    char getRequestBuffer[64];
    sprintf(getRequestBuffer, "GET /location?lat=%f&lon=%f HTTP/1.1", myLatitude, myLongitude);
    //client.println("GET / HTTP/1.1");
    client.println(getRequestBuffer);
    client.print("Host: ");
    client.println(WiFi.gatewayIP());
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Connection: close");
    client.println();

    // note the time that the connection was made:
    lastConnectionTime = millis();
  }
  else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
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
}