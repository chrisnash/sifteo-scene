sifteo-scene
============

Scene description framework for Sifteo Cubes

## Introduction

Sifteo Cubes have a rather unique graphics architecture which can present some coding challenges.

- Graphic assets need to be downloaded to each cube's local memory before they can be used;
- In-memory video buffers are sent asynchronously over the radio to the individual cubes;
- The video buffer can be in several graphics modes, but only one is operational at a time;
- there is limited support for windowing and rotation to render only part of the screen and allow mode composition;

Furthermore, the execution environment of the Sifteo Base itself has some restrictions

- Code in ROM needs to be fetched into a RAM cache to execute, a potentially slow operation;

Additionally, the environment can change

- Cubes may be connected or disconnected;
- The system sends event messages notifying such changes during a repaint call

Making a robust framework that can handle these events, correctly manage assets, and execute the core game loop
can be quite a challenge. Very soon I found myself writing the same code over and over again to disconnect a video
buffer, do some manipulation, reconnect it, redraw it, and so on. The purpose of this project is to develop a
simple framework that, given a scene list, will hide all the asset management and give a simple update/redraw/refresh
loop.

