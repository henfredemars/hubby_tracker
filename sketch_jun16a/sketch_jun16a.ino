
/* Library defines */
#define _TASK_SLEEP_ON_IDLE_RUN

/* Standard headers */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* Headers */
#include <TaskScheduler.h>
#include <avr/wdt.h>

/* 7-segment display defines */
#define SEG_G 8
#define SEG_F 7
#define SEG_E 13
#define SEG_D 12
#define SEG_C 11
#define SEG_B 10
#define SEG_A 9

/* General pin defines */
#define BUZZER_PIN 3

/* Other global defines */
#define IDLE_MILLIS 1200000

/* Forward function declarations (as needed) */
void bootAnimation();
void processSerialCommand();
void waitingAnimation();
void idleClear();
void reset_wdt();
void pulseTone();

/* Global variables */
uint8_t segPins[] = {SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F, SEG_G};
Scheduler scheduler;
long lastEvent = 0;
uint16_t pulseFreq = 0;

// Tasks
Task boot(1000, 11, bootAnimation, &scheduler);
Task serial(25, -1, processSerialCommand, &scheduler);
Task cycle(200, -1, waitingAnimation, &scheduler);
Task checkIdle(1000, -1, idleClear, &scheduler);
Task kickDog(4000, -1, reset_wdt, &scheduler);
Task pulseDriver(0, -1, pulseTone, &scheduler);

void setup()
{
  // put your setup code here, to run once:

  // WDT
  wdt_enable(WDTO_8S);
  reset_wdt();

  // Enable 7-segment display pins
  for (uint8_t i = 0; i < 7; i++)
    pinMode(i, OUTPUT);

  // Enable buzzer pin
  pinMode(BUZZER_PIN, OUTPUT);

  // Enable board features
  Serial.begin(9600);

  // Configure task scheduler
  scheduler.allowSleep(1);

  // Schedule system tasks
  boot.enable();
  serial.enable();
  checkIdle.enable();
  kickDog.enable();
  
}

void loop()
{
  // put your main code here, to run repeatedly:

  // Run the task scheduler
  scheduler.execute();

}

void reset_wdt()
{
  wdt_reset(); // macro
}

void idleClear()
{
  // Go back to cycle animation if idle for 30 minutes
  long curMillis = millis();
  if (curMillis < lastEvent)
  {
    // Wrapped around after 50 days. Causes us to wait an hour to go idle at most, once every 50 days
    lastEvent = 0;
  }
  
  if (curMillis - lastEvent > IDLE_MILLIS)
  {
    cycle.enable();
    lastEvent = curMillis;
  }
}

void processSerialCommand()
{
  // Command format: VERB ARGS... !
  
  static uint8_t cmdBuf[32];
  static uint8_t bufPos = 0;
  
  if (!Serial) return;
  if (!Serial.available()) return;

  // We did something! Cancel the cycle animation and record activity
  boot.disable();
  lastEvent = millis();
  cycle.disable();
  driveSegments(0);

  while (Serial.available())
  {
    if (bufPos >= 32)
    {
      // Prevent buffer overflow
      bufPos = 0;
    }
    cmdBuf[bufPos++] = Serial.read();
  }

  if (cmdBuf[bufPos-1] == '!')
  {
    // Process command
    if (memcmp("HELLO ARDUINO!", cmdBuf, 14) == 0)
    {
      Serial.write("HELLO RPI!");
    }
    else if (memcmp("RING ", cmdBuf, 5) == 0)
    {
      uint8_t ring = atoi(cmdBuf + 5);
      if (ring <= 5)
      {
        // Legal value
        if (ring < 5)
          drive7Seg(ring + 1);
        else
          cycle.enable();
        
        pulseFreq = 2000 - (ring * 220); // Must be positive
        uint16_t pulseDuration = 4000 / (ring + 1); // Must be > 200 for pause time in pulseTone()
        pulseDriver.setIterations(ring + 1);
        pulseDriver.setInterval(pulseDuration);
        pulseDriver.enable();
      }
    }

    // Dump the buffer
    bufPos = 0;
  }
  
}

void pulseTone()
{
  // Pulse at the scheduled frequency and duration, with 100ms of that duration being the pause time
  tone(BUZZER_PIN, pulseFreq, pulseDriver.getInterval() - 200);
}

void waitingAnimation()
{
  static uint8_t animationState = 1;
  if (animationState == 0b1000000)
    animationState = 0b0000001;
  driveSegments(animationState);
  animationState = bitRotate(animationState);
}

uint8_t bitRotate(uint8_t v)
{
  return v << 1 | (v & 0x80) >> 7;
}

void bootAnimation()
{
  static uint8_t numberDisplayed = 0;

  if (numberDisplayed == 0)
    tone(BUZZER_PIN, 1800, 1000);
  drive7Seg(numberDisplayed++);
  if (numberDisplayed > 9)
    numberDisplayed = 0;

  if (boot.isLastIteration())
  {
    // Show loading cycle until something happens
    cycle.enable();
  }
}

void drive7Seg(uint8_t v)
{
  if (v == 0)
    driveSegments(0b0111111);
  else if (v == 1)
    driveSegments(0b0000110);
  else if (v == 2)
    driveSegments(0b1011011);
  else if (v == 3)
    driveSegments(0b1001111);
  else if (v == 4)
    driveSegments(0b1100110);
  else if (v == 5)
    driveSegments(0b1101101);
  else if (v == 6)
    driveSegments(0b1111101);
  else if (v == 7)
    driveSegments(0b0000111);
  else if (v == 8)
    driveSegments(0xFF);
  else if (v == 9)
    driveSegments(0b1101111);
  else // (E)rror
    driveSegments(0b1111001);
}

void driveSegments(uint8_t v)
{
  for (uint8_t i = 0; i < 7; i++)
    digitalWrite(segPins[i], (v & (1 << i)) > 0);

}

