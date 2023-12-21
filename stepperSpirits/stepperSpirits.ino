//group AB
#define in_A_1 2
#define in_A_2 3
#define in_B_1 4
#define in_B_2 5

//group CD
#define in_C_1 6
#define in_C_2 7
#define in_D_1 8
#define in_D_2 9

//group EF
#define in_E_1 10
#define in_E_2 11
#define in_F_1 12
#define in_F_2 13

int currentStep = 0;

unsigned int time_delay =100;

void setup()
{
    Serial.begin(9600);
  
    pinMode(in_A_1, OUTPUT);
    pinMode(in_A_2, OUTPUT);
    pinMode(in_B_1, OUTPUT);
    pinMode(in_B_2, OUTPUT);

    pinMode(in_C_1, OUTPUT);
    pinMode(in_C_2, OUTPUT);
    pinMode(in_D_1, OUTPUT);
    pinMode(in_D_2, OUTPUT);

    pinMode(in_E_1, OUTPUT);
    pinMode(in_E_2, OUTPUT);
    pinMode(in_F_1, OUTPUT);
    pinMode(in_F_2, OUTPUT);
}

void stepGeneral(char group, unsigned int nStep, unsigned int delayStep) {
    int subGroup1, subGroup2, subGroup3, subGroup4;

    switch (group) {
        case 'A':
            subGroup1 = in_A_1;
            subGroup2 = in_A_2;
            subGroup3 = in_B_1;
            subGroup4 = in_B_2;
            break;
        case 'C':
            subGroup1 = in_C_1;
            subGroup2 = in_C_2;
            subGroup3 = in_D_1;
            subGroup4 = in_D_2;
            break;
        case 'E':
            subGroup1 = in_E_1;
            subGroup2 = in_E_2;
            subGroup3 = in_F_1;
            subGroup4 = in_F_2;
            break;
        default:
            return;
    }

    if (nStep == 1) {
        digitalWrite(subGroup1, HIGH);
        digitalWrite(subGroup2, LOW);
        digitalWrite(subGroup3, HIGH);
        digitalWrite(subGroup4, LOW);
    }

    if (nStep == 2) {
        digitalWrite(subGroup1, HIGH);
        digitalWrite(subGroup2, LOW);
        digitalWrite(subGroup3, LOW);
        digitalWrite(subGroup4, HIGH);
    }

    if (nStep == 3) {
        digitalWrite(subGroup1, LOW);
        digitalWrite(subGroup2, HIGH);
        digitalWrite(subGroup3, LOW);
        digitalWrite(subGroup4, HIGH);
    }
    
    if (nStep == 4) {
        digitalWrite(subGroup1, LOW);
        digitalWrite(subGroup2, HIGH);
        digitalWrite(subGroup3, HIGH);
        digitalWrite(subGroup4, LOW);
    }
    delay(delayStep);
}


void turnMotor(char group, unsigned int speedTurn, 
    unsigned int numTurns, bool clockWise){

if (clockWise){
  
  unsigned int iterStepper = 1;
  for (int i = 0; i <= numTurns; i++){
    stepGeneral(group, iterStepper, speedTurn);
    iterStepper++;
    if (iterStepper > 4){
      iterStepper = 1;
      }
  }
}
  else {
    
    unsigned int iterStepper = 4;
    for (int i = 0; i <= numTurns; i++){
      stepGeneral(group, iterStepper, speedTurn);
      iterStepper--;
      if (iterStepper < 1){
        iterStepper = 4;
        }    
  }
  }

    
}

void step1(){
    //A+,B+
    digitalWrite(in_A_1, 1);
    digitalWrite(in_A_2, 0);
    digitalWrite(in_B_1, 1);
    digitalWrite(in_B_2, 0);
    delay(time_delay);
}

void step2(){
    //A+,B-
    digitalWrite(in_A_1, 1);
    digitalWrite(in_A_2, 0);
    digitalWrite(in_B_1, 0);
    digitalWrite(in_B_2, 1);
    delay(time_delay);
}

void step3(){
    //A-,B-
    digitalWrite(in_A_1, 0);
    digitalWrite(in_A_2, 1);
    digitalWrite(in_B_1, 0);
    digitalWrite(in_B_2, 1);
    delay(time_delay);
}

void step4(){
    //A-,B+
    digitalWrite(in_A_1, 0);
    digitalWrite(in_A_2, 1);
    digitalWrite(in_B_1, 1);
    digitalWrite(in_B_2, 0);
    delay(time_delay);
}

void tickClock(){
  currentStep++;
  if( currentStep >= 4 ){
    currentStep = 0;
  }
  int cycle = currentStep % 4;
  if( cycle == 0 ){
    step1();
  } else if( cycle == 1 ){
    step2();
  } else if( cycle == 2 ){
    step3();
  } else if( cycle == 3 ){
    step4();
  }
}

void counter_tickClock(){
 
  step4();
  step3();
  step2();
  step1();
}

void clockwise(long st){

  step1();
  step2();
  step3();
  step4();
}

void counter_clockwise(long st){
  long i = 0;
  while( i < st ){
    //step1
    step1();
    //step4
    step4();
    //step3
    step3();
    //step2
    step2();
    i++;
  }
}
void loop()
{
    //clockwise(18);//counter_clockwise(18);
   // tickClock();
   // counter_tickClock();

   
   // for 1 turn
   // numTurns = 
   // speedTurn = 50
  char group = 'E';
  bool clockWise = false;
  int numTurns = 5; 
  int speedTurn = 50; // the bigger the slower
  //for (int i = 0; i <= numTurns; i++){
    turnMotor(group, speedTurn, numTurns, clockWise);
  //}

  group = 'C';
  clockWise = false;
  numTurns = 4; 
  speedTurn = 30; // the bigger the slower
  turnMotor(group, speedTurn, numTurns, clockWise);
  group = 'E';
  
  delay(400);

  clockWise = true;
  numTurns = 3; 
  speedTurn = 90; // the bigger the slower
  turnMotor(group, speedTurn, numTurns, clockWise);

  group = 'C';
  clockWise = false;
  numTurns = 7; 
  speedTurn = 10; // the bigger the slower
  turnMotor(group, speedTurn, numTurns, clockWise);
  group = 'E';
  
  delay(800);


  clockWise = false;
  numTurns = 6; 
  speedTurn = 20; // the bigger the slower
  turnMotor(group, speedTurn, numTurns, clockWise);
    
  group = 'C';
  clockWise = true;
  numTurns = 2; 
  speedTurn = 80; // the bigger the slower
  turnMotor(group, speedTurn, numTurns, clockWise);
  group = 'E';
  delay(300);

  
  clockWise = true;
  numTurns = 3; 
  speedTurn = 90; // the bigger the slower
  turnMotor(group, speedTurn, numTurns, clockWise);

  group = 'C';
  clockWise = false;
  numTurns = 4; 
  speedTurn = 20; // the bigger the slower
  turnMotor(group, speedTurn, numTurns, clockWise);
  group = 'E';
  delay(800);
}
