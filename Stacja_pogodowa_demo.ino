#include <LiquidCrystal_I2C.h>
#include <DHT.h> 
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <string.h>
#include <ArduinoJson.h>
#include<time.h>

#define dhtPIN D6 // definicja preprocesora
#define dhtTYPE DHT11 // definicja preprocesora
#define fotoPIN A0
#define redPIN D4//D5
#define greenPIN D3//D8
#define bluePIN D8//D4
#define switch1 D7 //D7
#define switch2 D5 //D0

#define SEA_LV_PRESSURE_HPA (1013.25)

//twożenie ikod do LCD
byte termometr[8] = {
  0b00100,
  0b00110,
  0b00100,
  0b00110,
  0b00100,
  0b01110,
  0b01110,
  0b00000
};
byte kropla[8] = {
  0b00100,
  0b00100,
  0b01110,
  0b01110,
  0b10111,
  0b10111,
  0b01110,
  0b00000
};
byte stopnie[8] = {
  0b00010,
  0b00101,
  0b00010,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};
byte lampka[8] = {
  0b00000,
  0b01110,
  0b10001,
  0b10001,
  0b10001,
  0b01110,
  0b01110,
  0b00100
};
byte zegar[8] = {
  0b00000,
  0b00100,
  0b01010,
  0b10101,
  0b10111,
  0b01010,
  0b00100,
  0b00000
};
const String endpoint = "http://api.openweathermap.org/data/2.5/weather?q=Krakow,pl&APPID="; //api for Cracow
const String key = "99172cd3279bf52f34e15e01e9808d04";

int chose_wifi=2;

static String wind_dir;

static float temperature;
static float humidity;
static float light;

static int i=0;

static float bmpTemperature;
static float bmpHumidity;
static float bmpPressure;
static float bmpAltitude;

static String redValue = "Off";
static String greenValue = "Off";
static String blueValue = "Off";
static float LEDbrightnes=0;

static int sw1 = 0b0;
static int sw2 = 0b0;

const char *ssid = " ";
const char *password = " ";

struct Weather {
    String main;
    String description;
    float temperature;
    float feels_like;
    int pressure;
    int humidity;
    float wind_speed;
    int wind_deg;
    int cloudiness;
    long dt;
    long timezone;
};

LiquidCrystal_I2C lcd(0x27, 20, 4); // tworzę obiekt klasy LiquidCrystal o nazwie lcd o podanych parametrach
DHT dht11(dhtPIN, dhtTYPE);
WiFiClient client;
HTTPClient http1;
Adafruit_BMP280 bmp;

ESP8266WebServer server(80); 

void parseWeatherJson(const char* json, Weather &weather) {
    // Zwiększony rozmiar bufora, aby pomieścić większy JSON
    const size_t capacity = 1024;
    DynamicJsonDocument doc(capacity);

    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }

    weather.main = doc["weather"][0]["main"].as<String>();
    weather.description = doc["weather"][0]["description"].as<String>();
    weather.temperature = doc["main"]["temp"].as<float>();
    weather.feels_like = doc["main"]["feels_like"].as<float>();
    weather.pressure = doc["main"]["pressure"].as<int>();
    weather.humidity = doc["main"]["humidity"].as<int>();
    weather.wind_speed = doc["wind"]["speed"].as<float>();
    weather.wind_deg = doc["wind"]["deg"].as<int>();
    weather.cloudiness = doc["clouds"]["all"].as<int>();
    weather.dt = doc["dt"].as<long>();
    weather.timezone = doc["timezone"].as<long>();
}

