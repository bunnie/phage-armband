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

//#define PIXMAX  (112)  // up to 1.8 meters of LEDs; may have to trim if we use more global variables...
#define PIXMAX 90

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

  strip.setBrightness(1); // reduce brightness for this test

  // sweep through all chain lengths until we've found the max length
  for( int i = 1; i <= PIXMAX; i++ ) {
    strip.setPixelColor(strip.numPixels()-1, 0xFFFFFF);  // set the color of the furthest-out pixel to white
    if(strip.measureLength() > 0 ) // we get a return value of about 5 when we've hit the end
      break; // if any high bits measured on the output, we've reached the max pixel length of the strip + 1
    strip.resize(i);
  }
  strip.resize(strip.numPixels() - 1 ); // we have to go over by one to mesure the true length of the chain
//  strip.resize(30 ); // we have to go over by one to mesure the true length of the chain
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
    wave_rainbow(0);  // good low power setting
    //rainbowCycle(30, 66, 1);  // 66 + depth 64 for individuals, 128 + depth 128 for strangelove
    break;
  case 1:
    vumeter2(1);
    break;
  case 2:
    wave_rainbow(1); // use a flag to convert rainbow to white
    break;
  case 3:  
    vumeter2(0); // make LEDs go up and down according to music volume
    break;
  case 4:
    sparkle(0xFFFFFF, 25, 150); // for strobe effect
    break;
    // when adding a case be sure to change the mode modulus above
  default:
    vumeter2(0);
    //rainbowCycle(30, 66, 1);  // 66 + depth 64 for individuals, 128 + depth 128 for strangelove
  }     

}


// this function attempts to determine the average noise level

#define AVG_SAMPLE_INTERVAL 20000  // average interval in microseconds
#define SAMP_AVG_DEPTH 64  // this is eating 128 bytes out of 512!
uint16_t avg_samples[SAMP_AVG_DEPTH];
uint8_t avg_sample_index = 0;
unsigned long avg_sampletime = 0;
unsigned long avg_sampletime_target = AVG_SAMPLE_INTERVAL;
#define MID_THRESH_H 665  // 30% over 512
#define MID_THRESH_L 358  // 30% below 512
uint16_t avg_value = 0;

void sample_average() {
  if( (micros() - avg_sampletime) > avg_sampletime_target ) {
    uint16_t temp = analogRead(A2) & 0x3FF;
    if( temp < MID_THRESH_L ) // "rectify" the signal, since 512 is the midpoint
      temp = 1023 - temp;
    if( temp < MID_THRESH_H ) // toss out the lower 10%, it's going to be noise/irrelevant
      temp = MID_THRESH_H;
    temp = temp - MID_THRESH_H; // now we have a rectified, low-noise filtered signal
    avg_samples[avg_sample_index] = temp;
    avg_sample_index = (avg_sample_index + 1) % SAMP_AVG_DEPTH;
    avg_sampletime_target = micros() + random(AVG_SAMPLE_INTERVAL * 2); // randomize to dither aliasing artifacts
    
    uint32_t local_avg = 0;
    for( int i = 0; i < SAMP_AVG_DEPTH; i++ ) {
      local_avg += avg_samples[i];
    }
    avg_value = (uint16_t) (local_avg / SAMP_AVG_DEPTH);
  }
}

