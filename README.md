Logger1
=======

Tool for logging RGB-D data from the Microsoft Kinect and ASUS Xtion Pro Live. 

Should build on Linux, MacOS and Windows. Requires CMake, Boost, Qt4, OpenNI, ZLIB and OpenCV. 

Grabs RGB and depth frames which are then compressed (lossless ZLIB on depth and JPEG on RGB) and written to disk in a custom binary format. Multiple threads are used for the frame grabbing, compression and GUI. A circular buffer is used to help mitigate synchronisation issues that may occur. 

Uses OpenNI 1.x.

The binary format is specified in Logger::writeData() in Logger.cpp

<p align="center">
  <img src="http://mp3guy.github.io/img/Logger1.png" alt="Logger1"/>
</p>

