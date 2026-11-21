#ifndef _crankstart_hal_CrankstartHal_h
#define _crankstart_hal_CrankstartHal_h

// The platform calls Crankstart's core (src/) makes: reading flash strings
// to match module names, and Serial and timing for its DEBUG-gated
// diagnostics. With -DNO_ARDUINO they go to BareMetalHAL instead of the
// Arduino core; that branch is defined in CrankstartHal.cpp.

#include "FlashStr.h"

#ifndef NO_ARDUINO
#include <Arduino.h>
#else
#include <stdint.h>
#endif

namespace CrankstartHal {

#ifndef NO_ARDUINO

inline char readByte(const char* flashPtr) { return (char)pgm_read_byte(flashPtr); }
inline void print(const FlashStr* s) { Serial.print(s); }
inline void println(const FlashStr* s) { Serial.println(s); }
inline void delay(uint32_t ms) { ::delay(ms); }

#else

char readByte(const char* flashPtr);
void print(const FlashStr* s);
void println(const FlashStr* s);
void delay(uint32_t ms);

#endif

// Returns true if the two flash strings have the same text.
bool flashStrEqual(const char* a, const char* b);

}  // namespace CrankstartHal

#endif
