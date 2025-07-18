/*
This will detect the RC drones/helicopters as well as humans.

Cross reference with a sensor that can only detect humans to accurately detect
when a presence is a drone vs. when it is a human.
*/

//#include <HardwareSerial.h>

#define LD2412_RX 26    //ESP<-LD
#define LD2412_TX 27    //ESP->LD

HardwareSerial LD_Serial(2);

void setup() 
{
  Serial.begin(115200);
  LD_Serial.begin(115200, SERIAL_8N1, LD2412_RX, LD2412_TX);

  while (!Serial)
    delay(10);

 /* while (!Serial2) 
  {
    Serial.println("LD2412 not detected!");
    delay(100);
  }
  Serial.println("LD2412 connected!");*/
}

void loop() 
{
  static uint8_t buffer[32];
  static int bufferIndex = 0;

  //Reads UART data
  while (LD_Serial.available()) 
  {
    buffer[bufferIndex] = LD_Serial.read();
    Serial.printf("%d ", buffer[bufferIndex]);
    bufferIndex++;
  }
  Serial.println();

  /*if (radar.presenceDetected()) 
  {
    Serial.print(" | No.: ");
    Serial.print(radar.getTargetNumber());//
    Serial.print(" | Range: ");
    Serial.print(radar.movingTargetDistance()/100);
    Serial.print(" | Speed: ");
    Serial.println(radar.getTargetSpeed());//
  }
  else 
    Serial.println("No motion detected.");*/

  delay(1000);       //displays every 1 second, able to be modified 
}
