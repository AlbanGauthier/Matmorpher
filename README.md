## MatMorpher: A Morphing Operator for SVBRDFs

![Teaser image](teaser_fig.png)

**MatMorpher: A Morphing Operator for SVBRDFs**<br>
[Alban Gauthier](https://perso.telecom-paristech.fr/gauthier/), [Jean-Marc Thiery](https://perso.telecom-paristech.fr/jthiery/) and [Tamy Boubekeur](https://perso.telecom-paristech.fr/boubek/)<br>
*EGSR 2021 (Symposium Track)* <br>
### [Project page](https://perso.telecom-paristech.fr/boubek/papers/MatMorpher/) | [Paper](https://diglib.eg.org/bitstream/handle/10.2312/sr20211298/141-153.pdf) | [Presentation](https://www.youtube.com/watch?v=WUBf30dzk3Q&t=3592s) | [EG diglib](https://diglib.eg.org/handle/10.2312/sr20211298)

---

## Citation
```
@inproceedings {Gauthier2021MatMorpher,
	booktitle = {Eurographics Symposium on Rendering - DL-only Track},
	editor = {Bousseau, Adrien and McGuire, Morgan},
	title = {MatMorpher: A Morphing Operator for SVBRDFs},
	author = {Gauthier, Alban and Thiery, Jean-Marc and Boubekeur, Tamy},
	year = {2021},
	publisher = {The Eurographics Association}
}
```

## Table of Contents

- [Usage](#usage)
	- [GUI with default arguments](#gui-with-default-arguments)
	- [GUI with custom materials and warpgrid](#gui-with-custom-materials-and-warpgrid)
	- [Warpgrid computation](#warpgrid-computation)
- [Building](#building)
	- [Prerequisites](#prerequisites)
	- [Windows](#windows)
	- [Linux](#linux)
- [License](#license-mit)
- [Acknowledgment](#acknowledgment)

## Usage

A lot of paths are hardcoded relatively to the binary, hence the /src/ and /bin/ folders should not be modified.

To get some help, launch "MatMorpher.exe" in /bin/ from a command line and add "-help" as an argument.

On Linux, the executable requires the qt5 gui lib (sudo apt-get install libqt5gui5)

### GUI with default arguments

Simply open "MatMorpher.exe" in the /bin/ directory

- Keys and mouse options are given in the cmd at launch.
- Timings for Gaussianization and Normal Map Reorientation are also printed.
- On the top left in the OpenGL view, info are given about :
	- Materials currently loaded
	- Warp grid currently loaded
	- YCbCr vs RGB Interpolation (for ours: right view)
- To load a material : 	File > Open Material 1 or 2
- To load a warp grid : File > Open Warp Grid

### GUI with custom materials and warpgrid

```
Matmorpher.exe gui material1 material2 warpgrid.txt
```

### Warpgrid computation

- First use the command "contours" to visualize the result of the contour extraction (output in .pdf)

```
Matmorpher.exe contours material_folder
```

- Then use the command "warpgrid" to compute and output the warpgrid from two provided materials.

```
Matmorpher.exe warpgrid mat1_folder XXXXX mat2_folder XXXXX
```

Replace each X in XXXXX with 0 or 1, to use in the following order : color, height, metallic, normal, roughness.

Optionnally, you can change some parameters for the computation :

```
Matmorpher.exe warpgrid mat1_folder XXXXX mat2_folder XXXXX grid_size alpha beta
```

Where:
- grid_size is the height/width of the warpgrid (default: 128)
- alpha modulates the interior regularity term (default: 4000)
- beta modulates the periodic harmonicity term (default: 200)

### Remarks

- The same default parameters have been used to create all results shown online. You can tweak these parameters to better adjust the warpgrid for a pair of material.
- The warpgrid computation is non-commutative : a great way of achieving the best interpolation is to compute the two warpgrids 1->2 and 2->1 and choose the best one with the least deformation.

## Building

### Prerequisites :
 - CMake
 - Python 3
 - Eigen3
 - Libpng
 - Qt 5.13 or above

### Windows

#### Getting the required libs

- Installing Qt5 & Eigen3 using Microsoft [vcpkg](https://github.com/microsoft/vcpkg.git) :
- If Qt is already installed on your computer, simply add your Qt path to CMAKE_PREFIX_PATH (e.g. C:/Qt/5.15.2/msvc2019_64/lib/cmake/)

note: It is recommended to install vcpkg in directories like 'C:\src\vcpkg' or 'C:\dev\vcpkg'.

```
git clone https://github.com/microsoft/vcpkg.git  
.\vcpkg\bootstrap-vcpkg.bat  
vcpkg.exe install qt5-base
vcpkg.exe install eigen3 --triplet x64-windows  
vcpkg.exe install libpng --triplet x64-windows
vcpkg.exe integrate install  
```

#### Building using CMake and Visual Studio 19

You can set up the build files using:
```
git clone --recurse-submodules https://github.com/AlbanGauthier/Matmorpher
cd material-interpolator
mkdir build-msvc16
cd build-msvc16
```

Do not forget to add '--recurse-submodules'.  
Otherwise you can fetch the submodules afterwards using : 

```
git submodule update --init --recursive
```

Now to build using CMake (replace PATH_TO_VCPKG): 

```
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/.../vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

### Linux

Export the Qt dir to the Linux PATH:
```
export PATH="~/Qt/5.15.2/gcc_64/lib/cmake:$PATH"
```

Install the eigen package using:
```
sudo apt install libeigen3-dev
```

Now build using CMake: 

```
git clone https://github.com/blablabla
cd material-interpolator  
mkdir build-linux  
cd build-linux  
cmake ..   
make
```

Then copy the executable in the /bin/ folder

## License (MIT)

```
Copyright (c) 2021 - Télécom Paris - Alban Gauthier

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the “Software”), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

The Software is provided “as is”, without warranty of any kind, express or
implied, including but not limited to the warranties of merchantability,
fitness for a particular purpose and non-infringement. In no event shall the
authors or copyright holders be liable for any claim, damages or other
liability, whether in an action of contract, tort or otherwise, arising
from, out of or in connection with the software or the use or other dealings
in the Software.
```

### Acknowledgment

The PBR textures provided in the Release are from Adobe Substance Source  
A huge thanks to [Elie Michel](https://github.com/eliemichel) and [Thibaud Lambert](https://github.com/Daepso) for allowing me to quickly edit their code and come up with an early prototype based on their work.
