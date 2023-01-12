//ChlorGen
//By: Anthony Maida
//Purpose: Switch the polarity of plates based of user time.
//Will go to sleep based of the range the user sets.
//Board: ESP32

//Pin Layout:
//RGB LED: Red = 32, Green = 33, Blue = 25, GND = GND
//Buzzer: POS = 12, GND = GND
//OLED Screen: SDA = 21, SCL = 22, VCC = 3V3, GND = GND
//Motor Controller: POS = 3V3, RPMW/White Wire = 26, LPMW/Yellow Wire = 27, GND = GND
//Power Source: Positive = VIN, Negative = GND


#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Put your information here
const char* ssid = "WIFI_SSID";
const char* password = "WIFI_PASSWORD";

AsyncWebServer server(80);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Wifi date and time info declarations
String formattedDate;
String dayStamp;
String timeStamp;
int timeHour;
int timeMinute;
int timeSecond;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
bool hourFrame;
bool minuteFrame;

// OLED Screen declarations
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// RGB Led Pin declarations
const int redPin = 32;
const int greenPin = 33;
const int bluePin = 25;

const int redChannel = 1;
const int greenChannel = 2;
const int blueChannel = 3;

//Motor control pins
int RPWM_Output = 26; // Arduino PWM output pin 26; connect to IBT-2 pin 1 (RPWM)
int LPWM_Output = 27; // Arduino PWM output pin 27; connect to IBT-2 pin 2 (LPWM)

const int rightChannel = 4;
const int leftChannel = 5;

int rightState = LOW;
int leftState = HIGH;

//Buzzer Pin declarations
const int buzzerPin = 12;

unsigned long startMillis;
unsigned long currentMillis;
unsigned long pauseMillis;
unsigned long pauseTime = 0;
int currentTime;
int timeLeft;

const int freq = 5000;
const int resolution = 8;

char brandName[] = {'C', 'h', 'l', 'o', 'r', 'G', 'e', 'n'};

//DEFAULT
int defUserMin = 1;

//USER INPUTED TIME
int userMin = 1;

float switchTime;
int userChangeTotal = 0;

int count = 0;
int sleepCount = 0;
int cycles = 0;

//Default time range to sleep
int defBegHour = 23;
int defEndHour = 1;

//User inputed time range
int begHour = 23;
int endHour = 1;

const int begMin = 0;
const int endMin = 0;

const char* PARAM_INPUT_1 = "input1";
const char* PARAM_INPUT_2 = "input2";
const char* PARAM_INPUT_3 = "input3";
const char* PARAM_INPUT_4 = "input4";

String begStrH;
String endStrH;
String begStrM;
String endStrM;

//Functions for what to display
bool sleepMode;

// HTML web page to handle 3 input fields (input1, input2, input3)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <form action="/get">
    Beginning Hour: <input type="text" name="input1">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    End Hour: <input type="text" name="input2">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    Timer Minutes: <input type="text" name="input3">
    <input type="submit" value="Submit">
  </form><br>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) 
{
  request->send(404, "text/plain", "Not found");
}

