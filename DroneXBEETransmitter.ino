#include <XBOXONE.h>

#define DEBUG 0

#define THROTTLE  0
#define ROLL	    1
#define PITCH     2
#define YAW       3
#define AUX1      4
#define AUX2      5
#define AUX3      6
#define AUX4      7

#define CHANNEL1_BREAK   0x50
#define CHANNEL2_BREAK   0x51
#define CHANNEL3_BREAK   0x52
#define CHANNEL4_BREAK   0x53
#define CHANNEL5_BREAK   0x54
#define CHANNEL6_BREAK   0x55
#define CHANNEL7_BREAK   0x56
#define CHANNEL8_BREAK   0x57

#define PWM_LOW 1000
#define PWM_MIDDLE 1500
#define PWM_HIGH 2000

#define STICK_DEADZONE 3275
#define STICK_MIN -32768
#define STICK_MAX 32767

#define TRIGGER_MAX 1023

#define TRIM 0.000

#define TIMEOUT 15

#define SENSITIVITY_CUT 0.50

byte breaks[8] = {CHANNEL1_BREAK, CHANNEL2_BREAK, CHANNEL3_BREAK, CHANNEL4_BREAK, CHANNEL5_BREAK, CHANNEL6_BREAK, CHANNEL7_BREAK, CHANNEL8_BREAK};

uint16_t rc_signals[8] = {0}; 

char flag[4] = {0};

enum Profile { TRIGGER_THROTTLE = 0, GAMING_VEHICLE = 1, GAMING_FPS = 2, DRONE_NORMAL = 3, SIZE = 4 };
int profile = 0;

bool inverted = false;
float sensitivity = 1.0;

unsigned long lastMillis = 0;

char incoming_char = 0; 

USB Usb;
XBOXONE Xbox(&Usb);

void setup() {
  Serial.begin(38400);
  
  while (!Serial);
  
  if (Usb.Init() == -1) {
    if (DEBUG) {
      Serial.print(F("\r\nOSC did not start"));
    }
    while (1); //halt
  }

  if (DEBUG) {
    Serial.print(F("\r\nXBOX USB Library Started"));
  }
}

int getAnalogStick(AnalogHatEnum stick)
{
  if (Xbox.getAnalogHat(stick) < -STICK_DEADZONE) {
    return (map(Xbox.getAnalogHat(stick), STICK_MIN, -STICK_DEADZONE, PWM_LOW + getSensitivityValue(), PWM_MIDDLE));
  } else if (Xbox.getAnalogHat(stick) > STICK_DEADZONE) {
    return (map(Xbox.getAnalogHat(stick), STICK_DEADZONE, STICK_MAX, PWM_MIDDLE, PWM_HIGH - getSensitivityValue()));
  }

  return PWM_MIDDLE;
}

int getInvertedAnalogStick(AnalogHatEnum stick)
{
  if (Xbox.getAnalogHat(stick) < -STICK_DEADZONE) {
    return (map(Xbox.getAnalogHat(stick), STICK_MIN, -STICK_DEADZONE, PWM_MIDDLE, PWM_HIGH - getSensitivityValue()));
  } else if (Xbox.getAnalogHat(stick) > STICK_DEADZONE) {
    return (map(Xbox.getAnalogHat(stick), STICK_DEADZONE, STICK_MAX, PWM_LOW + getSensitivityValue(), PWM_MIDDLE));
  }

  return PWM_MIDDLE;
}

int getAnalogTrigger(ButtonEnum trigger) {
  return (map(Xbox.getButtonPress(trigger), 0, TRIGGER_MAX, PWM_LOW, PWM_HIGH));
}

int getTriggerThrottle() {
  int accel = (getAnalogTrigger(R2) - PWM_LOW);
  int deccel = -(getAnalogTrigger(L2) - PWM_LOW);

  int difference = (PWM_HIGH - PWM_LOW);

  return (map((deccel + accel), -difference, difference, PWM_LOW + getSensitivityValue(), PWM_HIGH - getSensitivityValue()));
}

int getCombinedTriggerThrottle() {
  int first = map(getAnalogTrigger(R2), PWM_LOW, PWM_HIGH, 0, (PWM_MIDDLE - PWM_LOW) - getSensitivityValue());
  int second = map(getAnalogTrigger(L2), PWM_LOW, PWM_HIGH, 0, (PWM_MIDDLE - PWM_LOW) - getSensitivityValue());

  return (PWM_LOW + (first + second));
}

int getSensitivityValue() {
  return ((PWM_MIDDLE - PWM_LOW) * (1.0 - sensitivity));
}

int trimSignal(int signal) {
  return (signal - (signal * TRIM));
}

