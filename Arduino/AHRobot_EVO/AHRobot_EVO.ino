// JJROBOTS AHR: AIR HOCKEY ROBOT EVO PROJECT (Smartphone control ready!)
// Author: Jose Julio (JJROBOTS)
// Hardware: Arduino Leonardo + JJROBOTS brain shield v3 (devia)
// Date: 04/11/2015
// Last updated: 14/12/2016
// Version: 2.17
// Project page :
//   http://jjrobotos.com/air-hockey-robot-evo/
// GIT repository:
//   http://github.com/jjrobots/Air_Hockey_Robot_EVO/
// License: Open Software GPL License

#define VERSION "2.17"

// ROBOT and USER configuration parameters
#include "Configuration.h"
#include "Definitions.h"   // Variable definitions
#include <Pixy2.h>         // Requires Pixy2 Arduino Library: https://github.com/charmedlabs/pixy2/raw/master/releases/arduino/arduino_pixy2-1.0.3.zip

Pixy2 pixy;                // This is the main Pixy object 

const byte numChars = 32;
char receivedChars[numChars]; // an array to store the received data
boolean newData = false;

void setup()
{
  // STEPPER PINS ON JJROBOTS BRAIN SHIELD
  pinMode(4, OUTPUT); // ENABLE MOTORS
  pinMode(7, OUTPUT); // STEP MOTOR 1 PORTE,6
  pinMode(8, OUTPUT); // DIR MOTOR 1  PORTB,4
  pinMode(12, OUTPUT); // STEP MOTOR 2 PORTD,6
  pinMode(5, OUTPUT); // DIR MOTOR 2  PORTC,6
  digitalWrite(4, HIGH);  // Disbale stepper motors

  // Disabling servos to allow pins to be used for ISP - Nick
  // pinMode(10, OUTPUT);  // Servo1 (arm)
  // pinMode(13, OUTPUT);  // Servo2

  pinMode(A4, OUTPUT);  // Microstepping selector
  digitalWrite(A4, LOW); // 1/8 Microstepping

  pinMode(A3,INPUT);   // User puch button (pushed=>gnd)
  digitalWrite(A3,HIGH); // Enable pullup

  Serial.begin(115200);   // PC debug connection
  //Serial1.begin(115200);  // ESP serial connection
  delay(5000);
  Serial.print("AHR JJRobots Air Hockey Robot EVO version ");
  Serial.println(VERSION);
  delay(1000);

  //LED blink
  // for (uint8_t k = 0; k < 5; k++)
  // {
  //   digitalWrite(13, HIGH);
  //   delay(300);
  //   digitalWrite(13, LOW);
  //   delay(300);
  // }

  //Serial.println("Initializing Wifi module...");
  // ESP Wifi module initialization routine.
  // The Robot will generate it´s own wifi network JJROBOTS_xx and listen external UDP messages...
  //ESPInit();

  // Serial.println("Initializing Pixy Camera");
  // // This uses the Arduino's ICSP SPI port.
  // pixy.init();


  Serial.println("Initializing Stepper motors...");
  // STEPPER MOTORS INITIALIZATION
  // We use TIMER 1 for stepper motor 1 and Timer 3 for motor 2
  // TIMER1 CTC MODE
  TCCR1B &= ~(1 << WGM13);
  TCCR1B |=  (1 << WGM12);
  TCCR1A &= ~(1 << WGM11);
  TCCR1A &= ~(1 << WGM10);

  // output mode = 00 (disconnected)
  TCCR1A &= ~(3 << COM1A0);
  TCCR1A &= ~(3 << COM1B0);

  // Set the timer pre-scaler
  // Generally we use a divider of 8, resulting in a 2MHz timer on 16MHz CPU
  TCCR1B = (TCCR1B & ~(0x07 << CS10)) | (2 << CS10);

  OCR1A = ZERO_SPEED;   // Motor stopped
  dir_M1 = 0;
  TCNT1 = 0;

  // We use TIMER 3 for stepper motor 2
  // STEPPER MOTORS INITIALIZATION
  // TIMER3 CTC MODE
  TCCR3B &= ~(1 << WGM13);
  TCCR3B |=  (1 << WGM12);
  TCCR3A &= ~(1 << WGM11);
  TCCR3A &= ~(1 << WGM10);

  // output mode = 00 (disconnected)
  TCCR3A &= ~(3 << COM1A0);
  TCCR3A &= ~(3 << COM1B0);

  // Set the timer pre-scaler
  // Generally we use a divider of 8, resulting in a 2MHz timer on 16MHz CPU
  TCCR3B = (TCCR3B & ~(0x07 << CS10)) | (2 << CS10);

  OCR3A = ZERO_SPEED;   // Motor stopped
  dir_M2 = 0;
  TCNT3 = 0;

  delay(1000);
  TIMSK1 |= (1 << OCIE1A); // Enable Timer1 interrupt
  TIMSK3 |= (1 << OCIE1A); // Enable Timer1 interrupt

  // Enable steppers
  digitalWrite(4, LOW);

  delay(1000);

  //Initializing Robot initial position and parameters...
  position_M1 = (ROBOT_INITIAL_POSITION_X + ROBOT_INITIAL_POSITION_Y) * X_AXIS_STEPS_PER_UNIT;
  position_M2 = (ROBOT_INITIAL_POSITION_X - ROBOT_INITIAL_POSITION_Y) * Y_AXIS_STEPS_PER_UNIT;
  target_position_M1 = position_M1;
  target_position_M2 = position_M2;

  user_max_speed = MAX_SPEED;
  user_max_accel = MAX_ACCEL;
  max_acceleration = user_max_accel;
  max_speed = user_max_speed / 2;
  user_robot_defense_position = ROBOT_DEFENSE_POSITION_DEFAULT;
  user_robot_defense_attack_position = ROBOT_DEFENSE_ATTACK_POSITION_DEFAULT;
  defense_position = ROBOT_DEFENSE_POSITION_DEFAULT;   // Robot y axis defense position
  attack_position = ROBOT_DEFENSE_ATTACK_POSITION_DEFAULT;   // Robot y axis position for defense+attack

  // Output parameters to console
  Serial.print("Max_acceleration: ");
  Serial.println(user_max_accel);
  Serial.print("Max speed: ");
  Serial.println(user_max_speed);
  Serial.println("Moving to initial position...");
  Serial.println("Ready!!");

  setPosition_straight(ROBOT_CENTER_X, ROBOT_DEFENSE_POSITION_DEFAULT);

  Serial.print("Target_position:");
  Serial.print(ROBOT_CENTER_X);
  Serial.print(",");
  Serial.println(ROBOT_DEFENSE_POSITION_DEFAULT);
  //Serial.print(";");
  //Serial.print(target_position_M1);
  //Serial.print(",");
  //Serial.println(target_position_M2);

  digitalWrite(13, HIGH);
  Serial.print("AHR JJRobots Air Hockey Robot EVO version ");
  Serial.println(VERSION);
  Serial.println("ready...");

  testmode=false;
  robot_status = 0;
  timer_old = micros();
  timer_packet_old = timer_old;
  micros_old = timer_old;
}

