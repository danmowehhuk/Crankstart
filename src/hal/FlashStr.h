#ifndef _crankstart_hal_FlashStr_h
#define _crankstart_hal_FlashStr_h

// FlashStr is a type-tag for "this pointer is to a flash-resident
// string." The actual definition (and the F() macro that produces one)
// lives in BareMetalHAL off Arduino - it's shared across every library
// in this family, not something Crankstart owns. Branches on NO_ARDUINO
// directly, the one flag every library in this family checks. Anything
// platform-specific about how a flash string is actually read
// (pgm_read_byte on AVR, a plain dereference on ARM, ...) is
// BareMetalHAL's concern, never this file's.

#ifndef NO_ARDUINO
#include <Arduino.h>
using FlashStr = __FlashStringHelper;
// F() is already provided by Arduino.h
#else
#include <BareMetalHAL.h>
using FlashStr = BareMetalHAL::FlashStr;
// F() is already provided by BareMetalHAL.h
#endif

#endif
