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
  uint8_t buffer[256];
  static uint8_t capturedData[256];
  int b_index = 0;
  int cd_index = 0;
  bool storeData = false;

  //Reads UART data
  while (LD_Serial.available()) 
  {
    buffer[b_index] = LD_Serial.read();
    if (buffer[b_index] == 244)
      storeData = true;
    if (storeData == true)
      capturedData[cd_index++] = buffer[b_index];
    if (buffer[b_index] == 245)
      storeData = false;

   /* Serial.printf("%d ", buffer[b_index]);
    if (buffer[b_index]==245)
    {
      Serial.println();
    }*/
    b_index++;
  }
  /*
  This loop prints the following bits:
  Byte 1: Target status (0 is undetected, 1 is moving, 2 is stationary, 3 is both)
  Bytes 2-3: Move distance (in cm)
  Byte 4: Move energy (basically sensitivity, usually 0 or 100)
  Bytes 5-6: Stationary distance (in cm)
  Byte 7: Stationary energy (basically sensitivity, usually 0 or 100)
  */
  /*for (int i=8; i<15; i++)
    Serial.printf("%d ", capturedData[i]);
  Serial.println();*/

  if (capturedData[8] > 0 && capturedData[8] < 4) 
  {
    Serial.print("Target Type: ");
    Serial.printf("%d", capturedData[8]);
    Serial.print(" | M. Range: ");
    Serial.printf("%d", capturedData[9]);
    Serial.print(" | S. Range: ");
    Serial.printf("%d", capturedData[12]);
    Serial.println();
  }
  else 
    Serial.println("No motion detected.");

  delay(1000);       //displays every 1 second, able to be modified 
}
/**
UART Overall Frame Structure:
Header is 4 bytes, 0xF4-F1 (244-241)
Length of "inner frame data" is 2 bytes
Inner frame data is n bytes (specifies by length bytes)
Footer is 4 bytes, 0xF8-F4 (248-245)

Inner Frame Structure:
Data type is 1 byte, should always be 0x02
Head is 1 byte, should always be 0xAA (170)
Target state is 1 byte, 0 no target, 1 movement, 2 stationary, 3 both
Movement distance is 2 bytes
Exercise target energy is 1 byte
Stationary distance is 2 bytes
Stationary energy is 1 byte
End is 0x85 (55)
Check is 0x00
*/
