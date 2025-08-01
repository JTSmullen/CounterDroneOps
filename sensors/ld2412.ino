/*
This will detect the RC drones/helicopters as well as humans.

Cross reference with a sensor that can only detect humans to accurately detect
when a presence is a drone vs. when it is a human.
*/

#include "LD2412.h"

#define BAUD_RATE 115200
#define LD2412_RX 26    //ESP<-LD
#define LD2412_TX 27    //ESP->LD

LD2412 LD(Serial2);

void setup() 
{
  Serial2.begin(BAUD_RATE, SERIAL_8N1, LD2412_RX, LD2412_TX);
  Serial.begin(115200);
}

void loop() 
{ 
  if (LD.targetState() != 0) 
  {
    Serial.print("Target Type: ");
    Serial.printf("%d", LD.targetState());
    Serial.print(" | M. Range: ");
    Serial.printf("%d", LD.movingDistance());
    Serial.print(" | S. Range: ");
    Serial.printf("%d", LD.staticDistance());
    Serial.println();
  }
  else 
    Serial.println("No motion detected.");

  delay(1000);   
}
