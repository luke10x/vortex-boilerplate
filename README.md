
# Vortex Game Engine

Vortex is a modular game engine designed for flexibility and ease of use.
It allows developers to create games by providing a simple structure
that supports drop-in functionality and user-defined game logic.

## Architecture Overview

The core components of the Vortex engine are encapsulated
in the `VortexContext` structure, which contains the following members:

- **VBO**: Vertex Buffer Object for managing graphical data.
- **SDL_Window**: The SDL window used for rendering.
- **btWorld**: The Bullet physics world for handling physics simulations.
- **UserContext**: A user-defined structure of type `vtx::UserContext`
  that allows for game-specific data.


```cpp
VortexContext ctx;
User-Defined Functions
The engine provides three essential user-defined functions to manage game logic:

vtx::usr::init(): Called once at the beginning of the game to set up necessary resources and initial states.
vtx::usr::loop(): Called in a loop to update the game state and handle rendering. This is where the main game logic resides.
vtx::usr::exit(): Called once at the end of the game to clean up resources and perform any necessary shutdown operations.
```

## Example Usage

To get started with your own game, follow these steps:

Define Your User Context

Create a user-defined structure to hold your game-specific data:

cpp
Copy code
namespace vtx {
    struct UserContext {
        // Add your game-specific variables here
    }
}
Implement User Functions

Define the required functions in your main game file:

```cpp
namespace vtx::usr {
    void init() {
        // Initialization logic
    }

    void loop() {
        // Game loop logic
    }

    void exit() {
        // Cleanup logic
    }
}
```

The main game loop can be initiated as follows:

```cpp
int main() {
    ctx = VortexContext(); // Initialize the Vortex context
    vtx::usr::init();      // Call user-defined init

    while (true) {         // Main loop
        vtx::usr::loop();  // Call user-defined loop
    }

    vtx::usr::exit();      // Call user-defined exit
    return 0;
}
```

```bash
git clone https://github.com/luke10x/vortex-boilerplate.git
cd vortex
```

Run the example project to see the engine in action.

Contributing
Contributions are welcome! Please submit a pull request or open an issue to discuss changes.

License
This project is licensed under the MIT License. See the LICENSE file for details.