#include <TVout.h>
#include <fontALL.h>
#include <math.h>
#include <string.h>
#include <vector>
#include <time.h>       /* time */
#include <Controllers.h>
#include "vsynth.h"


using namespace std;

#define FMAX    1200
#define FMIN    10
#define WAVES   7
#define NUM_WAVES 8
#define NUM_KNOBS 10
#define SAMPLES 16001*2

#define set_bit(v,b) v |= _BV(b)
#define clear_bit(v,b) v &= ~_BV(b)
#define pulse_bit(v,b) do { set_bit(v,b); clear_bit(v,b); } while(0)
#define pulse_bit_low(v,b) do { clear_bit(v,b); set_bit(v,b); } while(0)

#define PASTE(x,y) x ## y
#define PORT(x) PASTE(PORT,x)
#define PIN(x) PASTE(PIN,x)
#define DDR(x) PASTE(DDR,x)

// Low pass filter

#define SF10 (10)
#define SCALE10 (2^SF10)
#define DT (0.01f) // time, was 0.01 originally
#define TAU (0.9f) // time constant in sec/rad, was 0.9 originally
                    // 0.9  = distorts at 70  hz, vol = 62
                    // 0.45 = distorts at 180 hz
                    // 0.12 = distorts at 440 hz
#define K ((float)DT/((float)TAU+(float)DT))
#define K_SCALE10 (K*(float)SCALE10) // Note this is 102, truncation of 0.4 doesn't affect filter really
#define FILTER_IC (long)(0.0*SCALE10) // Note this is 0 as a long

void mixbuffers(float,float);
unsigned int viewWidth=128, viewHeight=96;
TVout TV;
typedef unsigned int Uint16 ;
typedef int Int16 ;

const char *potnames[] = {"OSC_1",
                            " VOL ",
                            " FRQ ",
                            "ATTAC",
                            "DECAY",
                            "OSC_2",
                            " AMP ",
                            " FRQ ",
                            "SUSTA",
                            "RELEA"};
#define POTRAD 6

// parameters
uint16_t pitch = 200; // set pitch
uint16_t lfofreq = 0; // lfo period
uint16_t lfodepth = 0; // lfo wave amplitude
uint8_t envfreq = 0; // envelope period
volatile int8_t wavenum = 8; // oscillator waveform
uint8_t lfowavenum = 8; // lfo waveform
uint8_t vol = 0xff; // main osc volume

// state
uint16_t outpitch = 0; // OCR1A value
uint8_t lfotimer = 0; // lfo value, counts from 0 to lfofreq

char ADSR = false; char filter = false;
char osc2_mode = 0;
uint16_t Attack=SAMPLES/16, Decay=SAMPLES/16, Sustain=SAMPLES/4, Release=SAMPLES/16;

// my stuff
float px1 = viewWidth/4, py1 = viewHeight/4,pa=0;
float px2 = viewWidth/2, py2 = viewHeight/4;
int currentPot =0;


// low pass filter
int lag1order( int x, long last_y, float tau, int amp )
{
long x_scaled;
float temp;
static long y_scaled;

temp = K;
temp = K_SCALE10;
y_scaled = last_y;
x_scaled = x*SCALE10;
temp = x_scaled - y_scaled;
temp = temp * ((float)DT/((float)tau+(float)DT))* (float)SCALE10;
temp = temp / SCALE10;
y_scaled = y_scaled + temp;
return(y_scaled*amp/SCALE10);
}


int getInput() {

      if (Controller.firePressed()) {
            if (currentPot == 3 || currentPot == 4 || currentPot == 8 || currentPot == 9) ADSR = !ADSR;
            if (currentPot == 0 || currentPot == 1 || currentPot == 2) filter = !filter;
            if (currentPot == 5) osc2_mode ++;
            if (osc2_mode == 4) osc2_mode =0;
            return 1;
      }
      if (Controller.upPressed()) {
        increaseCurrentPot();
        return 1;
      }

      if (Controller.downPressed()) {
        decreaseCurrentPot();
        return 1;
      }

      if (Controller.leftPressed()) {
        currentPot--;
        if (currentPot<0) currentPot = NUM_KNOBS;
        return 1;
      }

      if (Controller.rightPressed()) {
        currentPot++;
        if (currentPot>NUM_KNOBS) currentPot = 0;
        return 1;
      }
      return 0;
}


