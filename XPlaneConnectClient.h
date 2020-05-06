//
// A attempt at an arduino port of the XPlaneClient 
// https://github.com/nasa/XPlaneConnect
//
//
// This is a work in progress and only a very limited set of features have been proted so far
//
// Huge thanks to all those involved with XPlaneConnect for this amazinh opprotunity!
// Please bear with me, I'm leaning C++ whilst writing this and thus there may be many examples of bad practice
// Always happy to receive feedback, let me know what I can improve!


#include <WiFiUdp.h>
#include <Arduino.h>

#ifndef XPlaneConnectClient_h
    #define XPlaneConnectClient_h
    class XPlaneConnectClient
    {

         private:
            char * udpAddress;
            int udpPort;

        public:
            XPlaneConnectClient(char* udpAddress, int udpPort);
            int sendUDP(WiFiUDP &sock, char buffer[], int len);
            int sendText(WiFiUDP &sock,char* msg, int x, int y);
            int getDREFResponse(WiFiUDP &sock);
            int sendCTRL(WiFiUDP &sock,float values[], int size, char ac);
    };
#endif
