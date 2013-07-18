phage-armband
=============

phage armband code base

Features implemented to date:

+ button to switch mode of operation
*   debounced
+ automatic loop length mesaurement
*   if no loop is detected, assumes max length of 112 pixels
+ animations:
*   rainbow fade in/out ~25 mW/LED (scales with strip length)
*   rainbow "walk" (just two LEDS at a time in rainbow mode) 125mW fixed (power does not scale with strip length)
*   flashlight "wave" (two LEDs at 100% white brightness, cycling around the band) 350 mW fixed
*   strobe mode (all LEDs strobe in random pattern) ~18mW/LED