/*
void mixbuffers(float pitch1, float pitch2){
    //if (osc2_mode == 1) lfodepth = 255;
    double iLength;
    Uint16 osc_add1, osc_add2, osc_ticks=0,phaser=10,phaserCount= 0, phaserToggle = 0,
    osc_ticks2,repeats=0, repeats2=0;
    Int16 dResult, noise, phaserDelta=1;
    sf::Sound testsnd;
    sf::SoundBuffer testbuf;
    long lastoutput; // for low pass filter
    iLength = SAMPLES;// 2 sec at 8khz samples
    Int16 sbuf[SAMPLES]; // osc 1 results
    Int16 sbuf2[SAMPLES]; // osc 2 results
    osc_ticks = 8000 / pitch1; // how many ticks per cycle
    if (pitch2) osc_ticks2 = 32000 / pitch2; // lfo, was 8000/pitch

    phaserDelta = pitch1 / 50 + 1;

    repeats = SAMPLES / osc_ticks;
    if (repeats % 2) repeats--;
    iLength = osc_ticks*repeats; // make sample length fit the increment
    osc_add1 = 65536/osc_ticks;
    if (pitch2) osc_add2 = 65536/osc_ticks2;
    if (pitch2) repeats2 = iLength / osc_ticks2;

    std::vector<sf::Int16> FinalSamplesVector;
    FinalSamplesVector.reserve(iLength);

    noise =  rand() % 65536 - 32767;
    // CALCULATE OSCILLATOR 1
    for(uint32_t i = 0; i < repeats; i++) // the length of the entire sample
    {
            for (uint32_t j=0; j<osc_ticks; j++) {
            if (wavenum == 8) { // off
                dResult = 0;
            }

            if (wavenum == 0) { // square
                if (j < osc_ticks / 2) {
                //first half of cycle
                dResult = 32767;
                } else {
                //2nd half of cycle
                dResult = -32767;
                }
                if (dResult == 0) {
                dResult++;
                }
            }

            if (wavenum == 1) { // triangle
                if (j < osc_ticks / 2) {
                //first half of cycle
                dResult = j*osc_add1*2 -32767;
                } else {
                //2nd half of cycle
                dResult = 32767-(j-osc_ticks/2)*osc_add1*2;
                }
            }

            if (wavenum==2) { // sawtooth
                dResult = j*osc_add1;
            }

            if (wavenum==3) { // inverted sawtooth
                dResult = 32767 - j*osc_add1;
            }

            if (wavenum == 4) { // 1/4 square
                if (j < osc_ticks / 4) {
                //first half of cycle
                dResult = 32767;
                } else {
                //2nd half of cycle
                dResult = - 32767;
                }
            }

            if (wavenum == 5) { // 3/4 square
                if (j < osc_ticks * 3 / 4) {
                //first half of cycle
                dResult = 32767;
                } else {
                //2nd half of cycle
                dResult = - 32767;
                }
            }

            if (wavenum == 6) { // noise
                if (j < osc_ticks / 2) {
                //first half of cycle
                dResult = noise;
                } else {
                //2nd half of cycle
                dResult = -noise;
                }
            }

            if (wavenum == 7) { // phaser
              phaserCount++;
              if (phaserToggle) dResult = 32767 * vol/255;
              if (!phaserToggle) dResult = -32767 * vol/255;
              if (phaserCount>phaser) {
                phaserToggle = !phaserToggle;
                phaser += phaserDelta;
                phaserCount =0;
                if (phaser > 100 || phaser < 10) phaserDelta *= -1;
              }

            }


            if ((j+1) % osc_ticks == 0) {
             noise =  rand() % 65536 - 32767;
            }
            sbuf[i*osc_ticks+j] = dResult;
        }
    }

    noise = rand() % 65536 - 32767;
    // CALCULATE OSCILLATOR 2
if (pitch2){
    phaserDelta = pitch2 / 50 + 1; phaserCount = 0; phaser = 10;
    for(uint32_t i = 0; i < repeats2+2; i++) // the length of the entire sample
    {
            for (uint32_t j=0; j<osc_ticks2; j++) {
            if (lfowavenum == 8) { // off
                dResult = 0;
            }

            if (lfowavenum == 0) { // square
                if (j < osc_ticks2 / 2) {
                //first half of cycle
                dResult = 32767;
                } else {
                //2nd half of cycle
                dResult = -32767;
                }
                if (dResult == 0) {
                dResult++;
                }
            }

            if (lfowavenum == 1) { // triangle
                if (j < osc_ticks2 / 2) {
                //first half of cycle
                dResult = j*osc_add2*2 -32767;
                } else {
                //2nd half of cycle
                dResult = 32767-(j-osc_ticks2/2)*osc_add2*2;
                }
            }

            if (lfowavenum==2) { // sawtooth
                dResult = j*osc_add2;
            }

            if (lfowavenum==3) { // inverted sawtooth
                dResult = 32767 - j*osc_add2;
            }

            if (lfowavenum == 4) { // 1/4 square
                if (j < osc_ticks2 / 4) {
                //first half of cycle
                dResult = 32767;
                } else {
                //2nd half of cycle
                dResult = - 32767;
                }
            }

            if (lfowavenum == 5) { // 3/4 square
                if (j < osc_ticks2 * 3 / 4) {
                //first half of cycle
                dResult = 32767;
                } else {
                //2nd half of cycle
                dResult = - 32767;
                }
            }

            if (lfowavenum == 6) { // noise
                if (j < osc_ticks2 / 2) {
                //first half of cycle
                dResult = noise;
                } else {
                //2nd half of cycle
                dResult = -noise;
                }
            }

            if (lfowavenum == 7) { // phaser
              phaserCount++;
              if (phaserToggle) dResult = 32767 * lfodepth/255;
              if (!phaserToggle) dResult = -32767 * lfodepth/255;
              if (phaserCount>phaser) {
                phaserToggle = !phaserToggle;
                phaser += phaserDelta;
                phaserCount =0;
                if (phaser > 100 || phaser < 10) phaserDelta *= -1;
              }
            }

            if ((j+1) % osc_ticks2 == 0) {
             noise =  rand() % 65536 - 32767;
            }

            if (i*osc_ticks2+j<iLength) sbuf2[i*osc_ticks2+j] = dResult;
        }
    }
} else {
    for (uint32_t i = 0; i< iLength; i++) sbuf2[i]=0;
    }

    // Add TOGETHER OSC1 AND OSC2

    Uint16 c,skipper=0;
    for(int i = 0; i < iLength; i++)
    {
    Int16 a,b;
    if (osc2_mode == 0) {
        // simple addition mode = average of 2 samples
        a = (sbuf[i]*vol)/255; b =(sbuf2[i]*lfodepth)/255;
        dResult = a+b/2;
    }
    if (osc2_mode == 1) {
        // FM modulation mode

        if (skipper == 0) {
            c = (sbuf2[i]*lfodepth)/255+32767;
            c = c/256/8; // was c/256/8
            if (c==0) c=1;
            skipper = i+osc_ticks/c; // length = osc ticks if c = 0
            dResult = (sbuf[i]*vol)/255;
        }
        if (i==skipper) {
            i++; skipper = 0;
            dResult = (sbuf[i]*vol)/255;
        }
    }

    if (osc2_mode == 2) {
        // AM modulation mode
        a = (sbuf[i]*vol)/255; b =(sbuf2[i]*lfodepth)/255;
        dResult = a*b/255;
    }

    if (osc2_mode == 3) {
        // Filter modulation mode
        a = (sbuf[i]*vol)/255; b =(sbuf2[i]*lfodepth)/255;
        if (i == 0) {
        lastoutput = a;
        } else {
        dResult = lag1order(a, lastoutput, TAU*(float)b/256, b/32 );
        lastoutput = dResult;
        }
    }

    if (ADSR) {
        if (i<=Attack) {
        dResult = dResult*i/Attack;
        }
        if (i>Attack && i <= Attack+Decay) {
        dResult = dResult-dResult*2/3*(i-Attack)/Decay;
        }
        if (i>Attack+Decay && i <= Attack+Decay+Sustain)
        {
        dResult>>=1;
        }
        if (i>Attack+Decay+Sustain) {
        dResult = dResult*2/3 - dResult*2/3*(i-Attack-Decay-Sustain)/Release;
        }
        if (i>=Attack+Decay+Sustain+Release) {
        dResult = 0;
        }
    }

    if (filter) {
    // Low pass filter
        if (i == 0) {
        lastoutput = 0;
        } else {
        dResult = lag1order(dResult, lastoutput, TAU, 8);
        lastoutput = dResult;
        }
    }

    FinalSamplesVector.push_back(static_cast<sf::Int16>(dResult));
    }

    int foo = testbuf.loadFromSamples(&FinalSamplesVector[0], FinalSamplesVector.size(), 1, 8000);
    int bar = testbuf.saveToFile("output.wav");
    testsnd.setBuffer(testbuf);
    testsnd.setLoop(true);
    testsnd.play();
    while (!getInput()) {
    }
    updatePots();
    TV.delay(100);
}*/

