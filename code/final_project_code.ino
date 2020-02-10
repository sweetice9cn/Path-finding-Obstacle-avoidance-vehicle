#include <ESP8266WiFi.h>

// for motor
#define Ain1 14
#define Ain2 12
#define Bin1 13
#define Bin2 4

// for two ultrasonic sensors
#define trigPin 5
#define echoPinFront 15
#define echoPinRight 16

// wifi AP point information
const char* ssid     = "ESPsoftAP_01";
const char* password = "EBD_project";

// for two ultrasonic sensors, one in the front, another one for the right
long durationFront;
long durationRight;
int distanceFront;
int distanceRight;
int tempDistance = 0;

// flag for determining current state
int obstacleAvoidance = 1;
int hasObstableFront = 0;
int hasObstableRight = 0;
int checkLeft = 0;
int checkRight = 0;

// variables for beacon navigation
int rotateForBeacon = 0;
int avgRSSI = 0;
int tempRSSI = 0;

void setup() {
  Serial.begin(9600);
  delay(100);
  connectWifi(); // We start by connecting to a WiFi network
  delay(100);
  setupMotor(); // setup motor pins
  delay(100);
  setupUltrasonic(); // setup ultrasonic sensors pins
  // initialize flags
  hasObstableFront = 0;
  hasObstableRight = 0;
  obstacleAvoidance = 1;
  checkLeft = 0;
  checkRight = 0;
  distanceFront = 100;
  distanceRight = 100;
}

void connectWifi() {
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  checkWifiStatus();
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setupMotor() {
  pinMode(Ain1, OUTPUT);  //Ain1
  pinMode(Ain2, OUTPUT);  //Ain2
  pinMode(Bin1, OUTPUT);  //Bin1
  pinMode(Bin2, OUTPUT);  //Bin2
}

void setupUltrasonic() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPinFront, INPUT);
  pinMode(echoPinRight, INPUT);
}

void loop() {
  checkWifiStatus(); // recheck if wifi is connected before doing anything
  // Either in the initial stage or right after finishing object avoidance,
  // the obstacleAvoidance flag would be 1
  if(obstacleAvoidance == 1) {
    hasObstableFront = 0;
    hasObstableRight = 0;
    findBeacon();
  }
  //After finishing finding the beacon direction and there is no obstacle in the front
  if (obstacleAvoidance == 0 && hasObstableFront == 0){
    if(calAvgRSSI(50) >= -45) { //If condition met, stop the car.
      stopVehicle();
    }
    initialCheckFront(); // check if there is obstacle in the front, move forward if route is clear
  }
  //If there is obstacle in the front, enter obstacle avoidance mode.
  if (obstacleAvoidance == 0 && hasObstableFront == 1) {
    avoidObstacle(); // avoid the obstacle
    // set the correspoding flags indicating the end of obstacle avoidance
    obstacleAvoidance = 1;
    hasObstableFront = 0;
    hasObstableRight = 0;
  }
  delay(200);
}

void findBeacon() {
  findDirectionCoarse(); // find the coarse direction of the beacon (4 directions, turn 90 degrees each)
  obstacleAvoidance = 0; // set the flag to 0
  delay(500);
  findDirectionFine(); // find the fine-grained direction for the beacon and turn the that direction
}

void stopVehicle() {
  while(1) {
    delay(30000);
  }
}

void initialCheckFront() {
  distanceFront = calAvgDistance(echoPinFront, 5); 
  delay(300);
  if(distanceFront <= 30) { // if there is an obstacle within 30 cm, set the flag to 1
    hasObstableFront = 1;
  } else { // if there is no obstacle in the front, move forward
    moveForward();
    delay(300);
  }
}

void checkWifiStatus() {
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}

void findDirectionCoarse() {
  rotateForBeacon = findDirection();
  delay(1000);
  while (rotateForBeacon > 0) { // turn the car based on rotateForBeacon
    turnRight();
    delay(800);
    rotateForBeacon -= 1;
  }
}

