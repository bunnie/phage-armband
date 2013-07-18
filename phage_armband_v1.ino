#include <Adafruit_NeoPixel.h>

// reset pin is pin 1
#define WS2812_IN   3  // physical pin 2
#define ANAMIC_PIN  4  // physical pin 3; also arduino analog input 2
// ground, pin 4
#define AGC_PIN     0  // physical pin 5; pwm, aref, mosi
#define WS2812_OUT  1  // physical pin 6; pwm, miso
#define BEHAVE_PIN  2  // physical pin 7; analog input 1, SCK
// power, pin 8

// start with just one pixel in the strip, we assume we can auto-detect length
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, WS2812_OUT, NEO_GRB + NEO_KHZ800);

#define PIXMAX  (112)  // up to 1.8 meters of LEDs; may have to trim if we use more global variables...

////////////////////
/// MATH ROUTINES
/// help with manipulating pixels
////////////////////

// saturating subtract. returns a-b, stopping at 0. assumes 8-bit types
uint8_t satsub_8( uint8_t a, uint8_t b ) {
  if( a >= b )
    return (a - b);
  else
    return 0;
}

// saturating add, returns a+b, stopping at 255. assumes 8-bit types
uint8_t satadd_8( uint8_t a, uint8_t b ) {
  uint16_t c = (uint16_t) a + (uint16_t) b;

  if( c > 255 )
    return (uint8_t) 255;
  else
    return (uint8_t) (c & 0xFF);
}

// saturating subtract, acting on a whole RGB pixel
uint32_t satsub_8p( uint32_t c, uint8_t val ) {
  uint8_t r, g, b;
  r = (c >> 16) & 0xFF;
  g = (c >> 8) & 0xFF;
  b = (c & 0xFF);
  r = satsub_8( r, val );
  g = satsub_8( g, val );
  b = satsub_8( b, val );

  return( ((uint32_t) r) << 16 | ((uint32_t) g) << 8 | b );
}

// saturating add, acting on a whole RGB pixel
uint32_t satadd_8p( uint32_t c, uint8_t val ) {
  uint8_t r, g, b;
  r = (uint8_t) ((c >> 16) & 0xFF);
  g = (uint8_t) ((c >> 8) & 0xFF);
  b = (uint8_t) ((c & 0xFF));
  r = satadd_8( r, val );
  g = satadd_8( g, val );
  b = satadd_8( b, val );

  return( ((uint32_t) r) << 16 | ((uint32_t) g) << 8 | (uint32_t) b );
}

// alpha blend, scale the input color based on a value from 0-255. 255 is full-scale, 0 is black-out.
// uses fixed-point math.
uint32_t alphaPix( uint32_t c, uint8_t alpha ) {
  uint32_t r, g, b;
  r = ((c >> 16) & 0xFF) * alpha;
  g = ((c >> 8) & 0xFF) * alpha;
  b = (c & 0xFF) * alpha;

  r = r / 255;
  g = g / 255;
  b = b / 255;

  return( ((uint32_t) r) << 16 | ((uint32_t) g) << 8 | b );  
}


#define ST_LOW 0
#define ST_HIGH 1
#define ST_DEBOUNCE 2
uint8_t mode = 0;
uint8_t lastsw = ST_HIGH;
uint8_t state = ST_HIGH;
uint8_t falling = ST_HIGH;

// the follow variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
unsigned long time = 0;         // the last time the output pin was toggled

//////////////////
/// button loop. This should be polled once per main loop, and also polled
/// after every pixel show() command. Returns 1 if the button has just been pressed,
/// 0 if there is no status change to report. Typically, a statement like
///     if( buttonLoop() ) return; 
/// is sufficient to insert after show() to implement the polling inside your animation routine.
//////////////////

uint8_t buttonLoop() {
  uint8_t sw;
  uint8_t retval = 0;
  if( digitalRead(BEHAVE_PIN) == LOW )
    sw = ST_LOW;
  else 
    sw = ST_HIGH;

  if( (sw == ST_LOW) && (lastsw == ST_HIGH) ) {
    time = millis();
    state = ST_DEBOUNCE;
  }
  lastsw = sw;

  if( state == ST_DEBOUNCE ) {
    if( ((millis() - time) > 50) && (sw == ST_LOW) ) {
      state = ST_LOW;
    } 
    else if( sw == ST_HIGH ) {
      state = ST_HIGH;
    } 
    else {
      state = ST_DEBOUNCE;
    }
  } 
  else {
    if( sw == ST_HIGH )
      state = ST_HIGH;
    else
      state = ST_LOW;
  }

  if( (falling == ST_DEBOUNCE) && (state == ST_LOW) ) {
    mode = mode + 1; // modulus should be handled inside the main loop
    retval = 1;
  }
  falling = state;  

  return retval;
}