// Main loop
void loop()
{
  int i;
  int dt;
  uint8_t logOutput = 0;

  debug_counter++;

  if (analogRead(A3)<100) // pushbutton pushed => TEST PATTERN
  {
    loop_counter=0;
    testmode=true;
  }
  
  timer_value = micros();
  if ((timer_value - timer_old) >= 1000) // MAIN 1Khz loop
  {
    timer_old = timer_value;
    loop_counter++;

    receiveSerialData();
    processSerialCommand();

//     //pixy cOE
//     pixy.ccc.getBlocks(false);

//       // If there are detect blocks, print them!
//     if (pixy.ccc.numBlocks)
//     {
// //      Serial.print("Detected ");
// //      Serial.println(pixy.ccc.numBlocks);
//       for (i=0; i<pixy.ccc.numBlocks; i++)
//       {
// //        Serial.print("  block ");
// //        Serial.print(i);
// //        Serial.print(": ");
// //        pixy.ccc.blocks[i].print();
        
//         if (pixy.ccc.blocks[i].m_signature == 1) // Puck Detected
//         {
//           puckOldCoordX = puckCoordX;
//           puckOldCoordY = puckCoordY;
//           puckCoordX = pixy.ccc.blocks[i].m_x;
//           puckCoordY = pixy.ccc.blocks[i].m_y;
//           Serial.println("");
//           Serial.print("PixyPuckX");Serial.println(puckCoordX);
//           Serial.print("PixyPuckY");Serial.println(puckCoordY);

//           // map(value, fromLow, fromHigh, toLow, toHigh)
//           puckCoordX = map(puckCoordX, 70, 220, 0, TABLE_WIDTH); // Map Pixy X Value range 7-200 to Table Width
//           puckCoordY = map(200 - puckCoordY, 0, 134, 0, TABLE_LENGTH / 2 - 100);
//           Serial.print("TablePuckX");Serial.println(puckCoordX);
//           Serial.print("TablePuckY");Serial.println(puckCoordY);

          

//         } else if (pixy.ccc.blocks[i].m_signature == 2) // Robot Detected
//         {
//           robotCoordX = pixy.ccc.blocks[i].m_x;
//           robotCoordY = pixy.ccc.blocks[i].m_y;
//           Serial.println("");
//           Serial.print("PixyRobotX");Serial.println(robotCoordX);
//           Serial.print("PixyRobotY");Serial.println(robotCoordY);          

//           // map(value, fromLow, fromHigh, toLow, toHigh)
//           robotCoordX = map(robotCoordX, 70, 220, 0, TABLE_WIDTH); // Map Pixy X Value range 7-200 to Table Width
//           robotCoordY = map(200 - robotCoordY, 0, 134, 0, TABLE_LENGTH / 2 - 100);
//           Serial.print("TableRobotX");Serial.println(robotCoordX);
//           Serial.print("TableRobotY");Serial.println(robotCoordY);          

//         }

//       }

//         if ((puckCoordX > TABLE_WIDTH) || (puckCoordY > TABLE_LENGTH) || (robotCoordX > TABLE_WIDTH) || (robotCoordY > ROBOT_CENTER_Y)) {
//           Serial.print("P ERR99!");
//           Serial.print(puckCoordX);
//           Serial.print(",");
//           Serial.print(puckCoordY);
//           Serial.print(";");
//           Serial.print(robotCoordX);
//           Serial.print(",");
//           Serial.println(robotCoordY);
//           robot_status = 0;
//         }

//       dt = (timer_value - timer_packet_old) / 1000.0;
//        //Serial.println(dt);
//        //dt = 16;  //60 Hz = 16.66ms
//       timer_packet_old = timer_value;
//       cameraProcess(dt);
//       // Strategy based on puck prediction
//       newDataStrategy();
//     }
    
//       robotStrategy();

//       // Robot position detection for missing steps detection in stepper motors.
//       missingStepsDetection();

    // packetRead();  // Check for new packets...
    // if (newPacket > 0)
    // {
    //   dt = (timer_value - timer_packet_old) / 1000.0;
    //   //Serial.println(dt);
    //   //dt = 16;  //60 Hz = 16.66ms
    //   timer_packet_old = timer_value;
    //   logOutput = 0;

    //   Serial.print("P");
    //   Serial.print(newPacket);

    //   if (newPacket == 1) // Camera packet
    //   {
    //     // Puck detection and trayectory prediction
    //     cameraProcess(dt);
    //     // Strategy based on puck prediction
    //     newDataStrategy();
    //     // Serial output (DEBUG)
    //     //Serial.print(" ");
    //     //Serial.print(cam_timestamp);
    //     //Serial.print(" ");
    //     //Serial.print(puckCoordX);
    //     //Serial.print(",");
    //     //Serial.print(puckCoordY);
    //     //Serial.print(" ");
    //     //Serial.print(predict_status);
    //     //Serial.print(",");
    //     //Serial.print(robot_status);
    //   }
    //   else if (newPacket == 2) { // User sends a manual move
    //     robot_status = 5;    // Manual mode
    //     Serial.print(" ");
    //     Serial.print(user_target_x);
    //     Serial.print(",");
    //     Serial.print(user_target_y);
    //   }
    //   else if (newPacket == 3) {
    //     Serial.print(" USER MAX SPEED:");
    //     Serial.println(user_max_speed);
    //     Serial.print("USER MAX ACCEL:");
    //     Serial.println(user_max_accel);
    //     Serial.print("USER DEFENSE POSITION:");
    //     Serial.println(user_robot_defense_position);
    //     Serial.print("USER ATTACK POSITION:");
    //     Serial.print(user_robot_defense_attack_position);
    //   }

      robotStrategy();

    //   // Robot position detection for missing steps detection in stepper motors.
    //   missingStepsDetection();

    //   Serial.println();
    //   newPacket = 0;
    // }  // End packet received

//     if (testmode)
//       testMovements();

// #ifdef DEBUG
//     // DEBUG: We inform of the position error of the robot as seen in the camera (util for calibrations)
//     if ((loop_counter % 1293) == 0) {
//       Serial.print("ROBOT MSD: ");
//       Serial.print(robotMissingStepsErrorX);
//       Serial.print(",");
//       Serial.println(robotMissingStepsErrorY);
//     }
// #endif

    if ((loop_counter % 10) == 0)
      updatePosition_straight();  // update straight line motion algorithm

      // if ((loop_counter % 1000) == 0) {
      //   Serial.print("real_position_x: ");Serial.println(real_position_x);
      //   Serial.print("real_position_y: ");Serial.println(real_position_y);
      // }

//     // Position, speed and acceleration control:
     positionControl();
  } // 1Khz loop
}

