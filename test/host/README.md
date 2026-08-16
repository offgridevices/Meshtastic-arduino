# Host tests

Compiles the library on a development machine against a stub Arduino layer,
so the parts that do not depend on a radio can be checked without one.

```sh
sh test/host/run.sh
```

Requires a C++17 compiler and nothing else.

`Arduino.h` here is a stub — just enough of the Arduino API for the library to
build and link. It is not a simulator, and passing these tests says nothing
about behaviour against real hardware.

What is covered: the packet-metadata callback, including that it reports
packets whose portnum is unrecognised and packets whose payload could not be
decrypted, since both are still evidence about the radio link.
