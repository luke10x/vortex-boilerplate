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

1. Add library repo as a new module:

    git submodule add https://github.com/libsdl-org/SDL.git external/SDL

2. Go into the library directory:

    cd ./external/SDL

Skip to step 4 if you don't want to use tags.

3. See available tags:

    git tag

4. Checkout your favorite :

    git checkout tags/release-2.30.8

or (if you dont want to pin to a tag):

    git checkout main

5. now go back to the repository root:

    cd ../../

6. Add and commit the library checked-out version:

    git add external/SDL
    git commit -m "Add SDL submodule at tag release-2.30.8"
