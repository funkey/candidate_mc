Candidate Multi-Cut
===================

Getting Submodules
------------------

  ```
  git submodule update --init
  ```

Dependencies
------------

  * CMake, Git, Mercurial, GCC
  * libboost-all-dev (make sure libboost-timer-dev and libboost-python-dev are included)
  * liblapack-dev
  * libfftw3-dev
  * libx11-dev
  * libx11-xcb-dev
  * libxcb1-dev
  * libxrandr-dev
  * libxi-dev
  * freeglut3-dev
  * libglew-dev
  * libpng12-dev
  * libtiff5-dev
  * libhdf5-serial-dev
  * Gurobi (optional)

  On Ubuntu, you can get these packages via

  ```
  sudo apt-get install libboost-all-dev liblapack-dev libfftw3-dev libx11-dev libx11-xcb-dev libxcb1-dev libxrandr-dev libxi-dev freeglut3-dev libglew-dev libpng12-dev libtiff5-dev libhdf5-serial-dev
  ```

  Get the gurobi solver from http://www.gurobi.com. Academic licenses are free.

  * XQuartz (only on OS X, http://xquartz.macosforge.org/)

Install the Python wrappers
---------------------------

  ```
  mkvirtualenv -p python3.5 ~/.virtualenvs/candidate_mc
  workon candidate_mc
  python setup.py install
  ```

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

Usage:
------

You need:

* raw volume (path "raw" in the following)
* boundary prediction volume (path "boundary" in the following)
* a hierarchical segmentation in one of the following formats:
  * merge-tree: A volume (path "mergetree" in the following) that encodes the hierarchical segmentation with different intensities. The candidates are all connected components of foreground of all possible thresholds of the image. Subset relations are added automatically for candidates that are contained in others. To get candidates without borders in between, the merge-tree image can also be given as a "spaced edge image", which has twice the resolution as the other volumes such that every other voxel corresponds to a voxel in the other volumes. Boundaries are drawn as non-zero intensity lines between the voxels. Candidates are again extracted as connected components of all possible thresholds, however, for spaced edge images they are downsampled by a factor of two to match the raw, intensity, and ground truth volumes.
  * supervoxel image plus merge history: provide a volume containing supervoxel ids, and a text file with rows `a b c` that state that candidate `a` and `b` got merged into `c`.
* optionally: ground truth volume (path "gt" in the following), either as black/white volume or as supervoxel id volume

Every volume can either be a stack of images (in this case, the path points to a directory) or a single image for 2D (in this case, the path points to a single image file).

### Create a Project

Create a project file using the binary `create_project` with the following arguments:

```
--intensities=<raw>
--boundaries=<boundary>
```

For anisotropic volumes, the options `--resX`, `--resY`, and `--resZ` can be given as well.

If your hierarchical segmentation is stored in a merge-tree volume (path "mergetree"), add

```
--mergeTree=<mergetree>
```

If the merge tree is a spaced edge image (see above), add

```
--spacedEdgeImage
```

as well. Ground truth is provided by

```
--groundTruth=<gt>
```

By default, it is assumed that the ground truth is a volume of supervoxel ids. If your ground truth is a black/white image, where each white connected component corresponds to a true foreground object, add the following option:

```
--extractGroundTruthLabels
```

There are more options available that influence the CRAG that is being extracted from the given files, see

```
./create_project --help
```

for details. Once done, you will find a `project.hdf` in you current directory.

### Inspect the CRAG

Start the CRAG viewer with

```
./crag_viewer
```

to inspect the created CRAG.

#### Keybindings

Use `CTRL` and mouse drag/wheel to pan/zoom. For 3D volumes, scrolling without
`CTRL` will move the visible section through the volume, and dragging rotates
the volume. Key 'r' resets the view.

Double clicking on a point on the visible section will show and select the
smallest candidate covering this point. Scrolling with `SHIFT` will show and
select candidates below and above in the candidate hierarchy. Scrolling with
`ALT` will show adjacent candidates of the currently selected candidate. In
assignment models, this shows possible assignments of candidates across
sections. The firs two adjacent candidates are invisible, they are noAssignmentNodes. Key `c` clears the current selection.

#### Usefull Options

| Option            | Description                                                                            |
|-------------------|----------------------------------------------------------------------------------------|
| `--showCosts`     | Show the costs of selected candidates and edges. The default value is `cost`            |
| `--showFeatures`  | Show the features of selected candidates and edges.                                    |
| `--showSolution`  | For the selected candidates and edges, show if they are part of the solution. The default value is `solution`  |
| `--overlay`       | If set to `solution`, the solution is shown instead of the leaf candidates of the CRAG.|
| `--cubeSize`      | Set the cube size for the mesh visualization (in world units).                         |

For more options, see `--help`.

### Extract Features

After the project has been created, features can be extracted for each candidate and adjacency edge. For that, simple run

```
./extract_features
```

For options that influence the type of features being extracted, see

```
./extract_features --help
```

### Learn Feature Weights

If ground truth was provided, you can learn feature weights that produce a segmentation minimizing a structured loss by calling

```
./train
```

Again, see

```
./train --help
```

for options like the loss to use for the learning or the regularizer weight. Once training finished, you will find a `feature_weights.txt` in the current directory.

### Solve

To create a segmentation (for the same or another project with the same features), simple call

```
./solve
```
