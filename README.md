# Modern C++ Project Template

A modern C++ project template using **Premake5** for flexible builds, external dependencies, and clean architecture separation. Ideal for scalable applications or libraries.


## Project Structure (Tree view)

```bash
premake5.lua # Main premake5 implemantation

src/ # If working with multiple projects, copy this folder and add a new group to premake5.lua

├── ProjName.lua # The project specific implementation of premake 

├── global/ # Global headers, macros, configs

├── include/ # Public API headers

├── json/ # JSON files or third-party JSON support

├── ProjName/ # Main entry point or core logic (e.g., main.cpp)

├── src/ # Core source files

scripts/ # Premake scripts or build utilities

vendor/ # External libraries (imgui, GLFW, etc.)

```
**Note: If you are planning to rename the project (which you should), be sure to edit the *.lua files, as they define the premake structure for your project

##  Build Instructions

### Requirements
- C++23 or later (Can be respecified in premkae5.lua)
- [Premake5](https://premake.github.io/)
- A supported C++ compiler (MSVC, Clang, or GCC)

### 🔧 Generating the Project
From the project root:

```bash
premake5 vs2022        # for Visual Studio 2022
# OR
premake5 gmake2        # for GNU Make
# OR
premake5 xcode4        # for Xcode
```

Then open the generated solution or run the build with your toolchain.

## External Libraries
Place external libraries (like ImGui, GLFW, GLEW, etc.) in the vendor/ directory. Make sure to:

Include them in your Premake scripts/ configuration

Set include/lib paths appropriately

## Tips for Modularity
Use include/ for headers you want to expose to other projects.

Keep internal headers and source in src/ and lib-api/internal/.

Avoid putting implementation logic directly in include/.

## Notes
This template is meant to be extended. You can:

Add unit tests under a tests/ folder

Extend scripts/ to support additional build platforms or configs

Integrate CI/CD or clang-format/linting tools


## Happy coding! 🎉
