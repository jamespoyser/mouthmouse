/*
 * Hardware:
 *  Pro Micro             
 *  HX711 bridge sensor on pins 8 & 9  
 *  Analogue joystick on pins A1 & A2
 * 
 */

#include <Mouse.h>

const boolean DEBUG = false;   // Outputs helpful data to serial port @ 57.6k

/* Hardware config: */
const int PRESSURE_CLOCK_PIN = 8;  
const int PRESSURE_DATA_PIN  = 9;   
const int JOYSTICK_Y_PIN     = A1;
const int JOYSTICK_X_PIN     = A2;

/* Characteristics: */
const int SENSITIVITY = 100;            // lower number = faster mouse speed
const int JOYSTICK_THRESHOLD = 1;       // mouse only moves when over this threshold
const long PRESSURE_THRESHOLD = 100000; // mouse clicks only when over this threshold

/* Run time variables: */
int zero_x;
int zero_y;
long zero_pressure;
int mouse_pressed = 0;                  // -1 == left; +1 == right


void setup() {
  if (DEBUG) {
    Serial.begin(57600);
  }; 

  pinMode(JOYSTICK_X_PIN, INPUT);
  pinMode(JOYSTICK_Y_PIN, INPUT);

  pinMode(PRESSURE_CLOCK_PIN, OUTPUT);
  pinMode(PRESSURE_DATA_PIN,  INPUT);
  digitalWrite(PRESSURE_CLOCK_PIN, LOW);

  // Read and discard first pressure reading
  pressureRead();
  delay(100);

  // Set the zero-values for x,y and pressure
  zero_x = analogRead(JOYSTICK_X_PIN);
  zero_y = analogRead(JOYSTICK_Y_PIN);
  zero_pressure = pressureReadRaw();
  
}

void loop() {
  int current_x = joystickRead(JOYSTICK_X_PIN, zero_x);
  int current_y = joystickRead(JOYSTICK_Y_PIN, zero_y);
  
  if (DEBUG) {
   Serial.print("x: ");
   Serial.print(current_x);
   Serial.print(" y: ");
   Serial.println(current_y);
  }; 

  // Is the joystick in a non-zero position?
  // If so, move mouse
  if (current_x != 0  || current_y != 0) {
    Mouse.move(current_x / SENSITIVITY, current_y / SENSITIVITY, 0);
    //if (abs(current_x) > JOYSTICK_THRESHOLD) Mouse.move(current_x/SENSITIVITY, 0, 0);
    //if (abs(current_y) > JOYSTICK_THRESHOLD) Mouse.move(0, current_y/SENSITIVITY, 0);

    delay(10);  // This governs the top speed of the mouse
  } 
  
  // Is the joystick in a zero position?
  // If so, read and act on pressure
  if (current_x == 0 && current_y == 0) {

    int current_pressure = pressureRead();

    if (current_pressure == 0 && mouse_pressed != 0 ) {
      Mouse.release(MOUSE_RIGHT); 
      Mouse.release(MOUSE_LEFT); 
      mouse_pressed = 0;     
    }
  
    if (current_pressure == 1 && mouse_pressed == 0) {
      Mouse.press(MOUSE_LEFT); 
      mouse_pressed = 1; 
    }
    
    if (current_pressure == -1 && mouse_pressed == 0) {
      Mouse.press(MOUSE_RIGHT); 
      mouse_pressed = -1; 
    }
    
  }

}

// Reads joystick value
// If it's above the threshold, returns the value
// Otherwise returns 0
int joystickRead(int pin, int zero) {

  int value = analogRead(pin) - zero;

  if (abs(value) < JOYSTICK_THRESHOLD) {
    value = 0;
  }

  return value;
}


// Reads & normalises a pressure value
// If it's above threshold, returns either:
//  1   'puffing' - for left click
//  -1  'sipping' - for right click
//  0   neutral
int pressureRead() {
  long value = pressureReadRaw() - zero_pressure;

  if (DEBUG) {
   Serial.print("p: ");
   Serial.println(value);
  }; 
  
  if (value > PRESSURE_THRESHOLD) 
    return 1;

  if (value < 0-PRESSURE_THRESHOLD) 
    return -1;

  return 0;
}

// Reads raw value from pressure sensor
long pressureReadRaw(){
  byte index;
  long value = 0L;
  
  /* Read in the 24 bit conversion data */
  while(digitalRead(PRESSURE_DATA_PIN));
  for (index = 0; index < 24; index++)
  {
    digitalWrite(PRESSURE_CLOCK_PIN, HIGH);
    value =  (value << 1) | digitalRead(PRESSURE_DATA_PIN);
    digitalWrite(PRESSURE_CLOCK_PIN, LOW);
  }
  
  /* Output some extra clock cycles to set the gain and input options */
  // 3 == CHAN_A_GAIN_64 conversion mode :-/
  for (index = 0; index < 3; index++)
  {
    digitalWrite(PRESSURE_CLOCK_PIN, HIGH);
    digitalWrite(PRESSURE_CLOCK_PIN, LOW);
  }
  
  /* Number is returned as a 24bit 2's compliment but we need to 
     convert it to convert to 32 bit singed integer */
  if (value >= 0x800000)
    value = value | 0xFF000000L;

  return value;

}

