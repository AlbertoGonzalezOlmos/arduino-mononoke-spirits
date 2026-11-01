/*
 * servoSpirits.ino -- Princess Mononoke forest spirits (kodama)
 *
 * Drives up to five hobby servos so their heads turn, rattle and pause the way
 * the kodama do in the film. Movements are randomized on every power-up so the
 * scene never visibly repeats.
 *
 * ---------------------------------------------------------------------------
 * Design
 * ---------------------------------------------------------------------------
 * Every spirit owns an independent, non-blocking state machine advanced by
 * millis(). loop() never calls delay(), so:
 *
 *   - all attached spirits animate at the same time and drift out of phase,
 *     which is what makes the ensemble look alive;
 *   - the sketch stays responsive to serial input and can be extended with
 *     triggers (button, PIR, IR) without touching the motion code.
 *
 * The software copy of each angle (Spirit::angle) is the single source of
 * truth. Servo::read() is deliberately not used: it reports the last value
 * written to the Servo object even when that object was never attached, so it
 * happily returns a plausible angle for hardware that has not moved.
 *
 * Every angle passes through spiritWrite(), which bounds-checks the spirit
 * index and constrains the angle to [ANGLE_MIN, ANGLE_MAX]. No command can
 * reach a servo outside its mechanical safe range, so no stall / gear-strip /
 * brown-out from a wrapped arithmetic value.
 *
 * ---------------------------------------------------------------------------
 * Wiring
 * ---------------------------------------------------------------------------
 *   Servo 0 signal -> D9      (up to NUM_SPIRITS of D9..D13)
 *   Servo 1 signal -> D10
 *   Servo 2 signal -> D11
 *   Servo 3 signal -> D12
 *   Servo 4 signal -> D13     (also drives the onboard LED -- it will flicker
 *                              at the servo refresh rate; harmless)
 *   Optional reset line -> D2 (see RESET_PIN notes below)
 *   A0 is left floating on purpose: it supplies entropy for randomSeed().
 *
 * Power: five servos rattling can draw 500 mA-1 A EACH. USB (500 mA) is not
 * enough -- the board will brown out and reset mid-show. Use a separate 5-6 V
 * supply rated for the combined stall current, tie its ground to the Nano's
 * ground, and put a bulk capacitor (1000 uF+) across the servo supply rails.
 *
 * Timer note: <Servo.h> claims Timer1 on AVR, so analogWrite() on pins 9 and
 * 10 stops working for the life of this sketch. Do not PWM the UV lights from
 * those two pins; use 3, 5, 6 or 11 instead.
 *
 * ---------------------------------------------------------------------------
 * Behaviour mapping from the previous version
 * ---------------------------------------------------------------------------
 *   rattle()          -> ACTION_RATTLE  (same +/-9 deg at 30 ms, now alternating
 *                                        cleanly and bounded)
 *   turn()            -> ACTION_SWEEP   (same 30-59 deg at 1-99 ms per degree,
 *                                        direction chosen within the safe range)
 *   random wait       -> ACTION_IDLE    (now a real 500-5000 ms pause; it used
 *                                        to collapse to a constant 244 ms)
 *   setupParameters() -> beginAction()
 *   executeActions()  -> updateAction()
 *   resetPin pulse    -> handleResetLine() (explicit pulse width, time based)
 */

#include <Servo.h>

// ============================== CONFIGURATION ==============================

static const byte MAX_SPIRITS = 5;      // size of the pin table / object array
static const byte NUM_SPIRITS = 5;      // <-- servos actually wired up (1..MAX_SPIRITS)

static const byte SERVO_PINS[MAX_SPIRITS] = { 9, 10, 11, 12, 13 };

// Mechanical safe range. Keep a margin from the hard end stops.
static const byte ANGLE_MIN  = 10;
static const byte ANGLE_MAX  = 170;
static const byte ANGLE_HOME = 90;

// Rattle: fast small-amplitude head shake.
static const byte          RATTLE_AMPLITUDE   = 9;     // +/- degrees around the centre
static const unsigned int  RATTLE_STEP_MS     = 30;    // per half-swing
static const byte          RATTLE_CYCLES_MIN  = 8;      // half-swings per burst
static const byte          RATTLE_CYCLES_MAX  = 20;

