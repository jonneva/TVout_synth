#include "TVout.h"
#include "vsynth.h"

unsigned char duty=0xff;

void synth_hook() {
// test function for the hbi hook  
  OCR2A = duty;
}

pt2Funct setsynth(){
  // returns address to the function that will be called on hbi_hook
  return &synth_hook;
}

void synthesize(unsigned char duty_start, unsigned int duration_ms) {
  // the function that will intialize the sound 
  

#define TIMER 2

	//this is init code

	TCCR2A = 0;
	TCCR2B = 0;
	//TCCR2A |= _BV(WGM21); // TCCR2A = timer control register, WGM 21 = CTC mode, loop to OCRA 
	//TCCR2B |= _BV(CS20); // TCCR2B = Timer/Counter Control Register B, CS20 = 1 = no prescaling
        /*TCCR2A = _BV(COM2A1) | _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
        TCCR2B = _BV(CS22);
        OCR2A = 180;
        OCR2B = 50;*/
        //---------------------------------------------------------------------
        // TCCR2A settings
        //---------------------------------------------------------------------
        // Arduino pins 11 and 3 are controlled by TCCR2B, audio = pin 11
        // If you use the default values set by the Arduino Diecimila's bootloader, these are your PWM frequencies:
        // Arduino Pins 5 and 6: 1kHz
        // Arduino Pins 9, 10, 11, and 3: 500Hz
        
        // PRESCALER
        // A prescaler dictates the speed of your timer according the the following equation:
        // (timer speed (Hz)) = (Arduino clock speed (16MHz)) / prescaler
        // example: TCCR2B |= (1 << CS22);  // Set CS#2 bit for 64 prescaler for timer 2
        // 
        // Now you can calculate the interrupt frequency with the following equation:
        // interrupt frequency (Hz) = (Arduino clock speed 16,000,000Hz) / (prescaler * (compare match register + 1))
        //
        // so, to get 62500 = 16000000 / prescaler * 256
        // prescaler = 16 000 000 / 256 / 62500 = 1 !!!!
        
        // First clear all three prescaler bits:
        int prescalerVal = 0x07; // create a variable called prescalerVal and set it equal to the binary number "00000111"
        TCCR2B &= ~prescalerVal; // AND the value in TCCR0B with binary number "11111000"
        
        // These bits control the Output Compare pin (OC2A) behavior. If one or
        // both of the COM2A1:0 bits are set, the OC2A output overrides the
        // normal port functionality of the I/O pin it is connected to.
        // However, note that the Data Direction Register (DDR) bit
        // corresponding to the OC2A pin must be set in order to enable the
        // output driver.
        // When OC2A is connected to the pin, the function of the COM2A1:0 bits
        // depends on the WGM22:0 bit setting.
        //
        // Fast PWM Mode
        // COM2A1 COM2A0
        // 0 0 Normal port operation, OC2A disconnected.
        // 0 1 WGM22 = 0: Normal Port Operation, OC0A Disconnected.
        //     WGM22 = 1: Toggle OC2A on Compare Match.
        // 1 0 Clear OC2A on Compare Match, set OC2A at BOTTOM
        // 1 1 Clear OC2A on Compare Match, clear OC2A at BOTTOM
        
        // COM2A1 COM2A0 
        // 0 0 Normal port operation, OC2A disconnected.
        // 0 1 WGM22 = 0: Normal Port Operation, OC0A Disconnected.
        //     WGM22 = 1: Toggle OC2A on Compare Match.
        // 1 0 Clear OC2A on Compare Match, set OC2A at BOTTOM, (non-inverting mode).
        // 1 1 Set OC2A on Compare Match, clear OC2A at BOTTOM, (inverting mode).
        
        // Setup the Timer/Counter Control Registers - TCCR2A
        // TCCR2A
        //   - set WGM bits to 011 - Fast PWM mode
        //   - set COM2A CS bits to 10 - Non inverted PWM
        // TCCR2A = (1 << COM2A1) | (1 << WGM21) | (1 << WGM20);
        
        //TCCR2A = _BV(COM2A0) | _BV(WGM21) | _BV(WGM20); // set fast PWM mode on timer 2
        TCCR2A = _BV(COM2A1) | _BV(WGM21) | _BV(WGM20); // set simple fast PWM mode on timer 2
        
         // Combined with the WGM22 bit found in the TCCR2B Register, these bits
        // control the counting sequence of the counter, the source for maximum
        // (TOP) counter value, and what type of waveform generation to be used
        // Modes of operation supported by the Timer/Counter unit are:
        // - Normal mode (counter),
        // - Clear Timer on Compare Match (CTC) mode,
        // - two types of Pulse Width Modulation (PWM) modes.
        //
        // Mode WGM22 WGM21 WGM20 Operation TOP
        // 0 0 0 0 Normal  0xFF
        // 1 0 0 1 PWM  0xFF
        // 2 0 1 0 CTC  OCRA
        // 3 0 1 1 Fast PWM 0xFF
        // 4 1 0 0 Reserved -
        // 5 1 0 1 PWM  OCRA
        // 6 1 1 0 Reserved -
        // 7 1 1 1 Fast PWM OCRA 
        
        
        // To use fast pwm we have two modes to choose, mode 3 and mode 7, 
        // the main difference between this two modes is that in mode 3 TOP is fixed at 0xFF and in mode 7 
        // TOP is defined by the TOP register, this means that if there is the need, we can change the maximum 
        // count that the timer will do until it overflows, so this means that we could control the frequency and the 
        // duty cycle, I never felt the need to do that, but if you want now you know that you can do it.
        // To use the mode 3, the WGM01 and WGM00 bits must be set to one, those bits can be found in the TCCR0A register.
        
        // TCCR2B = _BV(WGM22) | _BV(CS20); // fast pwm mode 7 (wgm22 must be set), OCRA = TOP, clock = 16mhz/256/1 = 62,5 khz 
        TCCR2B = _BV(CS20); // fast pwm mode 3, clock = 16mhz/256/1 = 62,5 khz, 0xFF = TOP 
        
        
        // Note that in this mode, only output B can be used for PWM; 
        // OCRA cannot be used both as the top value and the PWM compare value. 
        // However, there is a special-case mode "Toggle OCnA on Compare Match" that will toggle output A at the 
        // end of each cycle, generating a fixed 50% duty cycle and half frequency in this case. = WGM22:21:20 SET && COMA20 SET !!!!
        
        // CS22 CS21 CS20 Description
        // 0 0 0 No clock source (Timer/Counter stopped)
        // 0 0 1 clkI/O/(No prescaling) = 62 kHz
        // 0 1 0 clkI/O/8 (From prescaler) = 7 kHz
        // 0 1 1 clkI/O/64 (From prescaler) = 3 = 2kHz
        // 1 0 0 clkI/O/256 (From prescaler) = 4 = 1 kHz
        // 1 0 1 clkI/O/1024 (From prescaler) = 5 = 490 Hz
        // 1 1 0 clkI/O/2048 (From prescaler) = 6 = 240 Hz
        // 1 1 1 clkI/O/4096 (From prescaler) = 7 = 61 Hz


        DDR_SND |= _BV(SND_PIN); //set pb3 (digital pin 11) to output, DDR_SND = DDRB data direction register B

	// tone is stopped by vsync_line

	if (duration_ms > 0)

		remainingToneVsyncs = duration_ms*60/1000; //60 here represents the framerate

	else

		remainingToneVsyncs = -1; // continuous tone

 

    // Set the OCR for the given timer,

    OCR2A = duty_start; // duty cycle of fast PWM, was: cycle time for the timer, is repeated remainingToneVsyncs times 

}
