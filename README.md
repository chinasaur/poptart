poptart
=======

Toast notifications for RaspberryPi.

This allows you to popup toast notifications without having a running X session.  The notification text is rendered 
using OpenVG, just like how the hello_font example program works.

To run well with longer inputs, this needs some patches: https://github.com/raspberrypi/firmware/pull/95

mpcpop is an example program that I use to show the current playing song from Music Player Daemon.  I don't use the 
mpc idle functionality because I usually play internet radio streams, which don't give you any notification when the
track changes.  So for this case we simply run poptart in an infinite loop.