/////////////////////
/// setup routine, standard arduino libcall
/////////////////////
void setup() {
  pinMode( WS2812_IN, INPUT ); // configure feedback pin
  pinMode( BEHAVE_PIN, INPUT ); // change color mode
//  pinMode( ANAMIC_PIN, INPUT ); // analog input pin
  
  strip.setinpin(WS2812_IN);
  strip.begin();  // just initializes pins & stuff

  // sweep through all chain lengths until we've found the max length
  for( int i = 1; i <= PIXMAX; i++ ) {
    strip.setPixelColor(strip.numPixels()-1, 0xFFFFFF);  // set the color of the furthest-out pixel to white
    if(strip.measureLength() > 0 ) // we get a return value of about 5 when we've hit the end
      break; // if any high bits measured on the output, we've reached the max pixel length of the strip + 1
    strip.resize(i);
  }
  strip.resize(strip.numPixels() - 1 ); // we have to go over by one to mesure the true length of the chain
  return;

}

/////////////////////
/// main loop - remember that some of the animation routines are blocking and we're relying on
/// the implementor to play fair by calling the buttonLoop() and returning on status change to the main loop.
///
/// Don't forget to increment NUM_ANIMATIONS if we add more animation routines.
/////////////////////
#define NUM_ANIMATIONS 5

void loop() {
  buttonLoop();

  mode = mode % NUM_ANIMATIONS; // because the button manager doesn't care about the modulus

  switch( mode ) {
  case 0:
    rainbowCycle(30, 66, 1);  // 66 + depth 64 for individuals, 128 + depth 128 for strangelove
    break;
  case 1:
    wave_rainbow();  // good low power setting
    break;
  case 2:
    wave_white(); // higher power, higher brightness, sort of a "flashlight mode"
    break;
  case 3:  
    sparkle(0xFFFFFF, 25, 150); // for strobe effect
    break;
  case 4:
    vumeter(); // make LEDs go up and down according to music volume
    break;
    // when adding a case be sure to change the mode modulus above
  default:
    rainbowCycle(30, 66, 1);  // 66 + depth 64 for individuals, 128 + depth 128 for strangelove
  }     

}

#define AGC_INTERVAL 1000
#define SAMPLES 128
#define DC_VAL 511

////////
// sample_adc() should be called as often as possible
////////
#define SAMPLE_INTERVAL 2000  // average interval in microseconds
#define SAMPLE_DEPTH 32
#define MID_THRESH_H 538  // 5% over 512
#define MID_THRESH_L 486  // 5% below 512
unsigned long sampletime = 0;
uint16_t sampletime_target = SAMPLE_INTERVAL;
uint16_t samples[SAMPLE_DEPTH];
uint8_t sample_index = 0;
uint16_t last_sample_max = 512;
#define AVG_MAX 1  // 0 = don't average the max value over an interval
void sample_adc() {
  if( (micros() - sampletime) > sampletime_target ) {
    uint16_t temp = analogRead(A2) & 0x3FF;
    if( temp < MID_THRESH_L ) // "rectify" the signal, since 512 is the midpoint
       temp = 1023 - temp;
    if( temp < MID_THRESH_H ) // toss out the lower 10%, it's going to be noise/irrelevant
      temp = MID_THRESH_H;
    temp = temp - MID_THRESH_H; // now we have a rectified, low-noise filtered signal
    samples[sample_index] = temp;
    sample_index = (sample_index + 1) % SAMPLE_DEPTH;
    sampletime_target = micros() + random(SAMPLE_INTERVAL * 2); // randomize to dither aliasing artifacts
  }
  
  if( sample_index == 0 ) {
    bubblesort(samples);
#if AVG_MAX
    uint32_t avg = 0;
    for( int i = 0; i < SAMPLE_DEPTH / 2; i++ ) {
       avg += samples[i];
    }
    avg = avg / (SAMPLE_DEPTH / 2);
    last_sample_max = (uint16_t) avg;
#else
    last_sample_max = samples[0]; // just take the max from the sort
#endif
    interval_max();
  }
}

/// this is called by the sampling program every time a new sample set is updated
#define MAX_INTERVAL  2000   // in ms
uint16_t max_interval_value = 512;  // this is the output of the computation
unsigned long maxtime = 0;
uint16_t max_running_value = 0;  // this holds the intermediate value between calls
void interval_max() {
  if( millis() - maxtime > MAX_INTERVAL ) {
    maxtime = millis();
    // update the interval (long-term) value from the computed max value over the past interval
    max_interval_value = max_running_value; // this is the output of the function
    max_running_value = 0; // reset the search intermediate
  }
  // check if our running value needs an updated
  if( last_sample_max > max_running_value ) {
    max_running_value = last_sample_max; 
  }
}

