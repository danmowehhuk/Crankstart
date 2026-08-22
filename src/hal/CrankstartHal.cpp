#include "CrankstartHal.h"

#ifdef NO_ARDUINO
#include <BareMetalHAL.h>

namespace CrankstartHal {

void print(const FlashStr* s) {
  BareMetalHAL::Uart0::print(s);
}

void println(const FlashStr* s) {
  BareMetalHAL::Uart0::println(s);
}

void delay(uint32_t ms) {
  BareMetalHAL::delay(ms);
}

}  // namespace CrankstartHal

#endif  // NO_ARDUINO