void drawPot(int x,int y,float potvalue,int minval, int maxval,int potnum) {
    char name[20];
    char numchars=1;
    int dispvalue = (int) potvalue;
    strcpy(name, potnames[potnum]);
    potvalue = (float)potvalue - minval;
    potvalue = (float)potvalue/maxval;
    potvalue = (float)potvalue*2*PI;
    TV.draw_circle(x,y,POTRAD+2,0,0); // unselect
    if (currentPot == potnum) TV.draw_circle(x,y,POTRAD+2,1,0); // selected
    TV.draw_circle(x,y,POTRAD,1,0); // knob itsel
    TV.draw_line(x,y,x+cos(potvalue-PI/2)*POTRAD,y+sin(potvalue-PI/2)*POTRAD,1);
    if (dispvalue>9) numchars=2;
    if (dispvalue>99) numchars=3;
    TV.print(x-6/2*4-1,y-POTRAD*2-5,"       ");
    if (potnum == 5 || potnum == 0) {
    int tempwavenum = wavenum;
    if (potnum == 5) {
        tempwavenum=lfowavenum;
        if (osc2_mode == 1) strcpy(name, "FM MOD");
        if (osc2_mode == 2) strcpy(name, "AM MOD");
        if (osc2_mode == 3) strcpy(name, "FILTER ");
        }
    switch (tempwavenum) {
        case 0:
        TV.print(x-6/2*4-1,y-POTRAD*2-5,"square");
        break;
        case 1:
        TV.print(x-6/2*4-1,y-POTRAD*2-5,"triang");
        break;
        case 2:
        TV.print(x-6/2*4-1,y-POTRAD*2-5,"sawtot");
        break;
        case 3:
        TV.print(x-6/2*4-1,y-POTRAD*2-5,"invsaw");
        break;
        case 4:
        TV.print(x-6/2*4-1,y-POTRAD*2-5,"1/4 sq");
        break;
        case 5:
        TV.print(x-6/2*4-1,y-POTRAD*2-5,"3/4 sq");
        break;
        case 6:
        TV.print(x-6/2*4-1,y-POTRAD*2-5,"noise");
        break;
        case 7:
        TV.print(x-6/2*4-1,y-POTRAD*2-5,"phaser");
        break;
        case 8:
        TV.print(x-6/2*4-1,y-POTRAD*2-5," off ");
        break;

    }
    } else {
    TV.print(x-numchars/2*4-1,y-POTRAD*2-5,dispvalue);
    }

    TV.print(x-(sizeof(potnames[potnum])+1)/2*4-3,y+POTRAD+5,name);
}