/** 
  * Find the coarse direction of the beacon (4 directions, turn 90 degrees each)
  */
int findDirection() {
  avgRSSI = -100;
  rotateForBeacon = 0;
  for (int i = 0; i < 4; ++i) {
    if (i != 0) {
      turnRight();
    }
    delay(1000);
    tempRSSI = calAvgRSSI(120);
    Serial.print("tempRSSI :");
    Serial.println(tempRSSI);
    delay(100);
    if(tempRSSI > avgRSSI) {
      // if current detected avg RSSI is larger than current max, replace it 
      // and store its ditection in rotateForBeacon
      avgRSSI = tempRSSI;
      rotateForBeacon = i;
    }
  }
  delay(500);
  turnRight();
  delay(500);
  return rotateForBeacon;
}

/**
  * Find the find-grained direction for the beacon, (3 directions, turn around 30 degrees each).
  */
void findDirectionFine() {
  rotateForBeacon = 0;
  avgRSSI = -100;  
  turnLeftUnit(); // start with turning left for around 30 degrees
  delay(500);
  for (int i = -1; i < 2; i++) {
    if (i != -1) {
      turnRightUnit();
    }
    delay(1000);
    tempRSSI = calAvgRSSI(100);
    if(tempRSSI > avgRSSI) {
      avgRSSI = tempRSSI;
      rotateForBeacon = i;
    }
  }
  delay(500);
  turnLeftUnit();
  delay(800);
  if(rotateForBeacon < 0) { // adjust the car direction based on the result
    for(int i = 0; i > rotateForBeacon; i--) {
      delay(500);
      turnLeftUnit();
    }
  } else if(rotateForBeacon > 0) {
    for(int i = 0; i < rotateForBeacon; i++) {
      delay(500);
      turnRightUnit();
    }
  }
  delay(500);
}

/** 
  * Avoid obstavle. 
  */
void avoidObstacle() {
  checkLeft = 0;
  checkRight = 0;
  detectLeftObstacle(); // first turn left and check if there is obstacle on the left
  turnRight(); // return to previous direciton
  delay(1000);
  detectRightObstacle(); // turn right to check if there is obstacle on the right
  turnLeft(); // resume to previous position
  delay(1000);
  if (checkLeft == 1) { // there is obstacle on the left also
    avoidFrontAndLeftObstacle();
  } else { // there is obstacle on the right also
    avoidFrontAndRightObstacle();
  }
  obstacleAvoidanceFinish(); // successful avoid previous obstacle
}

void detectLeftObstacle() {
  turnLeft(); 
  setTrigPin();
  distanceFront = calAvgDistance(echoPinFront, 5);
  delay(1000);
  if(distanceFront <= 30) {
    checkLeft = 1; // if there is obstacle in the front after turning left, set the flag
  }
}

void detectRightObstacle() {
  turnRight(); 
  setTrigPin();
  distanceFront = calAvgDistance(echoPinFront, 5);
  delay(1000);
  if(distanceFront <= 30) {
    checkRight = 1; // if yes, set the flag
  } 
}

/**
  * If there is also obstacle on the left, turn left and move backward
  * using the right sensor the detect if there is obstacle
  * keep moving backward until away from obstacle 
  */
void avoidFrontAndLeftObstacle() {
  turnLeft();
  delay(500);
  setTrigPin();
  distanceRight = calAvgDistance(echoPinRight, 5);
  delay(1000);
  while (distanceRight <= 30) {
    moveBackward();
    delay(100);
    setTrigPin();
    distanceRight = calAvgDistance(echoPinRight, 5);
    delay(1000);
  }
  backwardFastMoving();
}

/** if there is also obstacle on the right, turn left and move forward
  * using the right sensor the detect if there is obstacle
  * keep moving forward until away from obstacle
  */
