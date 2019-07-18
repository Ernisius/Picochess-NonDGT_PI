PicoChess
=========

This is quite a complicated project.
I recommend you do not attempt it unless you have some knowledge of digital electronics, Arduino C programming and Python programming.
Without These skills you will find it difficult to locate any wiring or component errors.

It requires a I2c 20*4 LCD display connected to the arduino via I2c.

Functionality.
To scan the reed switches, The Arduino uses a 4017 chip to set each row in turn  to + 5v, Then reads the columns into the arduino via the shift register. 
It doesn't start to monitor buttons or reed matrix until it detects the pieces are set up to starting chess position, comment out the newgame line in the startup code to bypass this check.
When a reed swith opens or closes the led for that square are flashed.
The arduino also monitors the buttons in the same way.
It sends "B:n" for a button change or move (eg "D2D4") to the pi via the usb port.

To test the board, monitor Arduino serial to make sure it is sending the expected messages.

To configure the raspberry pi follow these steps.

Download the last free picochess image from the picochess website. (version N). 
remove or rename /usr/picochess ( to stop this from autostarting when the pi boots.)
download the python code here and copy it to home on the pi.
After rebooting start with 
python3 picochess.py.

picochess is well documented on the picochess website.

When everythin is working you can move the picochess directory to /usr so it starts automatically on boot.
