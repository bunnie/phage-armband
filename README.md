phage-armband
=============

phage armband code base

Note dependency upon my branch of the NeoPixel library:

https://github.com/bunnie/Adafruit_NeoPixel

You must use this branch to get the loop length detection features to work.

Features implemented to date:

* button to switch mode of operation
  * debounced

* automatic loop length mesaurement
  * if no loop is detected, assumes max length of 112 pixels

* animations
  1. rainbow fade in/out ~25 mW/LED (scales with strip length)
  2. rainbow "walk" (just two LEDS at a time in rainbow mode) 125mW fixed (power does not scale with strip length)
  3. flashlight "wave" (two LEDs at 100% white brightness, cycling around the band) 350 mW fixed
  4. strobe mode (all LEDs strobe in random pattern) ~18mW/LED
