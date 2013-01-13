/**
 * Copyright (c) 2009 Andrew Rapp. All rights reserved.
 *
 * This file is part of XBee-Arduino.
 *
 * XBee-Arduino is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * XBee-Arduino is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XBee-Arduino.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <XBee.h>
#include "WiFly.h"
#include "Credentials.h"
/*
This example is for Series 1 XBee (802.15.4)
Receives either a RX16 or RX64 packet and sets a PWM value based on packet data.
Error led is flashed if an unexpected packet is received
*/

XBee xbee = XBee();


// allocate two bytes for to hold a 10-bit analog reading
uint8_t payload[] = { 0, 0 };
Tx16Request tx = Tx16Request(0x5678, payload, sizeof(payload));
TxStatusResponse txStatus = TxStatusResponse();


XBeeResponse response = XBeeResponse();
// create reusable response objects for responses we expect to handle 
Rx16Response rx16 = Rx16Response();
Rx64Response rx64 = Rx64Response();


int light = 14;
int lightBut = 16;
boolean lightStatus = false;

uint8_t option = 0;
uint8_t data = 0;

byte tmpBuf[10];
char tmpBufIndex=0;
boolean readResponse()
{
  boolean response = false;  
  // after sending a tx request, we expect a status response
    // wait up to 5 seconds for the status response
    if (xbee.readPacket(5000)) 
    {
        // got a response!        
        // should be a znet tx status            	
    	if (xbee.getResponse().getApiId() == TX_STATUS_RESPONSE) 
        {
    	   xbee.getResponse().getZBTxStatusResponse(txStatus);    		
    	   // get the delivery status, the fifth byte
           if (txStatus.getStatus() == SUCCESS) 
           {
            // success.  time to celebrate
            Serial.println("success sending to zigbee node");
            response = true;
           } 
           else 
           {
             Serial.println("the remote XBee did not receive our packet. is it powered on?");
           }
        }      
    } 
    else if (xbee.getResponse().isError()) 
    {
      Serial.println("got error sending to zigbee node");
    } 
    else 
    {
      Serial.println("retry sending to zigbee node");
    }
    return response;
}


void setup() 
{
  pinMode(light, OUTPUT);
  pinMode(lightBut, INPUT);
  xbee.setSerial(Serial1);
  // start serial
  xbee.begin(9600);  
  Serial.begin(9600);
  Serial.println("initialized");
  WiFly.begin();  
  if (!WiFly.join(ssid, passphrase)) 
  {
    Serial.println("Association failed.");
    while (1) 
    {
      // Hang on failure.
    }
  }  
  Serial.println("Associated!");
}

// continuously reads packets, looking for RX16 or RX64
void loop() 
{    
  
  if(digitalRead(lightBut)==LOW)
  {
    delay(500);
    lightStatus = !lightStatus; 
    Serial.println(lightStatus);
  }
  digitalWrite(light, lightStatus?HIGH:LOW);
  while(SpiSerial.available() > 0) 
  {
    byte data = SpiSerial.read();
    tmpBuf[tmpBufIndex++] = data;
    Serial.write(data);    
    if(data=='#')//end of packet
    {
      boolean ack=false;
      tmpBufIndex = 0;
      if(tmpBuf[0]=='0')//address= self
      {
        if(tmpBuf[1]=='p')//refresh nodes data
        {
          
        }
        else if(tmpBuf[1]=='c')//command byte
        {
          //command in tmpBuf[2]
          if(tmpBuf[2]=='1')
          {
            //digitalWrite(light, HIGH);
            lightStatus = true;
          }
          else
          {
            //digitalWrite(light, LOW);
            lightStatus = false;
          }
          ack = true;
        }
      }
      else if(tmpBuf[0]=='1')//address= node
      {
        payload[0] = tmpBuf[2];
        payload[1]='0';//dummy
        Serial.println("Sending packet to zigbee node");
        xbee.send(tx);
        ack = readResponse();
      }
      if(ack==true)
      {
        SpiSerial.write('1');
      }
      else
      {
        SpiSerial.write('0');
      }
      SpiSerial.write(13);
      SpiSerial.write(10);
      SpiSerial.flush(); 
    }       
  } 
  xbee.readPacket();
    {    
      if (xbee.getResponse().isAvailable()) 
      {
        // got something   
        Serial.println("got something");   
        if (xbee.getResponse().getApiId() == RX_16_RESPONSE || xbee.getResponse().getApiId() == RX_64_RESPONSE) 
        {
          // got a rx packet        
          if (xbee.getResponse().getApiId() == RX_16_RESPONSE) 
          {
            xbee.getResponse().getRx16Response(rx16);
            option = rx16.getOption();
            data = rx16.getData(1);
            Serial.print("RX16: ");
            Serial.println(data);
          } 
          else 
          {
            xbee.getResponse().getRx64Response(rx64);
            option = rx64.getOption();
            data = rx64.getData(0);
            Serial.println("RX64: ");
            Serial.println(data);
          }
          
        } 
        else 
        {
        	// not something we were expecting
        }
      } 
      else if (xbee.getResponse().isError()) 
      {
        Serial.print("Error reading packet.  Error code: ");  
        Serial.println(xbee.getResponse().getErrorCode());
        // or flash error led
      } 
    }
    
}
