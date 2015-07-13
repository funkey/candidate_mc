Candidate Multi-Cut
===================

Getting Submodules
------------------

  ```
  git submodule update --init
  ```

Dependencies
------------

  * CMake, Git, GCC
  * libboost-all-dev (make sure libboost-timer-dev is included)
  * liblapack-dev
  * libfftw3-dev
  * libx11-dev
  * libx11-xcb-dev
  * libxcb1-dev
  * libxrandr-dev
  * libxi-dev
  * freeglut3-dev
  * libglew1.6-dev
  * libpng12-dev
  * libtiff4-dev
  * libhdf5-serial-dev
  * Gurobi

    Get the gurobi solver from http://www.gurobi.com. Academic licenses are free.

Configure:
----------

  ```
  mkdir build
  cd build
  cmake ..
  ```

  Set cmake variable Gurobi_ROOT_DIR (or the environment variable
  GUROBI_ROOT_DIR before you run cmake) to the Gurobi subdirectory containing
  the /lib and /bin directories.

Compile:
--------

  ```
  make
  ```
