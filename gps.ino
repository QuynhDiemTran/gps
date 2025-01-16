#include <TinyGPSPlus.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>

#define Led 14
#define RXD2 16 // Chân RX GPS
#define TXD2 17 // Chân TX GPS
#define GPSBaud 9600
const char* ssid = "Phương Thảo";         
const char* password = "123456789"; 
const char* thingsBoardUrl = "http://thingsboard.cloud/api/v1/tNC7AyN0zNHdT7kD7464/telemetry";  
// NTP Server
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7 * 3600; 
const int daylightOffset_sec = 0;   

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, 60000); 
TinyGPSPlus gps;
HardwareSerial gpsSerial(2); // UART2

unsigned long lastSendTime = 0;  
const unsigned long sendInterval = 1000; 
double lastLat = 0.0, lastLng = 0.0; 
bool isFirstSend = true; 

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(GPSBaud, SERIAL_8N1, RXD2, TXD2);  
  pinMode(Led, OUTPUT); 
  digitalWrite(Led, LOW); 
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  // Khởi động NTP Client
  timeClient.begin();
  timeClient.update();
  Serial.println(F("Test GPS"));
  Serial.println();
}
void loop() {
  // Đọc dữ liệu từ GPS
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read())) {
      displayInfo(); 
      blinkLED();
    }
  }
  unsigned long currentMillis = millis();
  if (currentMillis - lastSendTime >= sendInterval) {
    lastSendTime = currentMillis; 
    processAndSendGPSData();
  }
}

void blinkLED() {
  digitalWrite(Led, HIGH);
  delay(500);
  digitalWrite(Led, LOW);
  delay(500);
}

void processAndSendGPSData() {
  if (gps.location.isValid()) {
    double lat = gps.location.lat();
    double lng = gps.location.lng();
    if (lat != lastLat || lng != lastLng) {
      lastLat = lat;
      lastLng = lng;
      sendDataToThingsBoard(lat, lng);
    }
  }
}

void sendDataToThingsBoard(double lat, double lng) {
  Serial.println("Sending data to ThingsBoard");
  HTTPClient http;
  http.begin(thingsBoardUrl);
  http.addHeader("Content-Type", "application/json");
  String payload;
  if (isFirstSend) {
    payload = "{\"driver_name\":\"Nguyen Van A\",\"driver_id\":\"12345\",\"timestamp2\":\"" + getCurrentDateTime() + "\",\"latitude\":" + String(lat, 6) + ",\"longitude\":" + String(lng, 6) + "}";
    isFirstSend = false;  
  } else {
    payload = "{\"latitude\":" + String(lat, 6) + ",\"longitude\":" + String(lng, 6) + "}";
  }
  int httpResponseCode = http.POST(payload);
  if (httpResponseCode > 0) {
    Serial.print("Response Code: ");
    Serial.println(httpResponseCode);
    Serial.println("Response payload: " + http.getString());
  } else {
    Serial.print("Error: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

String getCurrentDateTime() {
  timeClient.update(); // Cập nhật thời gian từ NTP server
  unsigned long epochTime = timeClient.getEpochTime();
  setTime(epochTime); 

  String formattedDateTime = String(year()) + "-" + String(month()) + "-" + String(day());//+ " " + String(hour()) + ":" + String(minute()) + ":" + String(second());
  return formattedDateTime; 
}

void displayInfo() {
  Serial.print(F("Location: ")); 
  if (gps.location.isValid()) {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  } else {
    Serial.print(F("INVALID"));
  }
  Serial.println();
}
