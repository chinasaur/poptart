poptart
=======

Toast notifications for RaspberryPi.

This allows you to popup toast notifications without having a running X session.  The notification text is rendered using OpenVG, just like the hello_font example program.

RasPi firmware's libvgfont has been patched to fix some rendering issues; you
shouldn't need to do anything but here's the PR if for some reason you need to
refer to it:
  https://github.com/raspberrypi/firmware/pull/95

Right now there is no help text and the notifications only appear centered at the bottom of the screen.  There are a number of other options; see source for now (sorry!).

The Makefile should work on Raspbian; it assumes libvgfont is built and is in /opt/vc/...  Older versions requried Vera.ttf to be in PWD when you ran poptart, but I believe I've fixed this so that it instead assumes Vera.ttf is in /opt/vc/...  This is untested but will be tested soon.  I'll try to make it more flexible about where it finds the font soon.  And eventually I hope to rewrite libvgfont to be a lot more flexible about these things.

The tagged versions are commits that I at least tested that they run.  Other commits are not even guaranteed to compile as I am not at my RPi every day of the week and don't have a virtual dev environment setup.  No first release yet but will come soon.

muptart is an example program that I use to show the current playing song from Music Player Daemon.  I don't use the mpc idle functionality because I usually play internet radio streams, which don't give you any notification when the track changes.  So for this case we simply run poptart in an infinite loop.  A script using mpc idle should instead should be easy to put together.

conktart is an example program using conky-cli to display system info.

pantart is an example program for showing Pandora station information via 
pianod.