void setup() {
switch (chose_wifi)
{
  case 1:// Z telefonu
  {
    ssid = "Grzegorzwifi";
    password = "11235813";
    break;
  }
  case 2:// W domu
  {
    ssid = "domDarek_2";
    password = "b@racuda07";
    break;
  }
  case 3: //U Janka
  {
    ssid = "Poselska";
    password = "ALAmakota2017";
    break;
  }
  default://deafult
  {
    ssid = " ";
    password = " ";
    break;
  }
}

  bmp.begin(0x76);
  dht11.begin();
  lcd.begin(); // inicjalizuję lcd

  lcd.print("    ...Wait..."); // napis kontrolny

  lcd.createChar(0, termometr);
  lcd.createChar(1, kropla);
  lcd.createChar(2, stopnie);
  lcd.createChar(3, lampka);
  lcd.createChar(4, zegar);

  pinMode(redPIN, OUTPUT);
  pinMode(greenPIN, OUTPUT);
  pinMode(bluePIN, OUTPUT);
  pinMode(switch1, INPUT_PULLUP);
  pinMode(switch2, INPUT_PULLUP);
  //dht11.begin(); // inicjalizuję czujnik DHT

  Serial.begin(9600);

  connectToWiFi();
  server.on("/", handle_OnConnect);
  server.on("/Red", HTTP_GET, handle_Red);
  //server.on("/decreaseRed", HTTP_GET, handle_DecreaseRed);
  server.on("/Green", HTTP_GET, handle_Green);
  //server.on("/decreaseGreen", HTTP_GET, handle_DecreaseGreen);
  server.on("/Blue", HTTP_GET, handle_Blue);
  //server.on("/decreaseBlue", HTTP_GET, handle_DecreaseBlue);
  server.on("/White", HTTP_GET, handle_White);
  server.on("/off", HTTP_GET, handle_off);
  server.on("/brighten", HTTP_GET, handle_brighten);
  server.on("/dim", HTTP_GET, handle_dim);

  server.onNotFound(handle_NotFound);

  server.begin();
}

