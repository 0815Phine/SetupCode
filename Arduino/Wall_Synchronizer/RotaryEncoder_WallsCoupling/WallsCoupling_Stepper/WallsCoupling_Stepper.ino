#include <Tic.h>
#include <SoftwareSerial.h>

SoftwareSerial ticSerial(10, 11); //pin 10 to Driver RX; pin 11 to Driver TX
TicSerial tic(ticSerial);

// Sends a "Reset command timeout" command to the Tic.
void resetCommandTimeout() {
  tic.resetCommandTimeout();
}

// Delays for the specified number of milliseconds while resetting the Tic's command timeout so that its movement does not get interrupted.
void delayWhileResettingCommandTimeout(uint32_t ms) {
  uint32_t start = millis();
  do
  {
    resetCommandTimeout();
  } while ((uint32_t)(millis()-start) <= ms);
}

// Motor and Driver specs:
#define StepsperRevolution 200
#define MicrostepsPerStep 1 //can be adjusted in the pololu interface and has to be changed accordingly

// Rotary encoder setup:
//   Encoder A - pin 2 Black
//   Encoder B - pin 4 White
//   Encoder Z - NC    Orange
//   Encoder VCC - 5V Brown
//   Encoder ground GND Blue (0V common) and Shield
#define encAPin 2
#define encBPin 4
#define nSteps 1024 //number of steps per rotation

// Setup measurements
#define WallWheelCircumference 0.11 //in meters
#define wheelRadius (51*1000) //wheel radius in microns (1mm is 1000 microns)
#define wheelDiameter ((float)wheelRadius*2*PI)
#define DistancePerStep ((float)wheelDiameter/nSteps) 

// Direction, Speed and Time variables
#define FW 1 //code for forward direction rotation
#define BW -1 //code for backwards direction rotation
volatile int Direction = 0;
volatile bool DetectChange = false;
volatile static float TotalDistanceInMM = 0.00;
volatile uint32_t SampleStartTime = 0;
volatile uint32_t SampleStopTime = 0;
volatile uint32_t ElapsedTime = 0;
volatile static float CurrentSpeed = 0.00;
static int previousTargetVelocity = 0;
int targetVelocity = 0;

void MeasureRotations() {
  DetectChange = true;
  if (digitalRead(encAPin) == digitalRead(encBPin)) {
  Direction = BW;
  } else {
  Direction = FW;
  }
  TotalDistanceInMM += DistancePerStep/1000;
  SampleStopTime = micros();
  ElapsedTime = SampleStopTime-SampleStartTime;
  SampleStartTime = SampleStopTime;
  CurrentSpeed = (float)DistancePerStep/ElapsedTime*Direction; //in m/s 
}

int calculateTargetVelocity(float speed) {
  // Calculate Wall-Wheel revolutions per second (based on treadmill speed)
  float wheelRevolutionsPerSecond = speed/WallWheelCircumference;
  // Convert Wall-Wheel revolutions to motor steps per second
  float motorStepsPerSecond = wheelRevolutionsPerSecond*StepsperRevolution;
  // Convert steps to microsteps per second
  float microstepsPerSecond = motorStepsPerSecond*MicrostepsPerStep;
  // Convert to microsteps per 10,000 seconds
  int microstepsPerTenThousendSecond = microstepsPerSecond*10000;
  
  return microstepsPerTenThousendSecond;
}

void SynchWalls() {
  targetVelocity = calculateTargetVelocity(CurrentSpeed);
  if (targetVelocity != previousTargetVelocity) {
    tic.setTargetVelocity(targetVelocity);
    previousTargetVelocity = targetVelocity;
  }
}


void setup()
{
  // Set the baud rate.
  ticSerial.begin(9600);

  pinMode(encAPin, INPUT_PULLUP);
  pinMode(encBPin, INPUT_PULLUP);

  // Give the Tic some time to start up.
  delay(20);
  // Tells the Tic that it is OK to start driving the motor.
  tic.exitSafeStart();

  attachInterrupt(digitalPinToInterrupt(encAPin), MeasureRotations, RISING);
  SampleStartTime = micros();
}

void loop() {
  SynchWalls();
  delayWhileResettingCommandTimeout(500);
}