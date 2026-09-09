/*
  4-Wheel Robot — Dual Independent Speed Control (Proportional Joystick)
  ------------------------------------------------------------------------
  Joystick  -> selects DIRECTION, and how far you push it scales the SPEED
               proportionally, up to the max speed set by the buttons.
  Buttons   -> set the MAX speed for two independent speed variables:
                 UP/DOWN    -> forwardSpeed  (used for forward & backward)
                 LEFT/RIGHT -> rotationSpeed (used for turning)

  Pin map:
    Joystick X -> GPIO 34
    Joystick Y -> GPIO 35
    UP button    -> GPIO 16  (increase forwardSpeed)
    DOWN button  -> GPIO 15  (decrease forwardSpeed)
    RIGHT button -> GPIO 4   (increase rotationSpeed)
    LEFT button  -> GPIO 2   (decrease rotationSpeed)
*/

// ---------- Joystick pins ----------
const int JOY_X_PIN = 34;
const int JOY_Y_PIN = 35;

// ---------- Button pins ----------
const int BTN_UP    = 16;   // forwardSpeed +
const int BTN_DOWN  = 15;   // forwardSpeed -
const int BTN_RIGHT = 4;    // rotationSpeed +
const int BTN_LEFT  = 2;    // rotationSpeed -

// ---------- Speed variables (0-100 %) ----------
// These now act as the MAXIMUM speed cap. The joystick tilt decides what
// fraction of this cap actually gets applied to the motors.
int forwardSpeed  = 50;
int rotationSpeed = 50;
const int SPEED_STEP = 5;
const int SPEED_MIN  = 0;
const int SPEED_MAX  = 100;

// ---------- Joystick calibration ----------
const int JOY_CENTER   = 2048;
const int JOY_DEADZONE = 300;   // values within +-300 of center = "no input"

// ---------- Button debounce ----------
const unsigned long DEBOUNCE_MS = 150;
unsigned long lastUpTime = 0, lastDownTime = 0, lastLeftTime = 0, lastRightTime = 0;
int lastUpState = HIGH, lastDownState = HIGH, lastLeftState = HIGH, lastRightState = HIGH;

void setup() {
  Serial.begin(115200);

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);

  pinMode(25, OUTPUT);
  pinMode(26, OUTPUT);
  ledcSetup(0, 20000, 8);
  ledcAttachPin(33, 0);

  pinMode(27, OUTPUT);
  pinMode(32, OUTPUT);
  ledcSetup(1, 20000, 8);
  ledcAttachPin(14, 1);

  pinMode(18, OUTPUT);
  pinMode(21, OUTPUT);
  ledcSetup(2, 20000, 8);
  ledcAttachPin(5, 2);

  pinMode(22, OUTPUT);
  pinMode(23, OUTPUT);
  ledcSetup(3, 20000, 8);
  ledcAttachPin(19, 3);
}

void loop() {
  // 1) Read buttons and update the two MAX speed variables
  updateSpeeds();

  // 2) Read joystick
  int joyX = analogRead(JOY_X_PIN);
  int joyY = analogRead(JOY_Y_PIN);

  int diffX = joyX - JOY_CENTER;
  int diffY = joyY - JOY_CENTER;

  // 3) Decide motion + proportional speed based on joystick tilt
  //    Y-axis (forward/backward) takes priority over X-axis (rotation).
  if (abs(diffY) > JOY_DEADZONE) {

    // ---- IMPORTANT: this line makes speed proportional to tilt ----
    // scaleByJoystick() turns "how far diffY is past the dead zone"
    // into a percentage of forwardSpeed (the button-set max).
    int speed = scaleByJoystick(diffY, forwardSpeed);

    if (diffY < 0) {
      Serial.print("Forward  | speed=");
      Serial.println(speed);
      moveForward(mapSpeed(speed));
    } else {
      Serial.print("Backward | speed=");
      Serial.println(speed);
      moveBackward(mapSpeed(speed));
    }
  }
  else if (abs(diffX) > JOY_DEADZONE) {

    // ---- IMPORTANT: same idea, but for rotationSpeed ----
    int speed = scaleByJoystick(diffX, rotationSpeed);

    if (diffX < 0) {
      Serial.print("Turn Right | speed=");
      Serial.println(speed);
      turnRight(mapSpeed(speed));
    } else {
      Serial.print("Turn Left  | speed=");
      Serial.println(speed);
      turnLeft(mapSpeed(speed));
    }
  }
  else {
    Serial.println("Stop (joystick centered)");
    stopMotors();
  }

  delay(50);
}

