//requires CP210X Windows Driver from Silicon Labs if not already installed (download, extract, use 64 installer)

#include <DFRobot_C4001.h>

#define C4001_RX 19
#define C4001_TX 18

DFRobot_C4001_UART radar(&Serial1, 9600, C4001_RX, C4001_TX);

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

  radar.setSensorMode(eSpeedMode); 

  radar.setDetectThres(60, 1200, 50);       //range an object is considered detected in (third param is sensitivity, less is more sensitive)
  radar.setDetectionRange(60, 1200, 1200);  //range the c4001 scans in (detects all motion from .6m to 12m)
  radar.setTrigSensitivity(5);              //trigger sensitivity 
  radar.setKeepSensitivity(5);              //keep-locked-on sensitivity (this determines when a moving object is no longer detected)

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
  if (radar.getTargetNumber() > 0)
  {
    Serial.print(" | No.: ");
    Serial.print(radar.getTargetNumber());
    Serial.print(" | Range: ");
    Serial.print(radar.getTargetRange());
    Serial.print(" | Speed: ");
    Serial.println(radar.getTargetSpeed());
  }
  else
  {
    Serial.println("No motion detected.");
  }
  delay(1000);       //displays every 1 second, able to be modified 
}