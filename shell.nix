{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
    buildInputs = [
        pkgs.gcc
        pkgs.cmake
        pkgs.SDL2
        pkgs.glfw
        pkgs.glew
        pkgs.emscripten
        pkgs.zlib
    ];

    shellHook = ''
        set -e  # Enable exit on error
        set -u  # Enable exit on undefined variable

        export PATH=$PATH:$PWD/node_modules/.bin
        export PS1="\[\e[32m\]\w\[\e[33m\] (nix-shell) $ \[\e[0m\]"
        export PS1="\[\e[33m\](nix-shell) \[\e[32m\]\w $ \[\e[0m\]"

        git submodule update --init --recursive

        export DEPS_DIR="$HOME/.local/vortex_deps"
        mkdir -p $DEPS_DIR
        echo 🍍 DEPS_DIR=$DEPS_DIR

        if [ ! -f $DEPS_DIR/dependencies_installed ]; then
        
            ASSIMP_BUILD_SCRIPT=$(pwd)/external/build-assimp.sh
            ASSIMP_BUILD_DIR=$DEPS_DIR/assimp-build
            ASSIMP_SRC_DIR=$(pwd)/external/assimp
            mkdir -p $ASSIMP_BUILD_DIR/native $ASSIMP_BUILD_DIR/wasm 
            echo "🍍 Installing Assimp..."
            (cd $ASSIMP_BUILD_DIR/native && time $ASSIMP_BUILD_SCRIPT $ASSIMP_SRC_DIR)
            echo "🍍 Installed Assimp native!"
            (cd $ASSIMP_BUILD_DIR/wasm && time $ASSIMP_BUILD_SCRIPT $ASSIMP_SRC_DIR WASM)
            echo "🍍 Installed Assimp for Wasm!"

            ### BULLET3_BUILD_SCRIPT=$(pwd)/external/build-bullet3.sh
            ### BULLET3_BUILD_DIR=$DEPS_DIR/bullet3-build
            ### BULLET3_SRC_DIR=$(pwd)/external/bullet3
            ### mkdir -p $BULLET3_BUILD_DIR/wasm $BULLET3_BUILD_DIR/native
            ### echo "🍍 Installing Bullet3..."
            ### (cd $BULLET3_BUILD_DIR/native && time $BULLET3_BUILD_SCRIPT $BULLET3_SRC_DIR)
            ### echo "🍍 Installed Bullet3 native!"
            ### (cd $BULLET3_BUILD_DIR/wasm && time $BULLET3_BUILD_SCRIPT $BULLET3_SRC_DIR WASM)
            ### echo "🍍 Installed Bullet3 for Wasm!"

            touch $DEPS_DIR/dependencies_installed
            echo "🍍 Dependencies installed in $DEPS_DIR."
        else
            echo "🍍 Dependencies are already installed in $DEPS_DIR."
        fi

        set +e  # Disable exit on error
        set +u  # Disable exit on undefined variable
  ''; 
}
