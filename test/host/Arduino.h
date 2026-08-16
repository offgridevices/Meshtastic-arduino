// Just enough Arduino to compile the library on a development machine.
// Not part of the library; lives outside its tree on purpose.
#ifndef ARDUINO_H_STUB
#define ARDUINO_H_STUB

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <cmath>

inline uint32_t millis() { return 0; }
inline void delay(uint32_t) {}
inline long random(long howbig) { return howbig ? 12345 % howbig : 0; }

struct SerialStub {
  void begin(unsigned long) {}
  void flush() {}
  void print(const char * s) { std::fputs(s, stdout); }
  void print(int v) { std::printf("%d", v); }
  void println(const char * s) { std::printf("%s\n", s); }
  void println(unsigned long v) { std::printf("%lu\n", v); }
  void println(long v) { std::printf("%ld\n", v); }
  void println(unsigned int v) { std::printf("%u\n", v); }
  void println(int v) { std::printf("%d\n", v); }
  void println() { std::printf("\n"); }
  operator bool() const { return true; }
};

extern SerialStub Serial;

#endif
