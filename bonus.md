# Tank Battle Simulator – Running Tests

This guide explains how to build and run tests for the Tank Battle Simulator project, including comparative and competitive modes.

---

## 1. Prerequisites

* C++ compiler supporting C++17 (g++ recommended).
* CMake version >= 3.10.
* GoogleTest (fetched automatically via CMake).
* Your project folder should have:

```
/maps                 -> folder with game map files
/game_managers_folder -> folder with GameManager .so files
/algorithms_folder    -> folder with algorithms .so files
/Simulator            -> simulator source files
/common
/UserCommon
/tests                -> GoogleTest source files
```

---

## 2. Required Downloads / Dependencies

1. **C++ Compiler**

   * Must support **C++17**.
   * Recommended: **g++** (Linux/macOS), **MSVC** (Windows).

2. **CMake**

   * Version **>= 3.10**.
   * Download: [https://cmake.org/download/](https://cmake.org/download/)

3. **GoogleTest**

   * Automatically downloaded by CMake using `FetchContent`.

4. **Linux dependencies** (if running on Linux)

```bash
sudo apt update
sudo apt install build-essential cmake libpthread-stubs0-dev
```

5. **Project files** (should be included in repo)

   * Simulator sources (`Simulator/*.cpp`)\
   * common sources (`common/*.cpp`)
   * UserCommon sources (`UserCommon/*.cpp`)
   * game_managers_folder `.so` files
   * algorithms_folder `.so` files (at least have two - one must be Algorithm_322719139_211961057.so )
   * Maps folder with `.txt` map files
   * Tests folder with GoogleTest source files

6. **Optional for Windows users**

   * MinGW or Visual Studio for C++17 support.

---

## 3. Build the Project and Tests

1. Create a build directory (if not already created):

```bash
mkdir -p build
```

2. Generate CMake build files:

```bash
cmake -B build -S .
```

3. Build simulator, GameManager/Algorithm `.so` files, and tests:

```bash
cmake --build build
```

* The simulator executable will be in `build/Simulator/`.
* The tests executable will be in `build/tests`.

---

## 4. Run All Tests

Run the tests with:

```bash
./build/tests
```

* Output shows which tests ran and whether they passed.
* Each test prints debug info, including the exact simulator command being executed.

---

## 5. Notes

* Tests use absolute paths generated via `SOURCE_DIR` from CMake.
* Output files for comparative mode are generated under `game_managers_folder` with names like:

```
comparative_results_YYYYMMDD_HHMMSS.txt
```

* If a test fails, check the simulator exit code and the debug command printed.
* Ensure `.so` files for GameManagers and Algorithms are built with `-fPIC` and available in the expected folders.
