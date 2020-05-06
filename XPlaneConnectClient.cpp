#include <Arduino.h>
#include "XPlaneConnectClient.h"

#ifndef XPlaneConnectClient_class
    #define XPlaneConnectClient_class

    XPlaneConnectClient::XPlaneConnectClient (char* udpAddress, int udpPort)
        {
            this->udpAddress = udpAddress;
            this->udpPort = udpPort;
        };


    int XPlaneConnectClient::sendUDP(WiFiUDP &sock, char buffer[], int len)
        {
        //Serial.println("Sending packet");
        sock.beginPacket(udpAddress,udpPort);
        for (int i = 0; i < len; i++)
        {
            sock.write((int)buffer[i]);
        }
        if (sock.endPacket() == 0) {
            Serial.println("packet FAILED");
            return 1;
        }
        return 0;
        };

    int XPlaneConnectClient::sendText(WiFiUDP &sock, char* msg, int x, int y)
    {
    if (msg == NULL)
    {
        msg = strdup("");
    }
    size_t msgLen = strnlen(msg, 255);
    // Input Validation
    if (x < -1)
    {
        Serial.println("sendTEXT: x should be positive (or -1 for default).");
        // Technically, this should work, and may print something to the screen.
    }
    if (y < -1)
    {
        Serial.println("sendTEXT: y should be positive (or -1 for default).");
        // Negative y will never result in text being displayed.
        return -1;
    }
    if (msgLen > 255)
    {
        Serial.println("sendTEXT: msg must be less than 255 bytes.");
        return -2;
    }
    char buf[14 + msgLen +1];
    strcpy(&buf[0], "TEXT");
    memcpy(buf + 5, &x, sizeof(int));
    memcpy(buf + 9, &y, sizeof(int));
    buf[13] = msgLen;
    strcpy(&buf[14], msg);
    sendUDP(sock, buf, msgLen + 14);

    return 0;
    }

    int XPlaneConnectClient::getDREFResponse(WiFiUDP &sock)
    {
    char packetBuffer[255]; //buffer to hold incoming packet
    int packetSize = sock.parsePacket();
    if (packetSize) {
        int len = sock.read(packetBuffer, 255);
        if (len > 0) {
        packetBuffer[len] = 0;
        }

        float dataref;
        if (packetSize >= 7 + sizeof(float)) {
            memcpy(&dataref, packetBuffer + 7, sizeof(float));
            // float Tach_RPM = dataref * 180.0 / (6 * PI); // convert rad/sec to rev/min
            // Serial.printf("IAS: %f", Tach_RPM);
            if (dataref < 0)
            {
                dataref = 0;
            }
            Serial.printf("IAS: %f", dataref);
            Serial.println();
        }

    }
    else
    {
        Serial.println("no packet received");
        return -1;
    }

    return 0;
    }



    int XPlaneConnectClient::sendCTRL(WiFiUDP &sock, float values[], int size, char ac)
    {
    // Validate input
    if (ac < 0 || ac > 20)
    {
    // Serial.println("sendCTRL: aircraft should be a value between 0 and 20.");
        return -1;
    }
    if (size < 1 || size > 7)
    {
    // Serial.println("sendCTRL: size should be a value between 1 and 7.");
        return -2;
    }

    // Setup Command
    // 5 byte header + 5 float values * 4 + 2 byte values
    char buffer[31] = "CTRL";
    int cur = 5;
    int i; // iterator
    for (i = 0; i < 6; i++)
    {
        float val = -998;

        if (i < size)
        {
        val = values[i];
        }
        if (i == 4)
        {
        buffer[cur++] = val == -998 ? -1 : (char)val;
        }
        else
        {
        *((float*)(buffer + cur)) = val;
        cur += sizeof(float);
        }
    }
    buffer[26] = ac;
    *((float*)(buffer + 27)) = size == 7 ? values[6]: -998;
    // Send Command
    if (sendUDP(sock, buffer, 31) < 0)
    {
        //Serial.println("sendCTRL: Failed to send command");
        return -3;
    }
    return 0;
    }

#endif