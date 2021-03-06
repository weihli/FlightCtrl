#include "buzzer.h"

#include "mcu_pins.h"
#include "union_types.h"


// =============================================================================
// Private data:

static volatile uint32_t pending_pattern_ = 0;
static volatile uint8_t pending_repetions_ = 0;
static volatile uint8_t repetitions_ = 0;


// =============================================================================
// Private function declarations:

static inline void BuzzerOn(void);
static inline void BuzzerOff(void);


// =============================================================================
// Public functions:

void BuzzerInit(void)
{
  BUZZER_DDR |= BUZZER_PIN;
}

// -----------------------------------------------------------------------------
// This function commands the buzzer to sound for the requested duration in
// milliseconds (up to 2 seconds).
void BeepDuration(uint16_t duration)
{
  // Note: With link-time full-program optimization and constant arguments, this
  // function will likely get inlined and pending_pattern_ will be evaluated at
  // compile-time!!!
  pending_pattern_ = (1L << ((duration * 16) / 1000)) - 1;
  pending_repetions_ = 1;
}

// -----------------------------------------------------------------------------
// This function commands the buzzer to sound n times. Each repetition will
// sound for the requested duration, followed by a silence of the same duration.
// The maximum allowable duration is 1 second.
void BeepNTimes(uint8_t n, uint16_t duration)
{
  // Note: With link-time full-program optimization and constant arguments, this
  // function will likely get inlined and pending_pattern_ will be evaluated at
  // compile-time!!!
  pending_pattern_ = ((1L << (2 * (duration * 16) / 1000)) - 1)
    ^ ((1L << ((duration * 16) / 1000)) - 1);
  pending_repetions_ = n;
}

// -----------------------------------------------------------------------------
// This function commands the buzzer to beep in the specified pattern. (Each 1
// in the pattern represents a beep duration of 1/16 seconds. Leading zeros
// will be ignored, other zeros will be 1/16 seconds of silence.)
void BeepPattern(uint32_t beep_pattern)
{
  pending_pattern_ = beep_pattern;
  pending_repetions_ = 1;
}

// -----------------------------------------------------------------------------
// This function updates the status of the buzzer. It should be called at 16Hz.
void UpdateBuzzer(void)
{
  static union U32Bytes pattern = { 0 };

  static uint8_t mask = 0;  // A mask bit used to cycle through the pattern.
  static uint8_t byte = 0;  // The active byte of the 32-bit pattern.

  // Load the pending pattern if the buzzer is not currently active.
  if (pending_repetions_ && !repetitions_)
  {
    pattern.u32 = pending_pattern_;
    repetitions_ = pending_repetions_;
    pending_repetions_ = 0;

    // Make sure that there is actually something present in the pattern.
    if (!pattern.u32)
    {
      repetitions_ = 0;
      return;
    }
  }

  if (repetitions_)
  {
    // If the mask is not yet configured, set it to the location of the most
    // significant non-zero bit in the pattern. Otherwise, progress through the
    // pattern.
    if (!mask)
    {
      // First find the most significant non-zero byte (little endian).
      for (byte = 3; !pattern.bytes[byte]; byte--) continue;
      // Now set the mask to the location of the most-significant non-zero bit.
      for (mask = 0x80; !(pattern.bytes[byte] & mask); mask >>= 1) continue;

      BuzzerOn();
    }
    else
    {
      // Turn the buzzer on if the mask bit matches the pattern bit.
      if (pattern.bytes[byte] & mask) BuzzerOn();
      else BuzzerOff();
    }
    mask >>= 1;  // Shift the mask bit.

    // If the mask bit has completely traversed this byte, then move to the next
    // byte or repetition.
    if (!mask)
    {
      if (byte)
      {
        byte--;
        mask = 0x80;
      }
      else
      {
        repetitions_--;
      }
    }
  }
  else
  {
    BuzzerOff();
  }
}

// -----------------------------------------------------------------------------
void WaitForBuzzerToComplete(void)
{
  while (pending_repetions_ || repetitions_) continue;
}


// =============================================================================
// Private functions:

static inline void BuzzerOn(void)
{
  BUZZER_PORT |= BUZZER_PIN;
}

// -----------------------------------------------------------------------------
static inline void BuzzerOff(void)
{
  BUZZER_PORT &= ~BUZZER_PIN;
}
