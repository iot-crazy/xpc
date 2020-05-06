//
// (C) Copyright IoT Crazy 2020
//
// 
//
//
//



#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "XPlaneConnectClient.h"

int JoyStick_X = 34; //x
int JoyStick_Y = 35; //y
int JoyStick_Z = 35; //key
int rightStick_X = 36;
int rightStick_Y = 39;
int btn1 = 25;
int btn2 = 26;

// WiFi network name and password:
const char * networkName = "YOUR_WIFI_SSID";
const char * networkPswd = "YOUR_WIFI_PASSWORD";

//IP address to send UDP data to:
// either use the ip address of the server or 
// a network broadcast address
char udpAddress[] = "IP_ADDRESS_OF_PC_RUNNING_XPLANE";
const int udpPort = 49009;

int prevx, prevy, prevrx, prevry = 0;
int minChange = 20;
int numSamples = 20;
bool sentWelcome = false;
float stickMax = 4299;
float stickMin = 0;
float stickMidY = 2463; 
float stickMidX = 2584; ;

//Are we currently connected?
boolean connected = false;

//The udp library class
WiFiUDP sock;

//XpcSocket sock(udpAddress, udpPort);
XPlaneConnectClient xpc(udpAddress, udpPort);

void WiFiEvent(WiFiEvent_t event){
    switch(event) {
      case SYSTEM_EVENT_STA_GOT_IP:
          //When connected set 
          Serial.print("WiFi connected! IP address: ");
          Serial.println(WiFi.localIP());  
          //initializes the UDP state
          //This initializes the transfer buffer
          if(sock.begin(WiFi.localIP(),udpPort) == 1) {
            Serial.println("UDP success");
          }
          connected = true;
          break;
      case SYSTEM_EVENT_STA_DISCONNECTED:
          Serial.println("WiFi lost connection");
          connected = false;
          break;
      default: break;
    }
}

void connectToWiFi(const char * ssid, const char * pwd){
  Serial.println("Connecting to WiFi network: " + String(ssid));

  // delete old config
  WiFi.disconnect(true);
  //register event handler
  WiFi.onEvent(WiFiEvent);
  
  //Initiate connection
  WiFi.begin(ssid, pwd);

  Serial.println("Waiting for WIFI connection...");
}


int GetIAS()
{
  //float tVal[1];
	//int tSize = 1;

  /*if (getDREF(sock, "sim/test/test_float", tVal, &tSize) < 0)
	{
		printf("Error establishing connecting. Unable to read data from X-Plane.");
		return -1;
	}*/

  char dref[] = "sim/flightmodel/position/indicated_airspeed";
  // Setup command
	// 6 byte header + potentially 255 drefs, each 256 chars long.
	// Easiest to just round to an even 2^16.
	char buffer[1024]; // = "GETD";
  strcpy(&buffer[0], "GETD");
	buffer[5] = 1;
	int len = 6;
  size_t drefLen = strnlen(dref, 256);
  if (drefLen > 255)
  {
    Serial.println("getDREFs dref is too long.");
    return -1;
  }
 
  buffer[len++] = (char)drefLen;
  strncpy(buffer + len, dref, drefLen);
  len += drefLen;
	
	// Send Command
	xpc.sendUDP(sock, buffer, len);
  // delay(250);
  xpc.getDREFResponse(sock);
  
  return 0;
}


void setup() {
  Serial.begin(115200);
	delay(10);
  pinMode(JoyStick_Z, INPUT); 
  pinMode(JoyStick_X, INPUT); 
  pinMode(JoyStick_Y, INPUT); 
  pinMode(rightStick_X, INPUT); 
  pinMode(rightStick_Y, INPUT);
  pinMode(btn1, INPUT);
  pinMode(btn2, INPUT);

  connectToWiFi(networkName, networkPswd);
  sock.setTimeout(4050);
 
  while (connected == false)
  {
    delay(10);    
  }
}

int iasCount = 0;

void loop() {

  iasCount++; 

  if (sentWelcome == false) {
    Serial.println("Sending welcome message");
    char msg[] = "Stemulator Connected!";
    xpc.sendText(sock, msg , 500, 500);

    Serial.println("Request data ref");
    GetIAS();
    sentWelcome = true;
  }

  float CTRL[5] = { 0.0 };
  int x,y,z,ax,ay,rx,ry,arx,ary = 0;
  ax = 0;
  ay=0;
  rx = 0;
  ry = 0;
  arx = 0;
  ary = 0;

  for(int i = 0; i<=numSamples; i++)
  {
    // multiple reads to help cope with the effects of multiplexed ADC channels on the ESP32
    (void)analogRead(JoyStick_X);
    delay(1);
    ax += analogRead(JoyStick_X);
    delay(1);
    (void)analogRead(JoyStick_Y);
    delay(1);
    ay += analogRead(JoyStick_Y);
    delay(1);
    (void)analogRead(rightStick_X);
    delay(1);
    arx += analogRead(rightStick_X);
    delay(1);
    (void)analogRead(rightStick_Y);
    delay(1);
    ary += analogRead(rightStick_Y);
  }

  x = ax / numSamples;
  y = ay / numSamples;
  rx = arx / numSamples;
  ry = ary / numSamples;

  if (abs(x - prevx) > minChange || abs(y - prevy) > minChange
    || abs(rx - prevrx) > minChange || abs(ry - prevry) > minChange)
  {
    Serial.println(abs(x - prevx));
    Serial.println(abs(y - prevy));
    Serial.printf("Left: %d,%d difs: %d,%d \n", x, y, abs(x-prevx), abs(y-prevy));
    Serial.printf("Right: %d,%d difs: %d,%d \n", rx, ry, abs(rx-prevrx), abs(ry-prevry));
    prevx = x;
    prevy = y;
    prevrx = rx;
    prevry = ry;
    z=digitalRead(JoyStick_Z);

    float rudder = 0;
    if (y >= stickMidY)
    {
        rudder = (y - stickMidY) / (stickMax - stickMidY);
    }
    else
    {
      rudder =  -((stickMidY - y) / stickMidY);
    }

    // elevator - x
    float elevator = 0;
    if (rx >= stickMidX)
    {
        elevator = (rx - stickMidX) / (stickMax - stickMidX);
    }
    else
    {
        elevator =  -((stickMidX - rx) / stickMidX);
    }

    // Aileron
    float aileron = 0;
    if (ry >= stickMidY)
    {
        aileron = (ry - stickMidY) / (stickMax - stickMidY);
    }
    else
    {
      aileron =  -((stickMidY - ry) / stickMidY);
    }

    //send throttle
    float throttle = (float)x / stickMax;
    CTRL[0] = - elevator; // elevator
    CTRL[1] = aileron ; // Aileron
    CTRL[2] = rudder; // Rudder
    CTRL[3] = throttle; // Throttle
    
    xpc.sendCTRL(sock, CTRL, 5, 0);
    Serial.printf("T: %f R: %f A: %f E: %f \n", throttle, rudder, aileron, elevator);
    
    GetIAS();
    iasCount=0;
    
  }

  //Serial.printf("btns: %d, %d", digitalRead(btn1), digitalRead(btn2));

  // if the sticks haven't moved for a while, get the IAS - this really needs improving and called from a timer instead!
  if (iasCount > 10)
  {
    GetIAS();
      iasCount=0;
  }

  delay(10);
  
}
