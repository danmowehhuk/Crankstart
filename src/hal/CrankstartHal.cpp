#include "CrankstartHal.h"

#ifdef NO_ARDUINO
#include <BareMetalHAL.h>
#endif

namespace CrankstartHal {

bool flashStrEqual(const char* a, const char* b) {
  char c;
  do {
    c = readByte(a++);
    if (c != readByte(b++)) return false;
  } while (c != '\0');
  return true;
}

#ifdef NO_ARDUINO

char readByte(const char* flashPtr) {
  return BareMetalHAL::readByte(flashPtr);
}

void print(const FlashStr* s) {
  BareMetalHAL::Uart0::print(s);
}

void println(const FlashStr* s) {
  BareMetalHAL::Uart0::println(s);
}

void delay(uint32_t ms) {
  BareMetalHAL::delay(ms);
}

#endif  // NO_ARDUINO

}  // namespace CrankstartHal