void increaseCurrentPot(){
    switch (currentPot) {
    case 5:
    if (lfowavenum < NUM_WAVES) { lfowavenum++; } else {lfowavenum=0;}
        break;
    case 8:
    if (Sustain < SAMPLES/4-256) { Sustain += 256; } else {Sustain = SAMPLES/4;}
        break;
    case 6:
    if (lfodepth < 255-8) { lfodepth += 8 ; } else {lfodepth = 0;}
        break;
    case 7:
    if (lfofreq < 200) { lfofreq ++ ; } else {lfofreq = 200;}
        break;
    case 2:
    if (pitch < FMAX-10) { pitch+= 10 ; } else {pitch = FMAX;}
        break;
    case 1:
    if (vol < 255-5) { vol+= 5 ; } else {vol = 255;}
        break;
    case 9:
    if (Release < SAMPLES/4-256) { Release += 256; } else {Release = SAMPLES/4;}
        break;
    case 0:
    if (wavenum < NUM_WAVES) { wavenum++; } else { wavenum=0; }
        break;
    case 3:
    if (Attack < SAMPLES/4-256) { Attack += 256; } else {Attack = SAMPLES/4;}
        break;
    case 4:
    if (Decay < SAMPLES/4-256) { Decay += 256; } else {Decay = SAMPLES/4;}
        break;
    default:
        break;
    }
}