// ---- Speed handling ----
// ---- IMPORTANT NEW FUNCTION ----
// Converts joystick deflection (distance past the dead zone) into a
// percentage of maxSpeed. Light push -> small % of maxSpeed.
// Full push (diff near +-2048) -> ~100% of maxSpeed.
// This is what makes the joystick tilt amount matter, on top of the
// button-controlled max speed.
int scaleByJoystick(int diff, int maxSpeed) {
  int magnitude = abs(diff);
  int scaled = map(magnitude, JOY_DEADZONE, 2048, 0, maxSpeed);
  return constrain(scaled, 0, maxSpeed);
}

// Converts a 0-100 percentage speed into a 0-255 PWM value
uint8_t mapSpeed(int percent) {
  percent = constrain(percent, SPEED_MIN, SPEED_MAX);
  return (uint8_t)map(percent, 0, 100, 0, 255);
}

int clampSpeed(int value) {
  if (value < SPEED_MIN) return SPEED_MIN;
  if (value > SPEED_MAX) return SPEED_MAX;
  return value;
}

// Reads all 4 buttons once per loop and applies +/-5 on a falling edge,
// so holding a button does not spam changes every single loop iteration.
void updateSpeeds() {
  unsigned long now = millis();

  int upState    = digitalRead(BTN_UP);
  int downState  = digitalRead(BTN_DOWN);
  int leftState  = digitalRead(BTN_LEFT);
  int rightState = digitalRead(BTN_RIGHT);

  if (upState == LOW && lastUpState == HIGH && (now - lastUpTime) > DEBOUNCE_MS) {
    forwardSpeed = clampSpeed(forwardSpeed + SPEED_STEP);
    lastUpTime = now;
    Serial.print("forwardSpeed (max) increased -> ");
    Serial.println(forwardSpeed);
  }
  if (downState == LOW && lastDownState == HIGH && (now - lastDownTime) > DEBOUNCE_MS) {
    forwardSpeed = clampSpeed(forwardSpeed - SPEED_STEP);
    lastDownTime = now;
    Serial.print("forwardSpeed (max) decreased -> ");
    Serial.println(forwardSpeed);
  }
  if (rightState == LOW && lastRightState == HIGH && (now - lastRightTime) > DEBOUNCE_MS) {
    rotationSpeed = clampSpeed(rotationSpeed + SPEED_STEP);
    lastRightTime = now;
    Serial.print("rotationSpeed (max) increased -> ");
    Serial.println(rotationSpeed);
  }
  if (leftState == LOW && lastLeftState == HIGH && (now - lastLeftTime) > DEBOUNCE_MS) {
    rotationSpeed = clampSpeed(rotationSpeed - SPEED_STEP);
    lastLeftTime = now;
    Serial.print("rotationSpeed (max) decreased -> ");
    Serial.println(rotationSpeed);
  }

  lastUpState    = upState;
  lastDownState  = downState;
  lastLeftState  = leftState;
  lastRightState = rightState;
}

// ---- Motor helper functions ----

void stopMotors() {
  ledcWrite(0, 0);
  ledcWrite(1, 0);
  ledcWrite(2, 0);
  ledcWrite(3, 0);
}

void moveBackward(uint8_t speed) {
  digitalWrite(26, HIGH); digitalWrite(25, LOW);
  digitalWrite(32, HIGH); digitalWrite(27, LOW);
  digitalWrite(18, HIGH); digitalWrite(21, LOW);
  digitalWrite(22, HIGH); digitalWrite(23, LOW);

  ledcWrite(0, speed);
  ledcWrite(1, speed);
  ledcWrite(2, speed);
  ledcWrite(3, speed);
}

void moveForward(uint8_t speed) {
  digitalWrite(26, LOW); digitalWrite(25, HIGH);
  digitalWrite(32, LOW); digitalWrite(27, HIGH);
  digitalWrite(18, LOW); digitalWrite(21, HIGH);
  digitalWrite(22, LOW); digitalWrite(23, HIGH);

  ledcWrite(0, speed);
  ledcWrite(1, speed);
  ledcWrite(2, speed);
  ledcWrite(3, speed);
}

void turnLeft(uint8_t speed) {
  digitalWrite(26, HIGH); digitalWrite(25, LOW);
  digitalWrite(32, HIGH); digitalWrite(27, LOW);
  digitalWrite(18, LOW);  digitalWrite(21, HIGH);
  digitalWrite(22, LOW);  digitalWrite(23, HIGH);

  ledcWrite(0, speed);
  ledcWrite(1, speed);
  ledcWrite(2, speed);
  ledcWrite(3, speed);
}

void turnRight(uint8_t speed) {
  digitalWrite(26, LOW); digitalWrite(25, HIGH);
  digitalWrite(32, LOW); digitalWrite(27, HIGH);
  digitalWrite(18, HIGH); digitalWrite(21, LOW);
  digitalWrite(22, HIGH); digitalWrite(23, LOW);

  ledcWrite(0, speed);
  ledcWrite(1, speed);
  ledcWrite(2, speed);
  ledcWrite(3, speed);
}