#define VU_X_PERIOD 3.0   // number of waves across the entire band
#define VU_T_PERIOD 2500  // time to complete 2pi rotation, in integer milliseconds
void vumeter2(uint8_t mode) {
  uint32_t c;
  uint8_t t;
  uint32_t reftime;
  uint32_t curtime;
  strip.setBrightness(255);
  reftime = millis();
  uint16_t j = 0;
  uint16_t colorrate = 0;
  uint32_t offset = 0;
  float sign = 1.0;
  
  if( mode != 0 )
    sign = -1.0;
    
  while(1) {
    curtime = millis() + offset;
    if( (curtime - reftime) > VU_T_PERIOD )
       reftime = curtime;
       
    int rate = 0;
    sample_average(); 
    if( mode == 0 )
      rate = map(avg_value, 0, 25, 0, 100);
    else
      rate = map(avg_value, 0, 8, 0, 100);
      
    offset += rate;
    if( offset > 0x80000000) {
      offset = 0;
      curtime = millis();
      reftime = curtime;
    }
    for( uint16_t i = 0; i < strip.numPixels(); i++ ) {
      c = (uint8_t) (sin( (((float) i / ((float)strip.numPixels() - 1.0)) * VU_X_PERIOD +
         sign * (((float)(curtime - reftime)) / ((float) VU_T_PERIOD)))  * 6.283185 ) * 100.0 + 110.0);
      c = c * c;
      c = (c >> 8) & 0xFF;
      strip.setPixelColor(i, alphaPix(Wheel(((i * 256 / strip.numPixels()) + j) & 255), c));
      colorrate ++;
      if( (colorrate % 7) == 0 ) {
        j++;
        if( j == (256 * 5) ) {
          j = 0;
        }
      }
    }  
    strip.show();
    if( buttonLoop() ) return;
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
    if( random(strip.numPixels()) < (strip.numPixels() / 3) )
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

///////////////////////////
// this is a nice, power-saving but pretty animation
// may want to tweak to give more active pixels if the strip is very long
///////////////////////////
#define MAX_WAVE_RAINBOW 165
void wave_rainbow(uint8_t white) {
  uint16_t hotspot = 0;
  uint16_t nextspot = 1;
  uint8_t hotpix = MAX_WAVE_RAINBOW;
  uint8_t nextpix = 0;
  
  uint16_t r_hotspot = strip.numPixels() - 1;
  uint16_t r_nextspot = strip.numPixels() - 2;
  uint8_t r_hotpix = MAX_WAVE_RAINBOW;
  uint8_t r_nextpix = 0;
  
  uint16_t j;
  j = 0;
  uint8_t colorrate = 0;
  
  if(white) {
    uint8_t hotpix = 255;
    uint8_t r_hotpix = 255;
  }
  
  strip.setBrightness(255);  // restore brightness setting  
  while(1) {

    for(uint16_t i=0; i< strip.numPixels(); i++) {
      if( (i == hotspot) || (i == nextspot) || (i == r_hotspot) || (i == r_nextspot) ) {
        if( white )
           strip.setPixelColor(i, 0xffffff);
        else
           strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
      } else
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
    strip.setPixelColor( r_hotspot, alphaPix(strip.getPixelColor(r_hotspot), r_hotpix) );
    strip.setPixelColor( r_nextspot, alphaPix(strip.getPixelColor( r_nextspot), r_nextpix ) );

    // on each step, transfer one bit of energy from the hotspot to the next increment up
    hotpix = satsub_8( hotpix, 6 );
    nextpix = satadd_8( nextpix, 6 );     

    r_hotpix = satsub_8( r_hotpix, 6 );
    r_nextpix = satadd_8( r_nextpix, 6 );     

    if( hotpix == 0 ) {
      hotspot = hotspot + 1;
      if( hotspot == strip.numPixels() )
        hotspot = 0;
      nextspot = nextspot + 1;
      if( nextspot == strip.numPixels() )
        nextspot = 0;
      if(white)
        hotpix = 255;
      else
        hotpix = MAX_WAVE_RAINBOW;
      nextpix = 0;
    }
    
    if( r_hotpix == 0 ) {
      r_hotspot = r_hotspot - 1;
      if( r_hotspot == 0 )
        r_hotspot = strip.numPixels() - 1;
      r_nextspot = r_nextspot - 1;
      if( r_nextspot == 0 )
        r_nextspot = strip.numPixels() - 1;

      if(white)
        r_hotpix = 255;
      else
        r_hotpix = MAX_WAVE_RAINBOW;
      r_nextpix = 0;
    }

    strip.show();
    if( buttonLoop() )  return;   
//    delay(1);
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



