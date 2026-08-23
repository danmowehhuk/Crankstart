#ifndef _crankstart_hal_CrankstartHal_h
#define _crankstart_hal_CrankstartHal_h

// Consolidates the Serial and Timing calls Crankstart's core (src/)
// needs for its DEBUG-gated diagnostics, so they can be redirected to
// BareMetalHAL when building with -DNO_ARDUINO. The Arduino branch below
// is a pure refactor of what Crankstart.cpp already did inline - no
// behavior change. The NO_ARDUINO branch is declared here and defined in
// CrankstartHal.cpp, delegating to BareMetalHAL.

#include "FlashStr.h"

#ifndef NO_ARDUINO
#include <Arduino.h>
#else
#include <stdint.h>
#endif

namespace CrankstartHal {

#ifndef NO_ARDUINO

inline void print(const FlashStr* s) { Serial.print(s); }
inline void println(const FlashStr* s) { Serial.println(s); }
inline void delay(uint32_t ms) { ::delay(ms); }

#else

void print(const FlashStr* s);
void println(const FlashStr* s);
void delay(uint32_t ms);

#endif

}  // namespace CrankstartHal

#endif