void loop() {
  Weather weather;
  Serial.println("IP Address: ");
  Serial.print(WiFi.localIP());
 
  server.handleClient();

  http1.begin(client, (endpoint + key).c_str());
  int httpCode = http1.GET();
  if (httpCode > 0) { 
 
        String payload = http1.getString();
        Serial.println(httpCode);
        delay(300);
        Serial.println(httpCode);
        delay(1000);
        Serial.println(payload);
        parseWeatherJson(payload.c_str(), weather);
        delay(1000);
      }
    else {
      Serial.println("Error on HTTP request");
    }
    http1.end(); 
// DHT reading
  int t=dht11.readTemperature();
  if(t<150)
        temperature = t;
      
   int h=dht11.readHumidity();
  if(h<150)
      humidity = h;     
      
  //fotorezystor reading
  light =analogRead(fotoPIN);
light=int((light/1024)*100);
//switch handeling
  int sw1= digitalRead(switch1);
  int sw2= digitalRead(switch2);

  if (isnan(temperature) || isnan(humidity)) { // sprawdzam czy nie ma błędu odczytu
    lcd.home(); // kursor na 0, 0
    lcd.print("Blad odczytu"); // wypisuję napis na lcd
    return; // wychodzę z pętli
  }

//wind dir 
if (weather.wind_deg >= 337.5 || weather.wind_deg  < 22.5) {
    wind_dir = "N";
  } else if (weather.wind_deg  >= 22.5 && weather.wind_deg  < 67.5) {
    wind_dir = "N-E";
  } else if (weather.wind_deg  >= 67.5 && weather.wind_deg  < 112.5) {
    wind_dir = "E";
  } else if (weather.wind_deg  >= 112.5 && weather.wind_deg  < 157.5) {
    wind_dir = "S-E";
  } else if (weather.wind_deg  >= 157.5 && weather.wind_deg  < 202.5) {
    wind_dir = "S";
  } else if (weather.wind_deg  >= 202.5 && weather.wind_deg  < 247.5) {
    wind_dir = "S-W";
  } else if (weather.wind_deg  >= 247.5 && weather.wind_deg  < 292.5) {
    wind_dir = "W";
  } else if (weather.wind_deg  >= 292.5 && weather.wind_deg  < 337.5) {
    wind_dir = "N-W";
  }

// time
time_t rawtime = weather.dt+weather.timezone;
struct tm * timeinfo;
timeinfo = localtime(&rawtime);

 lcd.home(); // kursor na 0, 0
lcd.clear();
 if(sw1==0 && sw2==0)
 {
      lcd.print("        Home:");
      lcd.setCursor(0,1);

      lcd.write((byte)0); // rzutuję ikonę termomrtr na typ byte i wyświetlam ją na lcd
        lcd.print(int(temperature));
       // wypisuję zmienną temp
      lcd.write((byte)2); // jak ww.
      lcd.print("C");
      lcd.print(" ");

      lcd.write((byte)1); // jak ww
      lcd.print(int(humidity)); // wypisuję zmienną wilg
      

      lcd.print("% ");

      lcd.print("Light:");
      lcd.print(int(light));
      lcd.print("% ");
      
      lcd.setCursor(0,2);

      lcd.print("BMP: Press:");
      lcd.print(int(bmp.readPressure()/100));
      lcd.print("hPa ");

      lcd.setCursor(0,3);

      lcd.print("Alt:");
      lcd.print(bmp.readAltitude(weather.pressure));
      lcd.print("m ");

      lcd.write((byte)0); // rzutuję ikonę termomrtr na typ byte i wyświetlam ją na lcd
      lcd.print(bmp.readTemperature()); // wypisuję zmienną temp
      lcd.write((byte)2); // jak ww.
      lcd.print("C");
      
  }
    else if(sw1==1 && sw2==0)
    {
      
      lcd.print("Krakow:  ");

      if(timeinfo->tm_mday<10)
      lcd.print("0");
      lcd.print(timeinfo->tm_mday);
      lcd.print(".");

      if(timeinfo->tm_mon<10)
      lcd.print("0");
      lcd.print(timeinfo->tm_mon + 1);
      lcd.print(".");
      lcd.print(timeinfo->tm_year + 1900);
      
      
      lcd.setCursor(0,1);

      lcd.write((byte)4);

      if(timeinfo->tm_hour<10)
      lcd.print("0");
      lcd.print(timeinfo->tm_hour);
      lcd.print(":");

      if(timeinfo->tm_min<10)
      lcd.print("0");
      lcd.print(timeinfo->tm_min);
      lcd.print(" ");

      lcd.write((byte)0); // rzutuję ikonę termomrtr na typ byte i wyświetlam ją na lcd
      lcd.print(weather.temperature-272.15); // wypisuję zmienną temp
      lcd.write((byte)2); // jak ww.
      lcd.print("C");
      lcd.print(" ");

      lcd.write((byte)1); // jak ww.
      lcd.print(weather.humidity); // wypisuję zmienną wilg
      lcd.print("% ");

      lcd.setCursor(0,2);

      lcd.print("Pres:");
      lcd.print(weather.pressure);
      lcd.print("hPa ");

      lcd.print("C:");
      lcd.print(weather.cloudiness);
      lcd.print("% ");

      lcd.setCursor(0,3);

      lcd.print("Wind:");
      lcd.print(weather.wind_speed);
      lcd.print("m/s Dir:");
      lcd.print(wind_dir);

   }
    else if(sw1==0 && sw2==1)
    {

      lcd.print("    Description:");

      lcd.setCursor(0, 1);

      lcd.print(weather.main);

      lcd.setCursor(0, 2);

      lcd.print(weather.description);
    } 
    else if(sw1==1 && sw2==1)
    {
      lcd.print("IP address: ");
      lcd.setCursor(0, 1);
      lcd.print(WiFi.localIP());
      
    }
}


void connectToWiFi() {
    Serial.println();
    Serial.println();
    WiFi.begin(ssid, password);
    int retries = 0;
  while ((WiFi.status() != WL_CONNECTED) && (retries < 15)) {
    retries++;
    delay(500);
    lcd.print(".");
  }
  if (retries > 14) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("WiFi connection FAILED"));
  }
  if (WiFi.status() == WL_CONNECTED) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("WiFi connected!"));
      /*delay(300);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("IP address: ");
      lcd.setCursor(0, 1);
      lcd.print(WiFi.localIP());
      lcd.clear();
      lcd.setCursor(0, 0);*/
      
  }
}