void receiveSerialData() {
    // https://forum.arduino.cc/index.php?topic=288234.0
    static byte ndx = 0;
    char endMarker = '\r';
    char rc;

    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        Serial.print(rc); // echo receieved data

        if (rc != endMarker) {
            receivedChars[ndx] = rc;
            ndx++;
            if (ndx >= numChars) {
                ndx = numChars - 1;
            }
        }
        else {
            receivedChars[ndx] = '\0'; // terminate the string
            ndx = 0;
            newData = true;
        }
    }
}

void processSerialCommand() {
    if (newData == true) {
        String command(receivedChars);
        command.trim();
        Serial.print("Processing: ");
        Serial.print(command);
        Serial.println();

        if (command.startsWith("G0")) {
            int x = getValue(command, "X", " ");
            int y = getValue(command, "Y", " ");
            Serial.print("Moving to position [");
            Serial.print(x);Serial.print(",");Serial.print(y);
            Serial.println("]");
            setPosition_straight(x,y);
        }
        else if (command.startsWith("G1")) {
          setPosition_straight(ROBOT_CENTER_X, ROBOT_DEFENSE_POSITION_DEFAULT + 20);
        }
        else if (command.startsWith("X")) { 
            Serial.println("STOPPING!");
        }
        else {
            Serial.println("Command not supported");
        } 

        Serial.println("Done!");
        Serial.println();
        Serial.print("> ");
        newData = false;
    }
}

int getValue(String command, String argument, String delimiter) {
    
    int argIndex = command.indexOf(argument);
    int delimiterIndex = command.indexOf(delimiter, argIndex);
    int argLen = argument.length();
    
    return command.substring(argIndex + argLen, delimiterIndex).toInt();

}