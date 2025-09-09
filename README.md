# Gameboy Emulator

A Gameboy emulator written in C++, The project replicates the original Nintendo Gameboy released back in 1990. Its capable of loading the original ROM files and executing it to simulate the original console's behavior so it can be run on any windows computer.

![idk](screenshots/Screenshot%202025-09-07%20220909.png)
![idk](screenshots/Screenshot%202025-09-07%20215119.png)
![idk](screenshots/Screenshot%202025-09-07%20193549.png)

**(Does not support audio)**

# Features
- Supports Games that use the MBC0 - MBC3, memory bank controllers
- Built in debugging tools such as a basic disassembler and a memory viewer

# How to run
 - Requires Cmake and a Internet connection to download packages 
 - Requires a compiler that uses C++ 23
    
    Create build directory
        
        mkdir build && cd build

    Generate build files
        
        cmake ..

    Build the project
        
        cmake --build .
# Technologies
 - ImGui
 - Raylib
 - Gtest  (for basic testing)

# Future Improvements
 - Add support for other platforms than windows
 - Add audio
 - Switch backend to just OpenGL and GLFW  to use ImGui Dockspace
 - Add more ways to change emulation (Example changing color pallete)
 - Adding savestates 


#### *Aman Pathak* 