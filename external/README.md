External Library Management
===========================

External libraries are added to this repo as modules in ./external directory.

Getting the source code of external modules 
-------------------------------------------

If the Vortex Boilerplate is already cloned,
run this in in its root directory:

    git submodule init
    git submodule update

Adding a new library
--------------------

Add library repo as a new module:

    git submodule add https://github.com/libsdl-org/SDL.git external/SDL

Go into the library directory:

    cd ./external/SDL

See available tags:

    git tag

Checkout your favorite:

    git checkout tags/release-2.30.8

now go back to the repository root:

    cd ../../

Add and commit the library checked-out version:

    git add external/SDL
    git commit -m "Add SDL submodule at tag release-2.30.8"
