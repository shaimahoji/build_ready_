# 🛡️ Tank Battle Simulator — README

## 🧑‍💻 Submitters

* Adan Assi — 322719139
* Shaimaa Hoji — 211961057

---

## 🗂️ Project Structure

This project is divided into **5 folders** as required:

```
📁 Simulator/
📁 Algorithm/
📁 GameManager/
📁 common/        # Provided by course staff — not modified
📁 UserCommon/    # Shared code between our files (with unique namespace)
```

---

## ⚙️ Compilation

Each of the 3 main folders (`Simulator`, `Algorithm`, `GameManager`) contains its own Makefile.

At the root of the project is a master Makefile that builds all components.

---

## 🚀 Executable

The built simulator is named:

```
simulator_322719139_211961057
```

It supports both `-comparative` and `-competition` modes with robust validation and logging.

---

## 🧠 Implementation Overview

### 🔧 Simulator

* Dynamically loads `.so` files for:

  * Algorithms from the `Algorithm/` folder
  * GameManagers from the `GameManager/` folder
* Supports **multithreaded execution** using `num_threads=N` argument
* Dispatches runs according to mode:

  * **Comparative Mode:** Runs all GameManagers in folder with a pair of algorithms on one map.
  * **Competition Mode:** Runs a full round-robin tournament across multiple maps and algorithms.

🔹 Simulator Implementation Details
* Dynamic .so Loading
   * GameManagers and Algorithms are loaded at runtime via dlopen.
   * Handles are stored and later released via dlclose in the destructor.
   * Automatic registration ensures factories are available immediately after loading.
*Map Parsing
   * Reads a structured map file with a header (MaxSteps, NumShells, Rows, Cols) and the game    board.
   * Tanks are automatically detected and stored in player_tank_positions.
   * Missing or invalid fields are handled gracefully, returning std::nullopt without crashing.
* Comparative Mode Execution
   * Retrieves all registered GameManagers and algorithms.
   * Supports single-threaded or multithreaded execution via a ThreadPool.
   * Each game is run independently, and results are grouped by outcome.
   * Outputs a detailed comparative results file with map serialization.
* Competition Mode Execution
   * Loads multiple algorithms and maps.
   * Deduplicates matchups to avoid redundant games.
   * ThreadPool parallelization handles multiple games concurrently.
   * Scores are automatically computed:
      * 3 points for a win
      * 1 point per player in a tie
  * Results are sorted and written to a competition output file.
* Thread-Safe Operations
  * std::mutex ensures run_results and scores are updated safely in parallel.
  * Each game instance runs in isolation to avoid shared state conflicts.
* Utilities
  * serializeMap and serializeMap2 convert the board to string format for logging.
  * generateTimestamp provides unique filenames for output.
  * Detailed verbose logging available for debugging.
* Error Handling & Robustness
  * Missing or invalid files, invalid .so libraries, or failed registrations are handled gracefully.
  * Simulator avoids crashes even with malformed input, missing maps, or insufficient algorithms.

### 📚 Command Line Parsing

* Implemented with detailed validation
* Prints clear error messages and usage if:

  * An argument is missing
  * A file or folder path is invalid
  * Invalid flags are passed

### 🧰 Shared Library Handling

* `.so` files are loaded using `dlopen`, validated with `dlsym`, and unloaded with `dlclose`.
* Failures during loading/registration are gracefully handled and reported to stderr.

### 📦 Auto Registration

We implemented auto-registration via:

* `PlayerRegistration.cpp`
* `TankAlgorithmRegistration.cpp`
* `GameManagerRegistration.cpp`

Factories are added to the `AlgorithmRegistrar` and `GameManagerRegistrar` singletons. Each algorithm registers itself via the macros:

```cpp
REGISTER_TANK_ALGORITHM(OffensiveTankAlgorithm)
REGISTER_PLAYER(OffensivePlayer)
REGISTER_GAME_MANAGER(GameManager)
```

---

## 🧪 GameManager

* Full game logic implemented in `GameManager.cpp`
* Manages:

  * Tank movement and rotation
  * Shell collisions and damage
  * Wall health
  * Mine triggering
* Handles **cooldown**, **reverse moves**, and **backward sequences**
* Visualizes game state in a file-compatible JSON format

### 📄 Output Behavior

* In **comparative mode**, produces:

  ```
  game_managers_folder/comparative_results_<timestamp>.txt
  ```

* In **competition mode**, produces:

  ```
  algorithms_folder/competition_<timestamp>.txt
  ```

If the output file can't be written, results are printed to `stdout`.

---

## 🤖 Algorithms

Located in `Algorithm/`, we implemented:

### 🧠 OffensiveTankAlgorithm

* Reacts dynamically to battle info

* Triggers **BattleInfo** request every few steps or when needed

* Uses pathfinding and threat evaluation to:

  * Chase enemy tanks
  * Avoid collisions
  * Avoid friendly fire

* Our algorithms are within the unique namespace:

```cpp
namespace Algorithm_322719139_211961057
```

---

## 📛 Namespaces

Each part uses a unique namespace based on our IDs:

* **GameManager:** `GameManager_322719139_211961057`
* **Algorithm:** `Algorithm_322719139_211961057`
* **UserCommon:** `UserCommon_322719139_211961057`

---

## ❗ Error Handling

* Output files and streams are guarded with fallback behavior
* `input_errors.txt` is generated on invalid board characters or loading issues
* Simulator will **not crash** on invalid `.so` libraries or registrations

---

## ✅ Compliance with Assignment 3

✔ Each part is isolated
✔ `.so` file names include our IDs
✔ Auto-registration is used
✔ Output format and threading model follow specs
✔ `new/delete` avoided in favor of `std::unique_ptr`
✔ `dlclose()` used properly to unload libraries
✔ Verbose logging available with `-verbose`

---

## 🧾 Additional Notes

* No third-party libraries used beyond `nlohmann::json` (standard-approved)
* `Simulator` gracefully handles crash-free logic but does not recover from .so file crashes (as permitted)

* `GameManager` includes both log files and visualization JSON (per round)
