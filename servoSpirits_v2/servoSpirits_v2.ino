
#include <Servo.h>

Servo myservo;

const byte numSpirits = 5; //starts in port 9, it goes until port 13

Servo spiritPort_9;
Servo spiritPort_10;
Servo spiritPort_11;
Servo spiritPort_12;
Servo spiritPort_13;

int resetPin = 2;

byte posSpirit[numSpirits];
byte actSpirit[numSpirits];
byte whichSpirit[numSpirits];

  // create servo object to control a servo
// twelve servo objects can be created on most boards

byte pos = 0;    // variable to store the servo position
unsigned int longDelay = 1000;
unsigned int shortDelay = 90;

void setup() {

  //Serial.begin(9600);
 
  switch(numSpirits){
    case 1:
      spiritPort_9.attach(9);
      break;
    case 2:
      spiritPort_9.attach(9);
      spiritPort_10.attach(10);
      break;
    case 3:
      spiritPort_9.attach(9);
      spiritPort_10.attach(10);
      spiritPort_11.attach(11);
      break;
    case 4:
      spiritPort_9.attach(9);
      spiritPort_10.attach(10);
      spiritPort_11.attach(11);
      spiritPort_12.attach(12);
      break;
    case 5:
    
      spiritPort_9.attach(9);
      spiritPort_10.attach(10);
      spiritPort_11.attach(11);
      spiritPort_12.attach(12);
      spiritPort_13.attach(13);
      break;
    default:
      break;
    }
      //myservo.attach(13);  // attaches the servo on pin 9 to the servo object
  
  digitalWrite(resetPin, HIGH);
  delay(200);
  pinMode(resetPin, OUTPUT);
}

byte countCyclesToReset = 0;

void loop() {
  setupParameters();
  executeActions();
  countCyclesToReset++;
  if (countCyclesToReset >= 10){
    digitalWrite(resetPin, HIGH);
    digitalWrite(resetPin, LOW);
    
    countCyclesToReset = 0;
  }
}

// MAIN ACTION FUNCTIONS

void setupParameters(){
  for (byte iSpirit = 0; iSpirit <= numSpirits; iSpirit++){
    //byte whichRandomNumber = randomNumber(1,4);
    actSpirit[iSpirit] = randomNumber(1,4);
    whichSpirit[iSpirit] = randomNumber(1,6);
  }
}

void executeActions(){
  for (byte iSpiritSeq = 0; iSpiritSeq <= numSpirits; iSpiritSeq++){

    byte whichCase = actSpirit[iSpiritSeq];
    byte iSpirit = whichSpirit[iSpiritSeq];
    
    if(whichCase == 1){
        rattle(iSpirit);
    }
    else if(whichCase == 2){
        turn(iSpirit);
    }

    else if(whichCase == 3){
      unsigned int waitRandomTime = randomNumber(500, 5000);
      delay(waitRandomTime);
    }
  }
}

// BASIC FUNCTIONS

void turn(byte spiritNumber){

  byte initialPosition = positionSpirit(spiritNumber); //posSpirit[spiritNumber];
  byte randomAngle = randomNumber(30, 60);
  byte randomSpeed = randomNumber(1, 100);

  if (initialPosition - randomAngle < 0){
  
    for (byte iMove = initialPosition; 
              iMove <= initialPosition + randomAngle; 
              iMove++){
      //posSpirit[spiritNumber] = iMove;
      moveSpirit(spiritNumber, iMove);
      delay(randomSpeed);
    }
  }
  else if (initialPosition + randomAngle > 180){
    
    for (byte iMove = initialPosition; 
              iMove >= initialPosition - randomAngle; 
              iMove--){
      //posSpirit[spiritNumber] = iMove;
      moveSpirit(spiritNumber, iMove);
      delay(randomSpeed);
    }
   }
  else {
    
    byte upORdown = random(0,2);
    if (upORdown == 1){
      //Serial.print(upORdown);
      for (byte iMove = initialPosition; 
                iMove <= initialPosition + randomAngle; 
                iMove++){
        //posSpirit[spiritNumber] = iMove;
        moveSpirit(spiritNumber, iMove);
        delay(randomSpeed);
      }
    }
  
    else {
      //Serial.print(upORdown);
      for (byte iMove = initialPosition; 
                iMove >= initialPosition - randomAngle; 
                iMove--){
        //posSpirit[spiritNumber] = iMove;
        moveSpirit(spiritNumber, iMove);
        delay(randomSpeed);
    }
   }

  }
  
}

void rattle(byte spiritNumber){

  byte initialPosition = positionSpirit(spiritNumber);
  byte rattleUp = 1;
  
  for (byte iRattle = 0; iRattle <= 20; iRattle++){
    if (rattleUp == 1){
      moveSpirit(spiritNumber, initialPosition + 9);
      delay(30);
      rattleUp = 0;
    }
    if (rattleUp == 0) {
      moveSpirit(spiritNumber, initialPosition - 9);
      delay(30);
      rattleUp = 1;
    }
  }
}

byte randomNumber(byte minValue, byte maxValue){
  // randomSeed(analogRead(0));
  byte randomValue = random(minValue, maxValue);
  return randomValue;
}

void moveSpirit(byte spiritNumber, byte moveAngle){
  
  
  if (spiritNumber == 0 ){
      spiritPort_9.write(moveAngle);}
  else if (spiritNumber == 1 ){
      spiritPort_10.write(moveAngle);}
  else if (spiritNumber == 2 ){
      spiritPort_11.write(moveAngle);}
  else if (spiritNumber == 3 ){
      spiritPort_12.write(moveAngle);}
  else if (spiritNumber == 4 ){
      spiritPort_13.write(moveAngle);}

    if (posSpirit[spiritNumber] < 0){
      posSpirit[spiritNumber] = 0;
    }
    else if (posSpirit[spiritNumber] > 180) {
      posSpirit[spiritNumber] = 180;
    }
}

byte positionSpirit(byte spiritNumber){
  
  
  if (spiritNumber == 0 ){
      return spiritPort_9.read();}
  else if (spiritNumber == 1 ){
      return spiritPort_10.read();}
  else if (spiritNumber == 2 ){
      return spiritPort_11.read();}
  else if (spiritNumber == 3 ){
      return spiritPort_12.read();}
  else if (spiritNumber == 4 ){
      return spiritPort_13.read();}

}