void bubblesort(uint16_t *array) {
  uint8_t n = SAMPLE_DEPTH;
  uint16_t swap;
  for (uint8_t c = 0 ; c < ( n - 1 ); c++) {
    for (uint8_t d = 0 ; d < n - c - 1; d++)  {
      if (array[d] < array[d+1]) /* For increasing order use > */ {
        swap       = array[d];
        array[d]   = array[d+1];
        array[d+1] = swap;
      }
    }
  }
}

#define VU_COLOR_INTERVAL 10
#define BEAT_DEADTIME 100

#define SIMPLE_VU 1  // 1 = run the simplest VU setting that we know works
void vumeter() {
  analogReference(DEFAULT);
  pinMode(A2, INPUT);
  uint16_t j = 0;
  uint8_t divisor = 0;
  unsigned long time_beat = 0;
  
  uint16_t hotspot = 0;
  uint16_t nextspot = 1;
  uint8_t hotpix = 255;
  uint8_t nextpix = 128;
  uint32_t hotcolor = 0;
  uint32_t nextcolor = 0;
  
  strip.setBrightness(255);
  
  while(1) {
      sample_adc(); // this handles all the updating of samples and intervals
      uint32_t local_avg = 0;
      for( uint8_t i = 0; i < SAMPLE_DEPTH; i++ ) {
        local_avg += samples[i];
      }
      local_avg /= SAMPLE_DEPTH; // compute the average sample value, should be a value <512
      
      /// color update section
      divisor = (divisor + 1) % VU_COLOR_INTERVAL;
      if( divisor == 0 ) {
        // update colors
        j = (j + 1) % (256 * 5);
        for(uint16_t i=0; i< strip.numPixels(); i++) {
          if( (i == hotspot) || (i == nextspot) )
            strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255)); // start bright
          else
            strip.setPixelColor(i, alphaPix(Wheel(((i * 256 / strip.numPixels()) + j) & 255), 16)); // dim the rest of the strip
          
        }
        hotcolor = strip.getPixelColor(hotspot);
        nextcolor = strip.getPixelColor(nextspot);
      }

      strip.setPixelColor( hotspot, alphaPix(hotcolor, hotpix ) );
      strip.setPixelColor( nextspot, alphaPix(nextcolor, nextpix ) );    

      if( local_avg < 40 )
        local_avg = 12;
        
      if( local_avg > 255 ) local_avg = 255; 

      strip.setPixelColor( hotspot, alphaPix(strip.getPixelColor(hotspot), local_avg ) );
      strip.setPixelColor( nextspot, alphaPix(strip.getPixelColor(nextspot), local_avg ) );

      if( divisor == 0 ) {      
        // on each step, transfer one bit of energy from the hotspot to the next increment up
        hotpix = satsub_8( hotpix, 1 );
        nextpix = satadd_8( nextpix, 1 );     

        if( hotpix == 128 ) {
          hotspot = hotspot + 1;
          if( hotspot == strip.numPixels() )
            hotspot = 0;
            nextspot = nextspot + 1;
          if( nextspot == strip.numPixels() )
            nextspot = 0;

          hotpix = 255;
          nextpix = 128;
        }    
      }
      strip.show();
      if( buttonLoop() ) return;  
#if 0      
      /// intensity update section
#if SIMPLE_VU
      if( local_avg < 40 )
        local_avg = 12;
        
      if( local_avg > 255 ) local_avg = 200; // save power and eyes, go to 200
      strip.setBrightness(local_avg);
      strip.show();
      if( buttonLoop() )  return;   
#else
      strip.setBrightness(max_interval_value / 2);
//      strip.setBrightness(last_sample_max / 2);
      strip.show();
      if( buttonLoop() )  return;       
#endif
#endif
  }
}
/////////////////////////////
//// SPARKLE - stroboscopic effect
/////////////////////////////
void sparkle(uint32_t c, uint16_t time_on, uint16_t time_off) {
  uint16_t target;

  strip.setBrightness(255);  // restore brightness setting

  target = (uint16_t) random(strip.numPixels());
  for( uint16_t i = 0; i < strip.numPixels(); i++ ) {
    if( random(strip.numPixels()) < (strip.numPixels() / 4) )
      strip.setPixelColor(i, c);
    else
      strip.setPixelColor(i, 0);
  }
  strip.show();
  if( buttonLoop() )  return;
  delay( random(time_on) + 5 ); 
  for( uint16_t i = 0; i < strip.numPixels(); i++ ) {
    strip.setPixelColor(i, 0);
  }
  strip.show();
  if( buttonLoop() )  return;
  delay( random(time_off) + time_off / 5 );
}

