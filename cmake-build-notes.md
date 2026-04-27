# CMake Build Notes

This project was originally configured for Keil MDK. The CMake configuration keeps the same approach by using:

- armclang.exe for C and C++ compilation
- armasm.exe for the ARMASM startup file
- armlink.exe for linking
- Project/stm32f410.sct as the linker scatter file

## Build commands

From the project root:

```bash
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/armclang-keil.cmake
cmake --build build
```

If Keil is installed somewhere else:

```bash
cmake -S . -B build -G Ninja ^
  -DCMAKE_TOOLCHAIN_FILE=cmake/armclang-keil.cmake ^
  -DKEIL_ARMCLANG_DIR="C:/Keil_v5/ARM/ARMCLANG/bin"
cmake --build build
```

## Output

The expected output is:

```text
build/realtime_industrial_sensor.axf
```

## Notes

This CMake setup targets Keil ARMCLANG. It is not an ARM GCC configuration. ARM GCC would require converting the Keil ARMASM startup file and the .sct scatter file into GNU-compatible assembly and linker script formats.