// Sweep: a slow deliberate turn from the current angle to a new one.
static const byte          SWEEP_ANGLE_MIN    = 30;     // degrees
static const byte          SWEEP_ANGLE_MAX    = 59;
static const unsigned int  SWEEP_STEP_MS_MIN  = 1;      // ms per degree (1 = fastest)
static const unsigned int  SWEEP_STEP_MS_MAX  = 99;

// Idle pause between two actions performed by the same spirit.
static const unsigned long PAUSE_MIN_MS       = 500;
static const unsigned long PAUSE_MAX_MS       = 5000;

// Stagger the spirits at boot so they do not all start on the same beat.
static const unsigned long START_STAGGER_MS   = 400;

// Optional reset line on D2. Set RESET_INTERVAL_MS to 0 to disable.
// NOTE: the previous version pulsed this every 10 blocking loop() iterations,
// with no delay between HIGH and LOW (~3 us wide). That cadence was also being
// reseeded every loop by an out-of-bounds array write, so it never actually
// held. The interval here is wall-clock and the pulse width is explicit.
// If this line is not wired to anything, set RESET_INTERVAL_MS to 0.
static const byte          RESET_PIN          = 2;
static const unsigned long RESET_INTERVAL_MS  = 60000;  // 0 = disabled
static const unsigned int  RESET_PULSE_MS     = 20;

// Detaching a servo between moves stops the idle buzzing and saves current,
// but it also releases torque -- a head mounted on the horn will droop under
// gravity. Leave this false unless the heads are mechanically supported.
static const bool          DETACH_WHEN_IDLE   = false;
static const unsigned long DETACH_AFTER_MS    = 3000;   // idle time before detaching

// Define to stream per-spirit state over the serial port at 1 Hz.
// #define DEBUG_SERIAL

// ============================== SANITY CHECKS ==============================

static_assert(NUM_SPIRITS >= 1,               "NUM_SPIRITS must be at least 1");
static_assert(NUM_SPIRITS <= MAX_SPIRITS,     "NUM_SPIRITS must not exceed MAX_SPIRITS");
static_assert(ANGLE_MIN < ANGLE_HOME,         "ANGLE_HOME must sit above ANGLE_MIN");
static_assert(ANGLE_HOME < ANGLE_MAX,         "ANGLE_HOME must sit below ANGLE_MAX");
static_assert(RATTLE_AMPLITUDE < ANGLE_HOME,  "RATTLE_AMPLITUDE is too large");

// ================================ TYPES ==================================

enum SpiritAction : byte {
  ACTION_IDLE = 0,
  ACTION_RATTLE,
  ACTION_SWEEP
};

struct Spirit {
  Servo         servo;
  bool          attached;      // pin driven by the Servo library right now
  int           angle;         // authoritative position, degrees
  SpiritAction  action;
  unsigned long nextStepAt;    // millis() deadline for the next increment
  unsigned long idleUntil;     // millis() deadline before a new action starts

  // ACTION_SWEEP
  int           sweepTarget;
  int           sweepDir;      // +1 or -1
  unsigned int  sweepStepMs;

  // ACTION_RATTLE
  int           rattleCenter;
  byte          rattleLeft;    // half-swings remaining
  byte          rattlePhase;   // 0 -> centre+amp, 1 -> centre-amp

  // DETACH_WHEN_IDLE
  unsigned long idleSince;
};

// ================================ STATE ==================================

static Spirit spirits[MAX_SPIRITS];

static unsigned long g_resetDueAt   = 0;
static unsigned long g_resetLowAt   = 0;
static bool          g_resetPulsing = false;

#ifdef DEBUG_SERIAL
static unsigned long g_lastReportAt = 0;
#endif

// ============================== SMALL HELPERS ============================

// Wraparound-safe "has this deadline passed?" test.
static inline bool due(unsigned long now, unsigned long deadline) {
  return (long)(now - deadline) >= 0;
}

static inline unsigned int randomInRange(unsigned int lo, unsigned int hi) {
  // random(a, b) is [a, b-1]; callers want an inclusive upper bound.
  return (unsigned int)random(lo, (long)hi + 1);
}

static inline unsigned long randomInRange(unsigned long lo, unsigned long hi) {
  return (unsigned long)random((long)lo, (long)hi + 1);
}

