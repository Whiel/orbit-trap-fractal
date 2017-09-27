orbit-trap-fractal
------------------

Implementation of orbit trap fractals inspired by this video: https://youtu.be/ykxpflZl53c

Build
-----

This project depends on GLFW, GLM, GLEW and a modern c++ compiler.
Furthermore LodePNG is bundled in the source. No modifications were done to the original library.
    
    mkdir build
    cd build 
    cmake ..
    make

Use
---
    
    julia <source image> [-m]

The source image is the image embedded in the fractal. 
The -m flag can be specified to set the fractal coefficient by hand (dragging the mouse) insteaad of the by-default animation.
The space bar creates a snapshot. The snapshot size is quite huge and the encoding takes a while, so its done asynchronously.
Close the window or press q to exit.
When the window is closed, if some captures are being encoded, the process will wait before terminating.

Known Issues
------------

- Cannot switch to mandelbrot without recompiling.
- All snapshots are called the same and overwritten.
- Switching mouse and animation should be possible without closing and opening the window.
- Cleanup of main is required.

Results
-------
![rainbow input](https://raw.githubusercontent.com/Whiel/orbit-trap-fractal/master/example-images/in-rainbow.png)
![rainbow output](https://raw.githubusercontent.com/Whiel/orbit-trap-fractal/master/example-images/out-rainbow-scaled.png)
