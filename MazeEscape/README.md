# 3D Maze Escape

**Computer Graphics Fundamentals — Final Project**

## 1. Project Overview

3D Maze Escape is an interactive first-person 3D maze game developed in C++ using OpenGL.

The player explores a stone dungeon, searches for a hidden key, unlocks a door, and attempts to escape before the timer expires.

The project demonstrates real-time 3D rendering, shader programming, lighting, camera control, collision detection, texture mapping, and interactive gameplay.

## 2. Game Objective

The player must:

1. Explore the maze using first-person controls.
2. Find and collect the hidden key.
3. Locate the locked exit door.
4. Pass through the unlocked doorway.
5. Escape before the 90-second timer reaches zero.

The game ends with either a victory or a timeout.

## 3. Technologies Used

| Technology | Purpose |
|---|---|
| C++ | Main programming language |
| OpenGL 3.3 Core | Real-time 3D graphics |
| FreeGLUT | Window creation and keyboard/mouse input |
| GLEW | OpenGL extension loading |
| GLM | Matrix and vector mathematics |
| GLSL | Vertex and fragment shaders |
| stb_image | Loading image textures |
| Visual Studio | Development environment |

## 4. Main Features

### 3D Maze Environment
- Stone-textured walls and floors.
- Perspective projection and depth testing.
- Cube-based maze geometry.
- Collision detection to prevent walking through walls.

### Lighting and Shaders
- Custom vertex and fragment shaders.
- Blinn–Phong lighting model.
- Ambient, diffuse, and specular lighting calculations.
- Lighting toggle using the `L` key.
- Independently movable light source.

### Camera System
- First-person camera with mouse-look controls.
- Free-camera mode for exploring the scene.
- Perspective projection using GLM.

### Gameplay
- Randomized key and exit locations.
- Key collection and locked-door mechanics.
- 90-second countdown timer.
- Win and lose states.
- Game restart and pause functionality.

### User Interface
- Countdown timer.
- Real-time FPS counter.
- Key collection status.
- Shader, camera, and light mode indicators.
- Game status display.
- Help overlay and win/lose screens.

## 5. Controls

| Key | Action |
|---|---|
| W / A / S / D | Move the player |
| Mouse | Look around |
| F | Toggle free-camera mode |
| Q / E | Move vertically in free-camera mode |
| L | Toggle Blinn–Phong lighting |
| T | Toggle manual light control |
| I / J / K / ; / U / O | Move the light source in manual mode |
| P | Pause / resume |
| H | Show / hide help |
| R | Restart the game |

## 6. Installation and Running

### Requirements

- Windows operating system.
- Microsoft Visual Studio with C++ development tools.
- OpenGL-compatible graphics hardware.
- FreeGLUT, GLEW, and GLM dependencies.

### Steps

1. Clone the repository:

   ```bash
   git clone https://github.com/dauletbaeva06/CGF_FINAL
   ```

2. Open the Visual Studio solution (`.sln`).

3. Ensure that the required C++ libraries are installed and configured.

4. Confirm that the texture files are available:

   ```text
   assets/
     textures/
       wall.jpg
       floor.jpg
   ```

5. Select the `x64` build configuration.

6. Build and run the project using `Ctrl + F5` or Visual Studio's Run command.

**Note:** Texture paths are resolved relative to the application's working directory. Ensure the asset paths match those used in `main.cpp`.

## 7. Rendering Performance

The application was tested at a resolution of 1280 × 720.

| Rendering Configuration | Observed FPS | Estimated Frame Time |
|---|---:|---:|
| Blinn–Phong ON | 115 | 8.70 ms |
| Blinn–Phong OFF | 129 | 7.75 ms |
| Win State with UI Overlay | 145 | 6.90 ms |

All observed configurations exceeded the project's minimum target of 30 FPS.

These values represent observed FPS readings rather than averages collected over a timed benchmark.

## 8. Project Structure

```text
MazeEscape/
├── main.cpp
├── assets/
│   └── textures/
│       ├── wall.jpg
│       └── floor.jpg
├── .gitignore
├── packages.config
└── README.md
```

Visual Studio solution and project files are also required to build the application.

## 9. Graphics Implementation

The application uses a custom OpenGL rendering pipeline with GLSL shaders.

The vertex shader transforms 3D vertices using model, view, and projection matrices.

The fragment shader handles texture sampling and Blinn–Phong lighting calculations.

GLM is used for camera transformations, perspective projection, and object positioning.

The user interface is rendered using an orthographic projection and custom 2D elements.

## 10. Conclusion

3D Maze Escape demonstrates the practical application of computer graphics concepts in an interactive 3D environment.

The project combines real-time rendering, custom shaders, textured geometry, lighting, camera movement, collision detection, and gameplay mechanics.

The observed performance results show that the application runs above the minimum FPS requirement while supporting interactive rendering and user controls.