// Seed from a floating analog pin plus the free-running microsecond counter.
// The core never seeds random() itself, so without this the avr-libc default
// seed (1) makes every boot replay the identical show.
static void seedRandomness() {
  unsigned long seed = 0;
  for (byte i = 0; i < 8; i++) {
    seed = seed * 31u + (unsigned long)analogRead(A0);
  }
  seed ^= micros();
  randomSeed(seed ? seed : 1);   // randomSeed(0) is a no-op in the core
}

// The ONLY path to hardware. Bounds-checks the index and clamps the angle, so
// a stray value can never command a servo outside [ANGLE_MIN, ANGLE_MAX] and
// an out-of-range index can never touch memory it should not.
static void spiritWrite(byte index, int angle) {
  if (index >= NUM_SPIRITS) return;

  Spirit &s = spirits[index];
  angle = constrain(angle, (int)ANGLE_MIN, (int)ANGLE_MAX);

  if (!s.attached) {
    s.servo.attach(SERVO_PINS[index]);
    s.attached = true;
    s.idleSince = 0;
  }

  s.angle = angle;
  s.servo.write(angle);
}

// ============================== LIFECYCLE ================================

static void attachAll() {
  for (byte i = 0; i < NUM_SPIRITS; i++) {
    Spirit &s = spirits[i];
    s.servo.attach(SERVO_PINS[i]);
    s.attached    = true;
    s.angle       = ANGLE_HOME;
    s.action      = ACTION_IDLE;
    s.sweepTarget = ANGLE_HOME;
    s.sweepDir    = 1;
    s.sweepStepMs = 20;
    s.rattleCenter = ANGLE_HOME;
    s.rattleLeft  = 0;
    s.rattlePhase = 0;
    s.idleSince   = 0;

    s.servo.write(ANGLE_HOME);

    // Stagger the first action per spirit so the ensemble desynchronises
    // immediately instead of moving as one block.
    s.idleUntil  = millis() + (unsigned long)i * START_STAGGER_MS
                          + randomInRange((unsigned long)0, START_STAGGER_MS);
    s.nextStepAt = s.idleUntil;
  }
}

static void beginAction(byte index) {
  Spirit &s = spirits[index];
  unsigned long now = millis();

  // 1 = rattle, 2 = sweep, 3 = pause. Equal probability, matching the
  // original whichCase == 1/2/3 distribution.
  switch (random(1, 4)) {

    case 1: {                                   // ---- rattle ----
      s.action        = ACTION_RATTLE;
      s.rattleCenter  = s.angle;
      s.rattleLeft    = (byte)randomInRange((unsigned int)RATTLE_CYCLES_MIN,
                                            (unsigned int)RATTLE_CYCLES_MAX);
      s.rattlePhase   = (byte)random(0, 2);     // random first direction
      s.nextStepAt    = now;
      break;
    }

    case 2: {                                   // ---- sweep ----
      int delta = (int)randomInRange((unsigned int)SWEEP_ANGLE_MIN,
                                     (unsigned int)SWEEP_ANGLE_MAX);
      bool canUp   = (s.angle + delta) <= (int)ANGLE_MAX;
      bool canDown = (s.angle - delta) >= (int)ANGLE_MIN;

      if (!canUp && !canDown) {                 // no room: fall back to a pause
        s.action    = ACTION_IDLE;
        s.idleUntil = now + randomInRange(PAUSE_MIN_MS, PAUSE_MAX_MS);
        s.idleSince = now;
        break;
      }

      if (canUp && canDown)  s.sweepDir = random(0, 2) ? 1 : -1;
      else if (canUp)        s.sweepDir = 1;
      else                   s.sweepDir = -1;

      s.action      = ACTION_SWEEP;
      s.sweepTarget = s.angle + s.sweepDir * delta;
      s.sweepStepMs = randomInRange(SWEEP_STEP_MS_MIN, SWEEP_STEP_MS_MAX);
      s.nextStepAt  = now;
      break;
    }

    default:                                    // ---- pause ----
      s.action    = ACTION_IDLE;
      s.idleUntil = now + randomInRange(PAUSE_MIN_MS, PAUSE_MAX_MS);
      s.idleSince = now;
      break;
  }
}

static void finishAction(byte index) {
  Spirit &s = spirits[index];
  s.action    = ACTION_IDLE;
  s.idleSince = millis();
  s.idleUntil = s.idleSince + randomInRange(PAUSE_MIN_MS, PAUSE_MAX_MS);
}

