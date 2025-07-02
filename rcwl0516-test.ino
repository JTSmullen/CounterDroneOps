#define RCWL_IN 13

int motionState = LOW;

void setup() 
{
  pinMode(RCWL_IN, INPUT);
  Serial.begin(115200);
}

void loop() 
{
  if (digitalRead(RCWL_IN) == HIGH)
  {
    Serial.println("Motion detected.");
    motionState = HIGH;
  }
  else if (digitalRead(RCWL_IN) == LOW && motionState == HIGH)
  {
    Serial.println("Motion stopped.");
    motionState = LOW;
  }
  else
  Serial.println("No motion detected.");

  delay(250);

}