void handle_OnConnect() {
  int t=dht11.readTemperature();
  if(t<150)
      {
        temperature = t;
      }
   int h=dht11.readHumidity();
  if(h<150)
      {
      humidity = h;     
      }
  light =analogRead(fotoPIN);
light=int((light/1024)*100);

  bmpTemperature=bmp.readTemperature();
  bmpPressure=bmp.readPressure()/100;
  bmpAltitude=bmp.readAltitude();


  server.send(200, "text/html", SendHTML(temperature,humidity,bmpTemperature,bmpPressure,bmpAltitude, light, redValue, greenValue, blueValue)); 
}

void handle_Red()
{
  if (redValue == "On")
  {
      redValue="Off";
      digitalWrite(redPIN, HIGH);
  }else
  {
      redValue="On";
      digitalWrite(redPIN, LOW);
  }
    
  server.send(200, "text/html", SendHTML(temperature,humidity,bmpTemperature,bmpPressure,bmpAltitude, light, redValue, greenValue, blueValue));
}

void handle_Green()
{
 if (greenValue == "On")
  {
      greenValue="Off";
      digitalWrite(greenPIN, HIGH);
  }else
  {
      greenValue="On";
      digitalWrite(greenPIN, LOW);
  }
    
  server.send(200, "text/html", SendHTML(temperature,humidity,bmpTemperature,bmpPressure,bmpAltitude, light, redValue, greenValue, blueValue));
}

void handle_Blue()
{
  if (blueValue == "On")
  {
      blueValue="Off";
      digitalWrite(bluePIN, HIGH);
  }else
  {
      blueValue="On";
      digitalWrite(bluePIN, LOW);
  }
    
  server.send(200, "text/html", SendHTML(temperature,humidity,bmpTemperature,bmpPressure,bmpAltitude, light, redValue, greenValue, blueValue));
}
void handle_White()
{
  
  blueValue="On";
  digitalWrite(bluePIN, LOW);

  redValue="On";
  digitalWrite(redPIN, LOW);

  greenValue="On";
  digitalWrite(greenPIN, LOW);
    
  server.send(200, "text/html", SendHTML(temperature,humidity,bmpTemperature,bmpPressure,bmpAltitude, light, redValue, greenValue, blueValue));
}

void handle_off()
{
  blueValue="Off";
  digitalWrite(bluePIN, HIGH);

  redValue="Off";
  digitalWrite(redPIN, HIGH);

  greenValue="Off";
  digitalWrite(greenPIN, HIGH);
    
  server.send(200, "text/html", SendHTML(temperature,humidity,bmpTemperature,bmpPressure,bmpAltitude, light, redValue, greenValue, blueValue));
}
void handle_brighten()
{
  LEDbrightnes-=50;
   if(LEDbrightnes<0)
   LEDbrightnes=0;
   
   server.send(200, "text/html", SendHTML(temperature,humidity,bmpTemperature,bmpPressure,bmpAltitude, light, redValue, greenValue, blueValue));
}

void handle_dim()
{
  if(LEDbrightnes>255)
   LEDbrightnes=255;
   server.send(200, "text/html", SendHTML(temperature,humidity,bmpTemperature,bmpPressure,bmpAltitude, light, redValue, greenValue, blueValue));
}

void handle_NotFound()
{
  server.send(404, "text/plain", "Not found");
}

