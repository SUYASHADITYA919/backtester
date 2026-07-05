## Backtester in C++

An end-to-end, high-performance event-driven trading engine designed to simulate historical trading strategies with modular infrastructure components. Unlike synchronous, vector-based architectures (e.g., Python/Pandas loops) that suffer from look-ahead bias and unrealistic execution modeling, this project uses an asynchronous-style event loop processing framework. It mimics the tick-by-tick or bar-by-bar cascading workflow of an institutional-grade quantitative trading platform.

---

## 🏗️ Architecture Overview

The backtester relies on a central **Event Queue** to decouple data processing, strategy generation, risk management, and market execution.

              +-----------------------+
              |  Historical Data Feed |
              +-----------+-----------+
                          |
                     MarketEvent
                          v
              +-----------+-----------+
              |      Event Queue      | <=========+
              +-----------+-----------+           |
                          |                       |
             MarketEvent  |  SignalEvent          | Order/Fill Events
                          v                       |
     +--------------------+--------------------+  |
     |                                         |  |
     v                                         v  |
+--------+--------+                       +--------+--+-----+
| Trading Strategy|                       | Portfolio & Risk|
|     Module      |                       |    Management   |
+--------+--------+                       +--------+--------+
|                                         |
| SignalEvent                             | OrderEvent
+------------------->====================+
|
v
+--------+--------+
|    Execution    |
| Sim / Brokerage |
+-----------------+


### Components Matrix

1. **Event Infrastructure (`Event`, `MarketEvent`, `SignalEvent`, `OrderEvent`, `FillEvent`)**: Polymorphic types passed as smart pointers (`std::shared_ptr`) via a centralized `std::queue`.

2. **Data Handler (`DataHandler`)**: Handles structural I/O ingestion of historical CSV tick datasets, feeding simulated market information dynamically into the main processing loop.

3. **Strategy Core (`MovingAverageStrategy`)**: Concrete engine calculating algorithmic triggers (such as technical indicator crossovers) while entirely isolated from balance tracking or portfolio exposure state variables.

4. **Portfolio & Ledger Manager (`Portfolio`)**: Controls cash-to-equity transformations, keeps order sizing constraints aligned with strict risk criteria, and keeps mathematical tabs on account positions.

5. **Execution Simulator (`simulate_execution`)**: Simplifies exchange connectivity behavior, computing slippage adjustments and broker transaction commission fees before dispatching execution reports.

---

## 🛠️ Prerequisites & Installation

### Windows (Using MSYS2 & MinGW-w64 Toolchain)
1. Download and run the setup installer from [msys2.org](https://www.msys2.org/).
2. Open the **MSYS2 UCRT64** terminal and install the compiler environment and compilation utilities:
   ```bash
   pacman -Syu
   pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
Append the binary directory path to your Windows Environment Variables:

Path: C:\msys2\ucrt64\bin

macOS
Install Apple's Command Line Tools along with CMake using Homebrew:

```Bash
xcode-select --install
brew install cmake ninja
Linux (Ubuntu / Debian / Mint)
Update repository indices and pull the full compiler suite using apt:

```

```Bash
sudo apt update
sudo apt install build-essential cmake ninja-build gdb

```

⚙️ Configuration Setup

1. Build & Run Automation (.vscode/tasks.json)
JSON
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "CMake Build",
            "type": "shell",
            "command": "cmake --build build",
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "dependsOn": "CMake Configure"
        },
        {
            "label": "CMake Configure",
            "type": "shell",
            "command": "cmake -B build"
        }
    ]
}

2. Native C++ Debugger Targets (.vscode/launch.json)
JSON
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug Backtester",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/Debug/backtester.exe",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ]
        }
    ]
}

🚀 Execution & Compilation

Open your cpp_backtester/ root directory workspace inside VS Code.

Initialize and configure the system generator cache from the terminal layout:

PowerShell
cmake -B build -G "Ninja"
Run the automated executable construction:

PowerShell
cmake --build build
Run the newly generated native application bundle:

PowerShell
.\build\Debug\backtester.exe