# WiiMouse
It uses the Wii Controller to move your mouse.

The DPAD Controls the mouse

The A button is left click

The B button is right click 2.6 and above

Pressing A & B at the same time will press Windows key 3.0 and above

The - button decreases the Dpad sensitivity

The + button increases the Dpad sensitivity

The Home button "holds" the mouse in postition so you can move the controller down and use the Dpad instead.

The 1 button increases the pointer sensitivity

The 2 button decreases the pointer sensitivity

There is pointer stabilization in versions above 2.6


If you want to build it yourself
https://sourceforge.net/projects/wiiuse/
gcc -o wiimote_pointer13 wii.c wiiuse user32
