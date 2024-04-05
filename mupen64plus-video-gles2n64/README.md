mupen64plus-video-gles2n64
==========================

Manually ported from mupen64plus-ae for use with the Raspberry PI

Building and installing
=======================

First, ensure you have Mupen64Plus built and installed. Then, run make in the sub-directory ./project of this source tree, then sudo make APIDIR=$path all to install. $path is the path to Mupen64Plus headers located in ./mupen64plus/include/mupen64plus/ directory.

You'll then obtain the plugin mupen64plus-video-n64.so to be copied to ./mupen64plus/lib/mupen64plus/ directory.

You can start Mupen64Plus with --gfx mupen64plus-video-n64 to use the plugin.
