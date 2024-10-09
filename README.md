
Vortex ~Engine~~ Boilerplate for no-engine development
======================================================

Vortex is a modular system designed for flexibility and ease of use.
It allows developers to create games by putting aside the boilerplate code initializing OpenGL and libraries,
whilst exposing all the details of the underlying libraries.

Architecture Overview
---------------------

Vortex Boilertplate will setup:

- OpenGL initialization;
- Game loop (both compatible with Emscripten and native builds);
- Provide functions to perform common tasks,
  like compile shaderPrograms.


To write your program using Vortex Boilerplate:

- You must call `vtx::openVortex()` as last statement in your main function;
- you must define and link function `vtx::init(vtx.VortexContext ctx)`; 
- you must define and link function `vtx::loop(vtx.VortexContext ctx)`;
- within `vtx::loop` it should call `vtx::exitVortex()`, 
  it will effectively stop the main loop, and this is the right way
  to exit the program.


Example Usage
-------------

To get started with your own program using Vortex Boilerplate
follow these steps:

```bash
git clone https://github.com/luke10x/vortex-boilerplate.git
cd vortex
```

Define the required functions in your main game file:

```cpp
void vtx::init(vtx::VortexContext ctx) {
    // Initialization logic
}

void vtx::loop(vtx::VortexContext ctx) {
    // Main loop logic
    if (shouldEndNow) {

    }
}

```