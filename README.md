# ESP32 Dual-Speed Robot Control — Lab 1

**Course:** ICT 361
**Submission Type:** Group

## Group Members
- LY SOKPISEY
- LINH KIMSOURMANA

## Objective
This project implements a control system for a 4-wheel robot using a joystick and push buttons. Direction (forward, backward, left, right, stop) is selected by the joystick, while two independent speed variables — `forwardSpeed` (linear motion) and `rotationSpeed` (turning motion) — are controlled separately, similar to how real mobile robots decouple linear and rotational speed.

The joystick tilt amount is also used to proportionally scale the applied speed up to the button-set maximum, so a light push moves the robot slowly and a full push moves it at the current max speed.

## Hardware Setup

| Input             | GPIO | Function                     |
|-------------------|------|-------------------------------|
| Joystick X-axis   | 34   | Left / right direction        |
| Joystick Y-axis   | 35   | Forward / backward direction  |
| UP button         | 16   | Increase `forwardSpeed` (+5)  |
| DOWN button       | 15   | Decrease `forwardSpeed` (-5)  |
| RIGHT button      | 4    | Increase `rotationSpeed` (+5) |
| LEFT button       | 2    | Decrease `rotationSpeed` (-5) |

| Motor Driver Output | GPIO (Dir A / Dir B / PWM) |
|----------------------|------------------------------|
| Motor 1              | 25 / 26 / 33 (LEDC ch 0)     |
| Motor 2              | 27 / 32 / 14 (LEDC ch 1)     |
| Motor 3              | 18 / 21 / 5  (LEDC ch 2)     |
| Motor 4              | 22 / 23 / 19 (LEDC ch 3)     |

## Control Logic Summary
- **Joystick** selects movement direction: forward, backward, turn left, turn right, or stop. Y-axis takes priority over X-axis.
- **`forwardSpeed`** and **`rotationSpeed`** start at `50` and are clamped between `0` and `100`.
- **Buttons** adjust these speeds in steps of `±5`, using edge-detected debouncing (150 ms) so a single press produces a single step change.
- **Dead zone** (`JOY_DEADZONE = 300`) is applied around the joystick's center reading (`2048`) so small sensor noise near the resting position doesn't trigger unwanted movement.
- The joystick's push distance beyond the dead zone is scaled (`scaleByJoystick()`) into a percentage of the current max speed, then converted from a 0–100 percentage into a 0–255 PWM duty cycle (`mapSpeed()`) before being written to the motors.


## How to Build & Upload
1. Open `src/dual_speed_robot.ino` in the Arduino IDE.
2. Install the **ESP32 board package** (Boards Manager) if not already installed.
3. Select your ESP32 board under **Tools → Board**.
4. Select the correct COM port under **Tools → Port**.
5. Click **Upload**.
6. Open **Serial Monitor** at **115200 baud** to view direction and speed debug output.

## Flowchart
See [`docs/flowchart.jpg`](docs/flowchart.jpg) for the full control-logic diagram, covering: start, reading joystick/button inputs, updating speeds, clamping limits, dead zone check, motion decision, applying motor PWM, and looping.

## Demo Video
Watch the demonstration here: [Demo Video](PASTE_YOUR_DRIVE_LINK)

## Explanation
## Explanation

### 1. Purpose of Using Two Independent Speeds
Real mobile robots don't use a single speed value for every kind of motion, because linear movement (driving forward or backward) and rotational movement (turning) place different demands on the motors and often need to be tuned separately for smooth, controllable behavior. In this project, `forwardSpeed` controls how fast the robot drives forward or backward, while `rotationSpeed` controls how fast it turns left or right. Keeping these as two independent variables means a user can, for example, set a high forward speed for quick straight-line driving while keeping the rotation speed lower for more precise, controlled turns — something impossible if only one shared speed value existed. Both variables start at `50` and are clamped between `0` and `100` (via `clampSpeed()`), so neither can be pushed to an unsafe or invalid PWM range.

### 2. The Dead Zone
The joystick's analog output is never perfectly stable at its resting position — small electrical noise or minor physical drift means the ADC reading can fluctuate slightly even when the stick isn't being touched. If the robot reacted to every tiny fluctuation, it would twitch or creep even when the user intends for it to be stationary. To prevent this, a dead zone (`JOY_DEADZONE = 300`) is applied around the joystick's center reading (`JOY_CENTER = 2048`). Only when the joystick's deflection (`diffX` or `diffY`) exceeds ±300 counts from center does the code register it as an intentional input; anything smaller is treated as "centered" and the robot stops. This gives stable, predictable behavior at rest while still allowing responsive control once the stick is deliberately pushed.

### 3. Concept of Increasing and Decreasing Speed
The four push buttons adjust the two speed variables in fixed steps of `±5` (`SPEED_STEP = 5`). The UP/DOWN buttons increase or decrease `forwardSpeed`, while LEFT/RIGHT increase or decrease `rotationSpeed`. Each button press is edge-detected — meaning the code only registers a change the instant the button transitions from unpressed to pressed (`HIGH` to `LOW`), rather than continuously for as long as it's held down. This is combined with a 150 ms debounce timer (`DEBOUNCE_MS`) to filter out the rapid electrical bouncing that occurs when a mechanical button is pressed, ensuring one physical press results in exactly one `±5` step rather than several unintended increments. After each adjustment, `clampSpeed()` keeps the result within the valid `0–100` range.

Beyond the button-based adjustment, this implementation also makes the joystick's tilt distance matter: rather than always driving at the full `forwardSpeed`/`rotationSpeed` value the moment the dead zone is crossed, the function `scaleByJoystick()` scales the actual applied speed proportionally to how far the stick is pushed, using the button-set value as a maximum ceiling. A light push produces a small fraction of the max speed; a full push approaches the full max speed. This adds finer, more analog-like control on top of the required digital button adjustment.

### 4. Flowchart Walkthrough
The program follows this loop, matching the flowchart:
1. **Start** — initialization of pins and PWM channels.
2. **Read joystick X, Y** — analog readings from GPIO 34 and 35.
3. **Read button states** — digital readings from the four button GPIOs.
4. **Update forwardSpeed / rotationSpeed** — button presses adjust the relevant speed by ±5, using edge detection and debouncing.
5. **Clamp speeds to 0–100** — ensures speeds always stay within valid bounds.
6. **Check dead zone** — if the joystick is within ±300 of center, the robot stops.
7. **Decide motion** — if outside the dead zone, the Y-axis is checked first (forward/backward take priority); if Y is within the dead zone but X is not, the robot turns left or right instead.
8. **Apply motor speeds** — the selected direction's speed is scaled by joystick tilt (`scaleByJoystick()`), converted from a percentage to an 8-bit PWM value (`mapSpeed()`), and written to all four motors via `ledcWrite()`.
9. **Loop back** — the entire process repeats continuously inside `loop()`, giving continuous real-time control.
