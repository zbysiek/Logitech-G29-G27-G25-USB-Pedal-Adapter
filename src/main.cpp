#include <math.h>
#include <Arduino.h>
#include <Joystick.h>

const int CHANGE_TIME = 10000;  // How much time pedals need to be pressed before mode change (in miliseconds)
const int DEFAULT_MODE = 0;     // 0 = not combined pedals,  1 = clutch and throttle combined
const int PRESS_FORCE = 500;    // How much pedals need to be pressed down in order to activate mode change (scale 0 - 1023)
const int OFF_VALUE = PRESS_FORCE / 1.5;
const double NOTLINEAR_LEVEL = 2;
const int SMOOTHING_THRESHOLD = 3;



class Pedals {
public:
  int value;
  int mode;
  int pin;
  unsigned long lastTime;
  int thresholdPoint;
  Pedals(int pin) {
    this->value = 0;
    this->mode = 0;
    this->pin = pin;
    this->lastTime = millis();
    this->thresholdPoint = 0;
    pinMode(pin, INPUT);
  }
  void update() {
    int rawValue = 1023 - analogRead(this->pin);
    if (rawValue - thresholdPoint > SMOOTHING_THRESHOLD) {
      thresholdPoint = rawValue - SMOOTHING_THRESHOLD;
    }
    else if (rawValue - thresholdPoint < -SMOOTHING_THRESHOLD) {
      thresholdPoint = rawValue + SMOOTHING_THRESHOLD;
    }
    
    
    if (thresholdPoint < PRESS_FORCE) {
      this->lastTime = millis();
    };
    switch (this->mode) {
      case 0: {
        this->value = thresholdPoint;
        break;
      }
      case 1: {
        int value = thresholdPoint;
        double valueNormalized = (double) value / 1023.0;
        double valueNotlinear = pow(valueNormalized, NOTLINEAR_LEVEL) * 1023.0;
        this->value = (int) valueNotlinear;
        break;
      }
        
      default:
        break;
    }
  }
  void setMode(int mode) {
    this->mode = mode;
  }
};

Pedals throttle(A0);
Pedals brake(A1);
Pedals clutch(A2);
Joystick_ joystick(
    0x03,                     // Joystick ID
    JOYSTICK_TYPE_GAMEPAD,    // HID Type
    0, 0,                     // Button Count, Hat Switch Count
    false, false, true,       // X, Y, Z Axis
    true, true, false,      // Rx, Ry, Rz Axis
    false, false,              // rudder, throttle
    false, true, false        // accelerator, brake, steering
);  
int mode = DEFAULT_MODE;
bool isModeChange = false;

void setup() {
  joystick.begin();
  joystick.setRxAxisRange(0, 1023); 
  joystick.setBrakeRange(0, 1023);
  joystick.setRyAxisRange(0, 1023);
  joystick.setZAxisRange(-1023, 1023);

}

void loop() {
  throttle.update();
  brake.update();
  clutch.update();



  unsigned long timeNow = millis();
  if ((timeNow - throttle.lastTime >= CHANGE_TIME) &&
      (timeNow - clutch.lastTime >= CHANGE_TIME)
  ) {
    if (!isModeChange) {
      mode++;
      mode = mode % 2;
      isModeChange = true;
    }
  } else {
    isModeChange = false;
  };


  switch (mode) {
    case 0: {
      brake.setMode(1);
      joystick.setRxAxis(throttle.value);
      joystick.setBrake(brake.value);
      joystick.setRyAxis(clutch.value);
      joystick.setZAxis(512);
      break;
    }
    case 1: {
      int combinedValue = clutch.value * -1 + throttle.value;
      brake.setMode(1);
      joystick.setRxAxis(OFF_VALUE);
      joystick.setBrake(brake.value);
      joystick.setRyAxis(OFF_VALUE);
      joystick.setZAxis(combinedValue);
      break;
    }
    default: break;
  }
  
}