

# Official Statement: 
1. This project must be compiled on a Linux system
2.  requiring the use of C++20.

</details>

## How To Run?
### Install XMake
  * Please read this [here](https://xmake.io/#/zh-cn/guide/installation)
  ```
  curl -fsSL https://xmake.io/shget.text | bash
  ```
### Build and Run:
```
xmake
xmake r
```

  ## Dependencies
  - OS:   Ubuntu 24.04.2 LTS x86_64(kernal 6.11.0-26-genneric)
  - Complier: gcc 13.3.0 or clang 18.1.3 *(MSCV is not capable)*
  - Graphical API: glfw 3.4 + glad 4.0.1
  - GUI framework: ImGui-1.91.7-docking 
  - Data Visialization tool: implot v0.16
  - Constuction system: XMake v2.9.9+HEAD.40815a0
  - C++ standard: C++20
## Dir structure
```
ztest/
├── core/              # Core framework
│   ├── ztest_logger.hpp     # Logging system
│   ├── ztest_thread.hpp     # Thread pool implementation
│   ├── ztest_registry.hpp   # Test registration mechanism
│   └── ...                 # Other core modules
├── gui.hpp            # Main program for the graphical user interface
file1.hpp # Test file
file1.cpp # Test file
main.cpp # Main program
xmake.lua # Build script
```
## Development Background 
Existing unit testing frameworks, such as Google Test, have several drawbacks, including a steep learning curve and so on. Our team plans to develop a flexible, efficient, and easy-to-use testing tool (with a graphical user interface, GUI) to provide an intuitive and user-friendly environment for developers and testers to write, run, and manage test cases. This tool will support various types of testing (such as unit testing and integration testing) and provide detailed test result reports.

## Function Introduction
* Test Cases Management 
* Assertation
* Result Reporting with AI
* Native Parrallel Running Context with Thread Pool
* Data-Driven Testing with Data Monitoring
* BENCHMARK TESTING