////////////////////////////
// this is a full-bright white wave that moves gracefully from pixel to pixel
////////////////////////////
void wave_white() {
  uint16_t hotspot = 0;
  uint16_t nextspot = 1;
  uint32_t hotpix;
  uint32_t nextpix;

  strip.setBrightness(255);  // restore brightness setting
  // first clear the array
  for( uint16_t i = 0; i < strip.numPixels(); i++ ) {
    strip.setPixelColor(i, 0);
  }
  strip.setPixelColor(hotspot, 0xFFFFFF); // set an initial "seed" color

  while(1) {
    // on each step, transfer one bit of energy from the hotspot to the next increment up
    hotpix = strip.getPixelColor(hotspot);
    nextpix = strip.getPixelColor(nextspot);
    hotpix = satsub_8p( hotpix, 1 );
    nextpix = satadd_8p( nextpix, 1 );
    strip.setPixelColor( hotspot, hotpix );
    strip.setPixelColor( nextspot, nextpix );
    if( hotpix == 0 ) {
      hotspot = nextspot;
      nextspot = (nextspot + 1) % strip.numPixels();
    }
    strip.show();
    if( buttonLoop() )  return;     
    delay(1);
  }
}

///////////////////////////
// this is a nice, power-saving but pretty animation
// may want to tweak to give more active pixels if the strip is very long
///////////////////////////
#define MAX_WAVE_RAINBOW 165
void wave_rainbow() {
  uint16_t hotspot = 0;
  uint16_t nextspot = 1;
  uint8_t hotpix = MAX_WAVE_RAINBOW;
  uint8_t nextpix = 0;
  uint16_t j;
  j = 0;
  uint8_t colorrate = 0;
  
  strip.setBrightness(255);  // restore brightness setting  
  while(1) {

    for(uint16_t i=0; i< strip.numPixels(); i++) {
      if( (i == hotspot) || (i == nextspot) )
        strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
      else
        strip.setPixelColor(i, 0);
    }
    colorrate ++;
    if( (colorrate % 7) == 0 ) {
      j++;
      if( j == (256 * 5) ) {
        j = 0;
      }
    }

    strip.setPixelColor( hotspot, alphaPix(strip.getPixelColor(hotspot), hotpix) );
    strip.setPixelColor( nextspot, alphaPix(strip.getPixelColor( nextspot), nextpix ) );

    // on each step, transfer one bit of energy from the hotspot to the next increment up
    hotpix = satsub_8( hotpix, 1 );
    nextpix = satadd_8( nextpix, 1 );     

    if( hotpix == 0 ) {
      hotspot = hotspot + 1;
      if( hotspot == strip.numPixels() )
        hotspot = 0;
      nextspot = nextspot + 1;
      if( nextspot == strip.numPixels() )
        nextspot = 0;

      hotpix = MAX_WAVE_RAINBOW;
      nextpix = 0;
    }
    strip.show();
    if( buttonLoop() )  return;   
    delay(1);
  }
}

/////////////////////////////
//// makes a rainbow pattern that fades in and out based on a modulation depth, and whose rate
//// depends on the number of "friends" present, as rated on a scale of 1-8
//// "primary" mode of operation for locator badges
/////////////////////////////
#define MOD_DEPTH 64  // 128 for strangelove

void rainbowCycle(uint8_t wait, uint8_t brightness, uint8_t friends) {
  uint16_t i, j;
  uint8_t bright = brightness;
  uint8_t dir = 1;
  uint8_t rate = 1;
  if( friends > 10 )  
    friends = 10; // limit our excitedness so we still have an effect
  if( friends == 0 )
    friends = 1;  // we at least have ourself

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }

    strip.setBrightness(bright);
    if( dir ) {
      bright = satadd_8(bright, rate);
      if( bright > brightness )
        rate = friends;

      if( (bright == 255) || (bright >= (brightness + MOD_DEPTH))  ) {
        dir = 0;
      }
    } 
    else {
      bright = satsub_8(bright, rate);
      if( bright < brightness )
        rate = 1;
      if( (bright == 0) || (bright <= (brightness - MOD_DEPTH)) ) {
        dir = 1;
      }
    }  
    strip.show();
    if( buttonLoop() )  return;

    delay(wait / friends);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } 
  else if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } 
  else {
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}



