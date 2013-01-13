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
/*
This example is for Series 1 XBee
Sends a TX16 or TX64 request with the value of analogRead(pin5) and checks the status response for success
Note: In my testing it took about 15 seconds for the XBee to start reporting success, so I've added a startup delay
*/

XBee xbee = XBee();
unsigned long start = millis();

// allocate two bytes for to hold a 10-bit analog reading
uint8_t payload[] = { 0, 0 };

// with Series 1 you can use either 16-bit or 64-bit addressing

// 16-bit addressing: Enter address of remote XBee, typically the coordinator
Tx16Request tx = Tx16Request(0x1234, payload, sizeof(payload));

// 64-bit addressing: This is the SH + SL address of remote XBee
//XBeeAddress64 addr64 = XBeeAddress64(0x0013a200, 0x4008b490);
// unless you have MY on the receiving radio set to FFFF, this will be received as a RX16 packet
//Tx64Request tx = Tx64Request(addr64, payload, sizeof(payload));

TxStatusResponse txStatus = TxStatusResponse();


// create reusable response objects for responses we expect to handle 
Rx16Response rx16 = Rx16Response();
Rx64Response rx64 = Rx64Response();

uint8_t option = 0;
uint8_t data = 0;

int testLed = A15;
int testLedStatus = A14;


void setup() 
{
  pinMode(testLed, OUTPUT); 
  xbee.setSerial(Serial1);
  xbee.begin(9600);
  Serial.begin(9600);  
}

void loop() 
{
   
   // start transmitting after a startup delay.  Note: this will rollover to 0 eventually so not best way to handle
    if (millis() - start > 5000) 
    {
      // break down 10-bit reading into two bytes and place in payload
      int val = analogRead(testLedStatus);
      val = map(val, 0, 1023, 0, 255);
      payload[0] = 1;
      payload[1] = val;
      Serial.println("Sending packet");
      xbee.send(tx);
      start = millis();
    }   
  
    // after sending a tx request, we expect a status response
    // wait up to 5 seconds for the status response
    if (xbee.readPacket(5000)) 
    {
        if (xbee.getResponse().isAvailable()) 
        {
          // got something   
          Serial.println("got something");   
          if (xbee.getResponse().getApiId() == RX_16_RESPONSE || xbee.getResponse().getApiId() == RX_64_RESPONSE || xbee.getResponse().getApiId() == TX_STATUS_RESPONSE) 
          {
            // got a rx packet        
            if (xbee.getResponse().getApiId() == RX_16_RESPONSE) 
            {
              xbee.getResponse().getRx16Response(rx16);
              option = rx16.getOption();
              data = rx16.getData(0);
              Serial.println("RX16: ");
              Serial.println(data);
              if(data=='1')
              {
                digitalWrite(testLed, HIGH);
              }
              else
              {
                digitalWrite(testLed, LOW);
              }
            }
            else if (xbee.getResponse().getApiId() == TX_STATUS_RESPONSE) 
            {
                xbee.getResponse().getZBTxStatusResponse(txStatus);        		
        	   // get the delivery status, the fifth byte
               if (txStatus.getStatus() == SUCCESS) 
               {
                	 Serial.println("success.  time to celebrate");
              } 
               else 
               {
                	 Serial.println("the remote XBee did not receive our packet. is it powered on?");
               }
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
        } 
        else 
        {
           Serial.println("not something we were expecting");
          //flashLed(errorLed, 1, 25);    
        }
    } 
    else if (xbee.getResponse().isError()) 
    {
      Serial.println("got error");
      //nss.print("Error reading packet.  Error code: ");  
      //nss.println(xbee.getResponse().getErrorCode());
      // or flash error led
    } 
    else 
    {
      // local XBee did not provide a timely TX Status Response.  Radio is not configured properly or connected
      Serial.println("retry sending");
    }
    
    //delay(5000);
}
