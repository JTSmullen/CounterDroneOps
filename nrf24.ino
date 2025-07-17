/*
Designed to detect presence of packets on a set public frequency.

Does not transmit on any frequency.
Packets are NOT stored or analyzed.
*/

#include <SPI.h>
#include <RF24.h>

#define CE_PIN 16
#define CSN_PIN 17
#define SCK_PIN 18
#define MOSI_PIN 23
#define MISO_PIN 19

RF24 radio(CE_PIN, CSN_PIN);
const uint8_t CHANNEL = 76;               //0-125, drones usually use 76

void setup() 
{
  Serial.begin(115200);
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CSN_PIN);

  while (!radio.begin())
  {
    Serial.println("NRF24L01 failed to initialize.\n");
    delay(100);
  }

  //Radio configuration
  radio.setAutoAck(false);                //important to keep disabled
  radio.setDataRate(RF24_2MBPS);          //alternative is 250kb & 1mb
  radio.setChannel(CHANNEL);
  radio.setCRCLength(RF24_CRC_DISABLED);  
  radio.disableDynamicPayloads();
  radio.setPayloadSize(32);

  //Enters RX mode
  radio.openReadingPipe(1, 0xFFFFFFFFEE);
  radio.powerUp();
  radio.startListening();

  delay(1000);
}

void loop() 
{
  //Repeatedly scans a set channel
  if (radio.testCarrier())
    Serial.println("!!! Noise detected.");
  else
    Serial.println("--- No noise detected.");

  delay(500);
}

/*
Scans for noisy channels if you don't know the channel the RF is on.

Add scan() to setup() to scan once.
Add scan() to loop() to repeat (will need to comment out current code in loop()).
*/
void scan()
{
  for (int i=0; i<3; i++)
  {
    String string;
    switch (i)
    {
      case 0:
        radio.setDataRate(RF24_2MBPS);
        string = "2MBPS: ";
        break;
      case 1:
        radio.setDataRate(RF24_250KBPS);
        string = "250KBPS: ";
        break;
      case 2:
        radio.setDataRate(RF24_1MBPS);
        string = "1MBPS: ";
        break;
    }

    for (uint8_t j=0; j<=83; j++)
    {
      radio.setChannel(j);

      if (radio.testCarrier())
      {
        Serial.print(string);
        Serial.print(j);
        Serial.println(". !!! Noise detected.");
      }
      else
      {
        Serial.print(string);
        Serial.print(j);
        Serial.println(". --- No noise detected.");     
      }
      delay(250);
    }
  }
}
