
// Code by : ScitiveX
// DIY Smartwatch using ESP32 C3
//    ___              _      _        _                    __  __
//   / __|    __      (_)    | |_     (_)    __ __    ___   \ \/ /
//   \__ \   / _|     | |    |  _|    | |    \ V /   / -_)   >  <
//   |___/   \__|_   _|_|_   _\__|   _|_|_   _\_/_   \___|  /_/\_\  
// _|"""""|_|"""""|_|"""""|_|"""""|_|"""""|_|"""""|_|"""""|_|"""""|
// "`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'
//

// Video Link :
// https://youtu.be/6nNxSrAhyX4

#include <WiFi.h>

#include <ArduinoJson.h>
#include <TFT_eSPI.h>

#include "icon.h"
#include "wall.h"

TFT_eSPI tft = TFT_eSPI();

#define WIDTH 172
#define HEIGHT 320

const char* ssid = "**";   //  ssid
const char* password = "********";  //  password

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 5.5 * 3600;
const int daylightOffset_sec = 0;

char timeString[10];
char dateString[20];
int hr, minute, sec;

WiFiServer server(80);

void setup() {
  tft.begin();             // initialize a ST7789 chip
  tft.setSwapBytes(true);  // Swap the byte order for pushImage() - corrects endianness

  tft.fillScreen(TFT_BLACK);
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.print("ESP device IP address: ");
  Serial.println(WiFi.localIP());
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(5, 5);
  tft.print(WiFi.localIP());
  delay(5000);
  tft.fillRect(0, 0, 172, 50, TFT_BLACK);

  // Start the server
  server.begin();
  Serial.println("Server started");

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() {
  printLocalTime();
  WiFiClient client = server.available();

  if (client) {
    Serial.println("New client connected");
    String request = client.readString();
    String request_f = request.substring(request.indexOf("/data") + 6, request.indexOf("HTTP") - 1);
    Serial.println(request);
    Serial.println(request_f);
    String decodedJson = decodeUrlEncodedJson(request_f);

    // Check if the request is for the JSON data
    if (request.indexOf("/data") != -1) {
      // Parse the JSON data and split into different variables
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, decodedJson);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
      }
      String packageName = doc["packageName"];
      String title = doc["title"];
      String text = doc["text"];

      // Do something with the data
      Serial.print("Package name: ");
      Serial.println(packageName);
      Serial.print("Title: ");
      Serial.println(title);
      Serial.print("Text: ");
      Serial.println(text);

      // Send a response to the client
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println();
      client.println("Received JSON data:");
      client.print("Package name: ");
      client.println(packageName);
      client.print("Title: ");
      client.println(title);
      client.print("Text: ");
      client.println(text);
      //tft.fillScreen(TFT_BLACK);
      tft.fillRect(0, 0, 172, 100, TFT_BLACK);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextSize(2);

      if (packageName.indexOf("com.whatsapp") != -1) {
        tft.pushImage(5, 5, 25, 25, whatsapp_icon);
      } else if (packageName.indexOf("com.android.incallui") != -1) {
        tft.pushImage(5, 5, 25, 25, call_icon);
      }

      if (text.length() > 12) {
        // Truncate input to 9 characters and add "..." at the end
        text = text.substring(0, 9) + " ...";
      }

      if (title.length() > 11) {
        // Truncate input to 9 characters and add "..." at the end
        title = title.substring(0, 10);
      }
      tft.setCursor(38, 13);
      tft.print(title);
      tft.setCursor(5, 33);
      tft.print(text);


    }

    // Close the connection
    client.stop();


    Serial.println("Client disconnected");
  }

  if (hr < 16) {

    tft.pushImage(0, 172, 172, 172, day_);
  } else {
    tft.pushImage(0, 172, 172, 172, night);
  }

  tft.setTextColor(convertRGBto565(70, 242, 22), TFT_BLACK);
  tft.setTextSize(4);
  tft.setCursor(28, 100);
  tft.print(timeString);

  tft.setTextColor(convertRGBto565(250, 255, 110), TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(5, 150);
  tft.print(dateString);
}

String decodeUrlEncodedJson(String encodedJson) {
  String decodedJson = encodedJson;
  decodedJson.replace("%7B", "{");
  decodedJson.replace("%22", "\"");
  decodedJson.replace("%7D", "}");
  decodedJson.replace("%20", " ");

  return decodedJson;
}


unsigned int convertRGBto565(int r, int g, int b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}


void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }

  // Format the time into two separate strings
  timeString[10];
  dateString[20];

  strftime(timeString, sizeof(timeString), "%H:%M", &timeinfo);
  strftime(dateString, sizeof(dateString), "%e %B %Y", &timeinfo);

  hr = timeinfo.tm_hour;
  minute = timeinfo.tm_min;
  sec = timeinfo.tm_sec;

  Serial.print("Time: ");
  Serial.println(timeString);
  Serial.print("Date: ");
  Serial.println(dateString);
}
