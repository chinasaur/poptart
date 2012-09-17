poptart
=======

Toast notifications for RaspberryPi.

This allows you to popup toast notifications without having a running X session.  The notification text is rendered 
using OpenVG, just like the hello_font example program.

To run well with longer inputs, this needs some patches to libvgfont provided with Raspbian: 
  https://github.com/raspberrypi/firmware/pull/95

Right now there is no help text and the notifications only appear centered at the bottom of the screen.  See source for
available option flags.

mpcpop is an example program that I use to show the current playing song from Music Player Daemon.  I don't use the 
mpc idle functionality because I usually play internet radio streams, which don't give you any notification when the
track changes.  So for this case we simply run poptart in an infinite loop.