void setup() 
{
  Serial.begin(9600);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) 
  {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  WiFi.mode(WIFI_STA);
  
  ledcSetup(0,1E5,12);
  
  ledcSetup(1,freq,resolution);
  ledcSetup(2,freq,resolution);
  ledcSetup(3,freq,resolution);
  
  ledcSetup(4,freq,resolution);
  ledcSetup(5,freq,resolution);
  
  ledcAttachPin(buzzerPin,0);

  ledcAttachPin(redPin,1);
  ledcAttachPin(greenPin,2);
  ledcAttachPin(bluePin,3);

  ledcAttachPin(RPWM_Output,4);
  ledcAttachPin(LPWM_Output,5);
  
  intro();
  
  //Turn Blue Led On
  ledcWrite(blueChannel, 255);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(5,1);
  display.print("Connecting To: ");
  display.setCursor(5,17);
  display.print(ssid);
  display.display();
  delay(3000);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(250);
    display.print(".");
    display.display();
  }
  
  display.clearDisplay();
  display.setCursor(5,1);
  display.print("WiFi Connected");
  display.setCursor(5,17);
  display.print("IP address: ");
  display.setCursor(5,33);
  display.print(WiFi.localIP());
  display.display();
  delay(5000);
  
  timeClient.begin();
  timeClient.setTimeOffset(-25200);

  while(!timeClient.update()) 
  {
     timeClient.forceUpdate();
  }
  
  //Turn Blue Led Off
  ledcWrite(blueChannel, 0);

  ledcWrite(rightChannel,0);
  ledcWrite(leftChannel,255);

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send_P(200, "text/html", index_html);
  });

  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) 
  {
    String inputMessage;
    String firstInput;
    String secondInput;
    String thirdInput;
    String inputParam;

    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) 
    {
      firstInput = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputParam + ") with value: " + firstInput + "<br><a href=\"/\">Return to Home Page</a>");
      
      begHour = firstInput.toInt();
    }
    
    // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_2)) 
    {
      secondInput = request->getParam(PARAM_INPUT_2)->value();
      inputParam = PARAM_INPUT_2;
      request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputParam + ") with value: " + secondInput +
                                     "<br><a href=\"/\">Return to Home Page</a>");
      
      endHour = secondInput.toInt();
    }
    
    // GET input3 value on <ESP_IP>/get?input3=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_3)) 
    {
      thirdInput = request->getParam(PARAM_INPUT_3)->value();
      inputParam = PARAM_INPUT_3;
      request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputParam + ") with value: " + thirdInput +
                                     "<br><a href=\"/\">Return to Home Page</a>");
      
      userMin = thirdInput.toInt();
    }
    
    else 
    {
      inputMessage = "No message sent";
      inputParam = "none";
      request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputParam + ") with value: " + inputMessage +
                                     "<br><a href=\"/\">Return to Home Page</a>");
    }    

  });
  server.onNotFound(notFound);
  server.begin();
  
  startMillis = millis();
}

// Main loop function
void loop() 
{ 
  switchTime = (defUserMin * 60);
  
  currentMillis = millis();
  
  currentTime = currentMillis/1000 - switchTime*count - startMillis/1000 - pauseTime/1000 - userChangeTotal;
  timeLeft = switchTime-currentTime;
  
  formattedDate = timeClient.getFormattedDate();
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);

  timeHour = timeClient.getHours();
  timeMinute = timeClient.getMinutes();
  timeSecond = timeClient.getMinutes();

  if(defBegHour < 10)
  {
    begStrH = "0" + String(defBegHour);
  }
  else
  {
    begStrH = String(defBegHour);
  }

  if(defEndHour < 10)
  {
    endStrH = "0" + String(defEndHour);
  }
  else
  {
    endStrH = String(defEndHour);
  }

  if(begMin < 10)
  {
    begStrM = "0" + String(begMin);
  }
  else
  {
    begStrM = String(begMin);
  }

  if(endMin < 10)
  {
    endStrM = "0" + String(endMin);
  }
  else
  {
    endStrM = String(endMin);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  if(defEndHour >= defBegHour)
  {
    if(timeClient.getHours() >= defBegHour && timeClient.getHours() < defEndHour)
    {
      sleepMode = true;
      sleepBuzz();
    }
  
    else
    {
      sleepMode = false;
      sleepBuzz();
    }
  }

  else if(defEndHour < defBegHour)
  {
    if(timeClient.getHours() >= defEndHour && timeClient.getHours() < defBegHour)
    {
      sleepMode = false;
      sleepBuzz();
    }
  
    else
    {
      sleepMode = true;
      sleepBuzz();
    }
  }
  
    
  if(sleepMode == true)
  {
    sleepDisplay(timeLeft, timeStamp, switchTime);
    buzzerMotorLed(sleepMode, switchTime, timeLeft, currentTime, 0, 0, 0, 0, 0);
  }
  else
  {
    infoDisplay(timeLeft, cycles, dayStamp, timeStamp);
    buzzerMotorLed(sleepMode, switchTime, timeLeft, currentTime, 255, 255, 0, 255, 255);  //Third number is audio level
  }
}



// This function is called to display the sleep screen
int sleepDisplay(int timeLeft, String timeStamp, int switchTime)
{
    display.setTextSize(1);
    if(timeLeft > (switchTime-1)*.667)
    {
      display.clearDisplay();
      display.setCursor(0,1);
      display.print("Sleeping.");
    }
    else if(timeLeft > (switchTime-1)*.333)
    {
      display.clearDisplay();
      display.setCursor(0,1);
      display.print("Sleeping..");
    }
    else if(timeLeft > 0)
    {
      display.clearDisplay();
      display.setCursor(0,1);
      display.print("Sleeping...");
    }

    display.setCursor(0,40);
    display.print("Time:");
    display.setCursor(45,40);
    display.print(timeStamp);
    display.setCursor(0,55);
    display.print("Sleep:");
    display.setCursor(40,55);
    display.print(begStrH + ":" + begStrM + " To " + endStrH + ":" + endStrM);
    display.display();
}