void loop() {
//  if(Serial.available() >0)
//  {
//    incoming_char=Serial.read();
//    Serial.print(incoming_char);
//  }
  
  Usb.Task();

  if (Xbox.XboxOneConnected) {
    if (millis() > (lastMillis + TIMEOUT)) {
      lastMillis = millis(); 
      if (Xbox.getButtonClick(XBOX)) {
        profile++;
        
        if(profile == SIZE) {
          profile = 0;
          Xbox.doubleShortFastBuzz();
        } else { 
          Xbox.shortFastBuzz();
        }
      }
  
      if (Xbox.getButtonClick(BACK)) {
        inverted = !inverted;
        Xbox.shortBuzz();
      }

      if (Xbox.getButtonClick(START)) {
        if (sensitivity == SENSITIVITY_CUT) {
          sensitivity = 1.0;
        } else {
          sensitivity = SENSITIVITY_CUT;
        }
        Xbox.shortBuzz();
      }
  
      switch (profile) 
      {
        case GAMING_FPS :
          rc_signals[PITCH] = trimSignal(getAnalogStick(LeftHatY));
          rc_signals[ROLL] = trimSignal(getAnalogStick(LeftHatX));
          rc_signals[YAW] = trimSignal(getAnalogStick(RightHatX));
          rc_signals[THROTTLE] = trimSignal(getAnalogStick(RightHatY));
          break;
        case DRONE_NORMAL :
          rc_signals[PITCH] = trimSignal(getAnalogStick(RightHatY));
          rc_signals[ROLL] = trimSignal(getAnalogStick(RightHatX));
          rc_signals[YAW] = trimSignal(getAnalogStick(LeftHatX));
          rc_signals[THROTTLE] = trimSignal(getAnalogStick(LeftHatY));
          break;
        case GAMING_VEHICLE :
          rc_signals[PITCH] = trimSignal(getTriggerThrottle());
          rc_signals[ROLL] = trimSignal(getAnalogStick(LeftHatX));
          rc_signals[YAW] = trimSignal(getAnalogStick(RightHatX));
          if (inverted) {
            rc_signals[THROTTLE] = trimSignal(getInvertedAnalogStick(RightHatY));
          } else {
            rc_signals[THROTTLE] = trimSignal(getAnalogStick(RightHatY));
          }
          break;
        case TRIGGER_THROTTLE :
        default :
          rc_signals[PITCH] = trimSignal(getAnalogStick(LeftHatY));
          rc_signals[ROLL] = trimSignal(getAnalogStick(LeftHatX));
          rc_signals[YAW] = trimSignal(getAnalogStick(RightHatX));
          rc_signals[THROTTLE] = trimSignal(getCombinedTriggerThrottle());
          break;
      }
      
      if (Xbox.getButtonClick(A)) {
      	if (flag[0] == 1) {
      		flag[0] = 0;	
      	} else {
      		flag[0] = 1;
      	}
      }

      if (Xbox.getButtonClick(B)) {
        if (flag[1] == 1) {
          flag[1] = 0;  
        } else {
          flag[1] = 1;
        }
      }

      if (Xbox.getButtonClick(X)) {
        if (flag[2] == 1) {
          flag[2] = 0;  
        } else{
          flag[2] = 1;
        }
      }

      if (Xbox.getButtonClick(Y)) {
        if (flag[3] == 0) {
          flag[3] = 1;
        } else if (flag[3] == 1) {
          flag[3] = 2;
        } else {
          flag[3] = 0;
        }
      }
      	
    	if (flag[0] == 1){
    		rc_signals[AUX1] = trimSignal(PWM_HIGH);
    	} else {
    	  rc_signals[AUX1] = trimSignal(PWM_LOW);	
    	}

      if (flag[1] == 1){
       rc_signals[AUX2] = trimSignal(PWM_HIGH);
      } else {
        rc_signals[AUX2] = trimSignal(PWM_LOW); 
      }
      
      if (flag[2] == 1){
        rc_signals[AUX3] = trimSignal(PWM_HIGH);
      } else {
        rc_signals[AUX3] = trimSignal(PWM_LOW); 
      }

      if (flag[3] == 0) {
        rc_signals[AUX4] = trimSignal(PWM_LOW);
      } else if (flag[3] == 1) {
        rc_signals[AUX4] = trimSignal(PWM_MIDDLE);
      } else {
        rc_signals[AUX4] = trimSignal(PWM_HIGH);
      }

      
  		for(char i = 0; i < 8; i++){
        byte MSB = getMSB(rc_signals[i]);
        byte LSB = getLSB(rc_signals[i]);

        if (DEBUG) {
          Serial.print(getInt(MSB, LSB), DEC);
          Serial.print(" ");
          
  //        Serial.print(rc_signals[i], DEC);
  //        Serial.print("\t");
        }

        if (!DEBUG) {
          Serial.write(breaks[i]);
        } else {
          Serial.print(breaks[i], HEX);
          Serial.print(" ");
        }
  
        if (!DEBUG) {
  			  Serial.write(MSB); 
        } else {
          Serial.print(MSB, HEX);
          Serial.print(" ");
        }

        if (!DEBUG) {
  			  Serial.write(LSB);
        } else {
          Serial.print(LSB, HEX);
          Serial.print(" ");
        }
  		}
    }
  }
}

byte getMSB(uint16_t value) {
  return (value >> 8);
}

byte getLSB(uint16_t value) {
  return (value & 0x00FF);
}

uint16_t getInt(byte MSB, byte LSB) {
  return (MSB << 8) | LSB;
}