// Advance one spirit's state machine. Never blocks.
static void updateAction(byte index, unsigned long now) {
  Spirit &s = spirits[index];

  switch (s.action) {

    case ACTION_SWEEP: {
      if (!due(now, s.nextStepAt)) break;
      if (s.angle == s.sweepTarget) { finishAction(index); break; }

      int step = (s.angle < s.sweepTarget) ? 1 : -1;
      spiritWrite(index, s.angle + step);
      s.nextStepAt += s.sweepStepMs;
      if (due(now, s.nextStepAt + 250UL)) s.nextStepAt = now;  // resync if starved
      break;
    }

    case ACTION_RATTLE: {
      if (!due(now, s.nextStepAt)) break;

      if (s.rattleLeft == 0) {
        spiritWrite(index, s.rattleCenter);     // settle back on centre
        finishAction(index);
        break;
      }

      int target = s.rattleCenter + (s.rattlePhase == 0 ? (int)RATTLE_AMPLITUDE
                                                        : -(int)RATTLE_AMPLITUDE);
      spiritWrite(index, target);               // constrained inside spiritWrite
      s.rattlePhase ^= 1;
      s.rattleLeft--;
      s.nextStepAt += RATTLE_STEP_MS;
      if (due(now, s.nextStepAt + 250UL)) s.nextStepAt = now;
      break;
    }

    case ACTION_IDLE:
    default:
      break;
  }
}

// Optional torque release. Off by default -- see DETACH_WHEN_IDLE above.
static void updateDetach(unsigned long now) {
  if (!DETACH_WHEN_IDLE) return;

  for (byte i = 0; i < NUM_SPIRITS; i++) {
    Spirit &s = spirits[i];
    if (!s.attached) continue;
    if (s.action != ACTION_IDLE) { s.idleSince = now; continue; }
    if (due(now, s.idleSince + DETACH_AFTER_MS) && due(now, s.idleUntil)) {
      s.servo.detach();
      s.attached = false;
    }
  }
}

// ============================== RESET LINE ===============================

// Drives a proper-width pulse on RESET_PIN. Non-blocking, polarity as before
// (idle LOW, pulses HIGH). If the line is not wired, set RESET_INTERVAL_MS = 0.
static void handleResetLine(unsigned long now) {
  if (RESET_INTERVAL_MS == 0) return;

  if (g_resetPulsing) {
    if (due(now, g_resetLowAt)) {
      digitalWrite(RESET_PIN, LOW);
      g_resetPulsing = false;
      g_resetDueAt   = now + RESET_INTERVAL_MS;
    }
    return;
  }

  if (due(now, g_resetDueAt)) {
    digitalWrite(RESET_PIN, HIGH);
    g_resetPulsing = true;
    g_resetLowAt   = now + RESET_PULSE_MS;
  }
}

// ============================== ARDUINO API ==============================

void setup() {
#ifdef DEBUG_SERIAL
  Serial.begin(9600);
#endif

  seedRandomness();

  pinMode(RESET_PIN, OUTPUT);      // direction BEFORE the first write
  digitalWrite(RESET_PIN, LOW);
  g_resetDueAt = millis() + RESET_INTERVAL_MS;

  attachAll();
}

void loop() {
  unsigned long now = millis();

  for (byte i = 0; i < NUM_SPIRITS; i++) {
    if (spirits[i].action == ACTION_IDLE) {
      if (due(now, spirits[i].idleUntil)) beginAction(i);
    } else {
      updateAction(i, now);
    }
  }

  updateDetach(now);
  handleResetLine(now);

#ifdef DEBUG_SERIAL
  if (due(now, g_lastReportAt + 1000UL)) {
    g_lastReportAt = now;
    for (byte i = 0; i < NUM_SPIRITS; i++) {
      Spirit &s = spirits[i];
      Serial.print(F("s"));   Serial.print(i);
      Serial.print(F(" pin="));  Serial.print(SERVO_PINS[i]);
      Serial.print(F(" ang="));  Serial.print(s.angle);
      Serial.print(F(" act="));  Serial.print((int)s.action);
      Serial.print(F(" left=")); Serial.print(s.rattleLeft);
      Serial.print(F(" att="));  Serial.print(s.attached ? 1 : 0);
      Serial.print(' ');
    }
    Serial.println();
  }
#endif
}