// This function is called to display the normal screen
int infoDisplay(int timeLeft, int cycles, String dayStamp, String timeStamp)
{
  display.setCursor(0,1);
  display.print("Timer:");
  display.setCursor(45,1);
  display.print(timeLeft);
  display.setCursor(0,14);
  display.print("Cycles:");
  display.setCursor(45,14);
  display.print(cycles);
  display.setCursor(0,27);
  display.print("Date:");
  display.setCursor(45,27);
  display.print(dayStamp);
  display.setCursor(0,40);
  display.print("Time:");
  display.setCursor(45,40);
  display.print(timeStamp);
  display.setCursor(0,55);
  display.print("Sleep:");
  display.setCursor(40,55);
  display.print((begStrH + ":" + begStrM + " To " + endStrH + ":" + endStrM));
  display.display();
}



// This function is used to light up the RGB, make a sounds and also count when the countdown reaches 0.
int buzzerMotorLed(bool sleepMode, float switchTime, int timeLeft, int currentTime, int redNum, int greenNum, int buzzNum, int rightValue, int leftValue)
{
  if(timeLeft == 10)
  {
    ledcWrite(redChannel, redNum);
  }
  
  if(timeLeft == 1.00000)
  {
    ledcWrite(redChannel, 0);
    ledcWrite(greenChannel, greenNum);
    ledcWriteTone(0,buzzNum);
  }
  
  else if(timeLeft == 0.00000)
  {
    if(rightState == HIGH)
    {
      rightState = LOW;
      leftState = HIGH;
      ledcWrite(rightChannel, 0);
      ledcWrite(leftChannel, leftValue);
    }
    
    else if(rightState == LOW)
    {
      rightState = HIGH;
      leftState = LOW;
      ledcWrite(leftChannel, 0);
      ledcWrite(rightChannel, rightValue);
    }
    
    ledcWriteTone(0,0);
    ledcWrite(greenChannel, 0);

    count += 1;
    if(sleepMode == false)
    {
      cycles += 1;
    }
    
    if(defUserMin != userMin || defBegHour != begHour || defEndHour != endHour)
    {
      if(defUserMin != userMin)
      {
        userChangeTotal = (userChangeTotal + (defUserMin * 60)) * count;
        defUserMin = userMin;
        count = 0;
      }
      if(defBegHour != begHour || defEndHour != endHour)
      {
        defBegHour = begHour;
        defEndHour = endHour;
      }
      
      ledcWriteTone(0,700);
      quickPause();
      delay(1000);
      quickPause();
      ledcWriteTone(0,0);
    }

  }

  return count;
  return cycles;
}


int sleepBuzz()
{
  if(timeClient.getHours() == defBegHour && timeClient.getMinutes() == 0 && timeClient.getSeconds() == 0)
  {
    ledcWriteTone(0,700);
  }
  
  else if(timeClient.getHours() == defBegHour && timeClient.getMinutes() == 0 && timeClient.getSeconds() == 5)
  {
    ledcWriteTone(0,0);
  }


  if(timeClient.getHours() == defEndHour && timeClient.getMinutes() == 0 && timeClient.getSeconds() == 0)
  {
    ledcWriteTone(0,700);
  }
  
  else if(timeClient.getHours() == defEndHour && timeClient.getMinutes() == 0 && timeClient.getSeconds() == 5)
  {
    ledcWriteTone(0,0);
  }
}

// Introduction screen
int intro()
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);

  display.setCursor(14,25);

  for(int i = 0; i <= 7; i += 1)
  {
    display.print(brandName[i]);
    display.display();
    ledcWriteTone(0,i*100+300);
    if(i <= 2)
    {
      ledcWrite(redChannel, 255);
    }
    else if(i <= 5)
    {
      ledcWrite(redChannel, 0);
      ledcWrite(greenChannel, 255);
    }
    else if(i <= 7)
    {
      ledcWrite(greenChannel, 0);
      ledcWrite(blueChannel, 255);
    }
    delay(180);
  }

  ledcWrite(blueChannel, 0);
  ledcWriteTone(0,0);
  delay(3000);
}

// Run this after a delay
int quickPause()
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(27,25);

  display.print("PAUSED");
  display.display();
  pauseMillis = millis();
  pauseTime = pauseMillis- currentMillis+pauseTime;
  currentMillis = millis();
  currentTime = currentMillis/1000 - switchTime*count - startMillis/1000 - pauseTime/1000;
}