void avoidFrontAndRightObstacle() { 
  turnLeft();
  delay(1000);
  setTrigPin();
  distanceRight = calAvgDistance(echoPinRight, 5);
  delay(100);
  while (distanceRight <= 30) {
    moveForward();
    delay(100);
    setTrigPin();
    distanceRight = calAvgDistance(echoPinRight, 5);
    delay(1000);
  }
  forwardFastMoving();
}

void forwardFastMoving() {
  delay(500);
  moveForward();
  delay(500);
  moveForward();
  delay(500); 
}

void backwardFastMoving() {
  delay(500);
  moveBackward();
  delay(500);
  moveBackward();
  delay(500);
}

/**
  * After going away from the obstacle, turn right and move forward three steps.
  */
void obstacleAvoidanceFinish() {
  delay(500);
  turnRight();
  delay(500);
  moveForward();
  delay(500);
  moveForward();
  delay(500);
  moveForward();
  delay(500);
}

/**
  * Calculate average RSSI strength, num denotes how many data for calculating an average value.
  */ 
int calAvgRSSI(int num){
  tempRSSI = 0;
  for(int i = 0; i < num; ++i) {
    delay(50);
    tempRSSI += WiFi.RSSI();
  }
  return tempRSSI/num;
}

void setTrigPin() {
  // Clear the trigPin by setting it LOW:
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  // Trigger the sensor by setting the trigPin high for 10 microseconds:
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
}

/**
  * Calculate distance for a certain pin (either front sensor or right sensor).
  */
int calDistance(int echoPin) {
  setTrigPin();
  int duration = pulseIn(echoPin, HIGH);
  return duration * 0.034 / 2; // Calculate the distance
}

/**
  * Calculate average distance for a certain pin, num denotes how many data for calculating an average value.
  */
int calAvgDistance(int echoPin, int num) {
  setTrigPin();
  tempDistance = 0;
  for(int i = 0;i < num; i++) {
    tempDistance = calDistance(echoPin) + tempDistance;
    delay(100);
  }
  return tempDistance/num;
}

/**
  * Move car forward, the delay() is used for controlling moving distance.
  */ 
void moveForward(){
  wheelRollForward(Ain1, Ain2);
  wheelRollForward(Bin1, Bin2);
  delay(200);
  setLow();
}

/**
  * Move car backward, the delay() is used for controlling moving distance.
  */ 
void moveBackward(){
  wheelRollBackward(Ain1, Ain2);
  wheelRollBackward(Bin1, Bin2);
  delay(200);
  setLow();
}

/**
  * Turn car left for around 30 degrees, delay for controlling turning degrees.
  */ 
void turnLeftUnit() {
  wheelRollForward(Ain1, Ain2);
  wheelRollBackward(Bin1, Bin2);
  delay(90);
  setLow();
}

/**
  * Turn car right for around 30 degrees, delay for controlling turning degrees.
  */ 
void turnRightUnit() {
  wheelRollBackward(Ain1, Ain2);
  wheelRollForward(Bin1, Bin2);
  delay(90);
  setLow();
}

/**
  * Turn car left for around 90 degrees, delay for controlling turning degrees.
  */ 
void turnLeft() {
  wheelRollForward(Ain1, Ain2);
  wheelRollBackward(Bin1, Bin2);
  delay(225);
  setLow();
}

/**
  * Turn car left for around 90 degrees, delay for controlling turning degrees.
  */ 
void turnRight() {
  wheelRollBackward(Ain1, Ain2);
  wheelRollForward(Bin1, Bin2);
  delay(225);
  setLow();   
}

void wheelRollForward(int pinOne, int pinTwo) {
  digitalWrite(pinOne, HIGH);
  digitalWrite(pinTwo, LOW);
}

void wheelRollBackward(int pinOne, int pinTwo) {
  digitalWrite(pinOne, LOW);
  digitalWrite(pinTwo, HIGH);
}

void setLow() {
  digitalWrite(Ain1, LOW);
  digitalWrite(Ain2, LOW);
  digitalWrite(Bin1, LOW);
  digitalWrite(Bin2, LOW);
}