String SendHTML(float temperature,float humidity,float bmpTemperature,float bmpPressure,float bmpAltitude, int light, String red, String green, String blue){
String ptr = "<!DOCTYPE html> <html>\n";
ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
ptr += "<title>ESP8266 Weather Station</title>\n";
ptr += "<script>function Color(color) {"
       "var xhr = new XMLHttpRequest();"
       "xhr.open('GET',color, true);"
       "xhr.send();}</script>\n";
ptr += "<script>function turnOff(){"
       "var xhr = new XMLHttpRequest();"
       "xhr.open('GET', '/off', true);"
       "xhr.send();}</script>\n";
ptr += "<script>function brighten() {"
       "var xhr = new XMLHttpRequest();"
       "xhr.open('GET', '/brighten', true);"
       "xhr.send();}</script>\n";
ptr += "<script>function dim() {"
       "var xhr = new XMLHttpRequest();"
       "xhr.open('GET', '/dim', true);"
       "xhr.send();}</script>\n";
ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
ptr += "body{margin-top: 50px; background: linear-gradient(to bottom, #0099FF, #0044CC);}\n";
ptr += "h1 {color: #ffffff; margin: 50px auto 30px;}\n"; 
ptr += "table {margin: 0 auto; border-collapse: collapse; width: 50%;}\n";
ptr += "th, td {border: 1px solid #dddddd; text-align: left; padding: 8px; font-size: 24px;}\n";
ptr += "th {background-color: #f2f2f2;}\n";
ptr += ".button-container {display: flex; flex-direction: column; align-items: center;}\n";
ptr += ".button-row {display: flex; justify-content: center; gap: 10px; margin-bottom: 10px; flex-wrap: wrap;}\n";
ptr += "button {font-size: 24px; padding: 15px 30px; width: 150px; height: 60px; border: 2px solid black; color: white;}\n";
ptr += ".red {background-color: red;}\n";
ptr += ".green {background-color: green;}\n";
ptr += ".blue {background-color: blue;}\n";
ptr += ".white {background-color: white; color: black;}\n";
ptr += ".off {background-color: gray;}\n";
ptr += ".brighten {background-color: yellow; color: black;}\n";
ptr += ".dim {background-color: brown; color: white;}\n";
ptr += "@media (max-width: 600px) {\n";
ptr += "  .button-row {flex-direction: column;}\n";
ptr += "  button {width: 100%;}\n";
ptr += "}\n";
ptr += "</style>\n";
ptr += "</head>\n";
ptr += "<body>\n";
ptr += "<div id=\"webpage\">\n";
ptr += "<h1>Weather Station V2</h1>\n";
ptr += "<table>\n";
ptr += "<tr><th>Measurement</th><th>Value</th></tr>\n";
ptr += "<tr><td>DHT11 Temperature</td><td>";
ptr += temperature;
ptr += "&deg;C</td></tr>\n";
ptr += "<tr><td>DHT11 Humidity</td><td>";
ptr += humidity;
ptr += "%</td></tr>\n";
ptr += "<tr><td>Light</td><td>";
ptr += light;
ptr += "%</td></tr>\n";
ptr += "<tr><td>BMP Temperature</td><td>";
ptr += bmpTemperature;
ptr += "&deg;C</td></tr>\n";
ptr += "<tr><td>BMP Pressure</td><td>";
ptr += bmpPressure;
ptr += " hPa</td></tr>\n";
ptr += "<tr><td>BMP Altitude</td><td>";
ptr += bmpAltitude;
ptr += " m</td></tr>\n";
ptr += "</table>\n";
ptr += "<p>RGB LED:</p>";
ptr += "<div class=\"button-container\">\n";
ptr += "  <div class=\"button-row\">\n";
ptr += "    <button class=\"red\" onclick=\"Color('Red')\"> Red </button>\n";
ptr += "    <button class=\"green\" onclick=\"Color('Green')\">Green</button>\n";
ptr += "    <button class=\"blue\" onclick=\"Color('Blue')\"> Blue </button>\n";
ptr += "  </div>\n";
ptr += "  <div class=\"button-row\">\n";
ptr += "    <button class=\"white\" onclick=\"Color('White')\">White</button>\n";
ptr += "    <button class=\"off\" onclick=\"turnOff()\"> OFF </button>\n";
ptr += "  </div>\n";
ptr += "  <div class=\"button-row\">\n";
ptr += "    <button class=\"brighten\" onclick=\"brighten()\"> Brighten </button>\n";
ptr += "    <button class=\"dim\" onclick=\"dim()\"> Dim </button>\n";
ptr += "  </div>\n";
ptr += "</div>\n";
ptr += "<p>RED: ";
ptr += red;
ptr += "</p><p>GREEN: ";
ptr += green;
ptr += "</p><p>BLUE: ";
ptr += blue;
ptr += "</p><p>by Robert Zubek And Grzegorz Spirytulski</p>";
ptr += "</div>\n";
ptr += "</body>\n";
ptr += "</html>\n";
  return ptr;
}
