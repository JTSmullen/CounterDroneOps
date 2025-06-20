//requires CP210X Windows Driver from Silicon Labs if not already installed (download, extract, use 64 installer)

#include <DFRobot_C4001.h>

#define RX_PIN 19
#define TX_PIN 18

DFRobot_C4001_UART radar(&Serial1, 9600, RX_PIN, TX_PIN);

void setup()
{
  Serial.begin(115200);
  while (!Serial)
  {
    delay(10);
  }

  while (!radar.begin())
  {
    Serial.println("C4001 not detected!");
    delay(100);
  }
  Serial.println("C4001 connected!");

  radar.setSensorMode(eExitMode);

  sSensorStatus_t status = radar.getStatus();
  Serial.print("Work status: ");
  Serial.println(status.workStatus);
  Serial.print("Work mode: ");
  Serial.println(status.workMode);
  Serial.print("Init status: ");
  Serial.println(status.initStatus);
}

void loop()
{
  if (radar.motionDetection())
  {
//    sSensorStatus_t data = radar.getStatus(); //have to fix this section later
    Serial.print("Motion detected!\n");
    /*Serial.print("Motion: ");   
    Serial.print(data.motion);
    Serial.print(" | Presence: ");
    Serial.print(data.presence);
    Serial.print(" | Range: ");
    Serial.print(data.range);
    Serial.print(" | Speed: ");
    Serial.println(data.speed);*/
  }
  delay(500);
}