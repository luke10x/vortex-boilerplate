{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = [
    pkgs.gcc
    pkgs.cmake
    pkgs.SDL2
    pkgs.glfw
    pkgs.glew
    pkgs.emscripten
  ];

  shellHook = ''
    echo "SDL2, OpenGL, and GLEW development environment is ready."
  '';
}