void decreaseCurrentPot(){
    switch (currentPot) {
    case 5:
    if (lfowavenum > 0) { lfowavenum--; } else {lfowavenum=NUM_WAVES;}
        break;
    case 8:
    if (Sustain > 256) { Sustain -= 255; } else {Sustain = 1;}
        break;
    case 6:
    if (lfodepth < 8) { lfodepth = 255 ; } else {lfodepth -= 8;}
        break;
    case 7:
    if (lfofreq > 0) { lfofreq -- ; } else {lfofreq = 0;}
        break;
    case 2:
    if (pitch > FMIN) { pitch-= 10 ; } else {pitch = FMIN;}
        break;
    case 1:
    if (vol > 5) { vol-= 5 ; } else {vol = 0;}
        break;
    case 9:
    if (Release > 256) { Release -= 255; } else {Release = 1;}
        break;
    case 0:
    if (wavenum > 0) { wavenum--; } else { wavenum=NUM_WAVES; }
        break;
    case 3:
    if (Attack > 256) { Attack -= 255; } else {Attack = 1;}
        break;
    case 4:
    if (Decay > 256) { Decay -= 255; } else {Decay = 1;}
        break;
    default:
        break;
    }
}


void updatePots() {

    // row 1
    drawPot(7+POTRAD+0*viewWidth/5,viewHeight/3,wavenum,0,9,0);
    drawPot(7+POTRAD+1*viewWidth/5,viewHeight/3,vol,0,0xff,1);
    drawPot(7+POTRAD+2*viewWidth/5,viewHeight/3,pitch,FMIN,FMAX,2);
    drawPot(7+POTRAD+3*viewWidth/5,viewHeight/3,Attack,0,SAMPLES/4,3);
    drawPot(7+POTRAD+4*viewWidth/5,viewHeight/3,Decay,0,SAMPLES/4,4);

    // row 2
    drawPot(7+POTRAD+0*viewWidth/5,2*viewHeight/3+10,lfowavenum,0,9,5);
    drawPot(7+POTRAD+1*viewWidth/5,2*viewHeight/3+10,lfodepth,0,255,6);
    drawPot(7+POTRAD+2*viewWidth/5,2*viewHeight/3+10,lfofreq,0,200,7);
    drawPot(7+POTRAD+3*viewWidth/5,2*viewHeight/3+10,Sustain,0,SAMPLES/4,8);
    drawPot(7+POTRAD+4*viewWidth/5,2*viewHeight/3+10,Release,0,SAMPLES/4,9);

    if (ADSR) TV.print(7+POTRAD+3*viewWidth/5,5,"ADSR ON ");
    if (!ADSR) TV.print(7+POTRAD+3*viewWidth/5,5,"ADSR OFF");
    if (filter) TV.print(7+POTRAD+0*viewWidth/5,5,"LO-PASS ON ");
    if (!filter) TV.print(7+POTRAD+0*viewWidth/5,5,"LO-PASS OFF");

}

void setup() {
  //srand (time(NULL));
  TV.begin(PAL,viewWidth,viewHeight);
  TV.select_font(font4x6);
  updatePots();
  TV.delay(1);
  TV.set_hbi_hook(setsynth()); // this is where we set the hook 
  synthesize(0xff,0); // turn on fast PWM signal 
}

void loop() {
  //mixbuffers(pitch,lfofreq);
  //setPitch(vol);
  duty=vol;
  getInput();
  updatePots();
  TV.delay(10);
  
}


