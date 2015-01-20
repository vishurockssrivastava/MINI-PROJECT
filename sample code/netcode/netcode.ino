/*
* sinewave_pcm
*
* Generates 8-bit PCM sinewave on pin 6 using pulse-width modulation (PWM).
* For Arduino with Atmega368P at 16 MHz.
*
* Uses timers 1 and 0. Timer 1 reads the sinewave table, SAMPLE_RATE times a
second.
* The sinewave table has 256 entries. Consequently, the sinewave has a
frequency of
* f = SAMPLE_RATE / 256
* Each entry in the sinewave table defines the duty-cycle of Timer 0. Timer 0
* holds pin 6 high from 0 to 255 ticks out of a 256-tick cycle, depending on
* the current duty cycle. Timer 0 repeats 62500 times per second (16000000 /
256),
* much faster than the generated sinewave generated frequency.
*
* References:
* http://www.atmel.com/dyn/resources/prod_documents/doc2542.pdf
* http://www.analog.com/library/analogdialogue/archives/38-08/dds.html
* http://www.evilmadscientist.com/article.php/avrdac
* http://www.arduino.cc/playground/Code/R2APCMAudio
* http://www.scienceprog.com/generate-sine-wave-modulated-pwm-with-avrmicrocontroller/
* http://www.scienceprog.com/avr-dds-signal-generator-v10/
* http://documentation.renesas.com/eng/products/region/rtas/mpumcu/apn/
sinewave.pdf
* http://ww1.microchip.com/downloads/en/AppNotes/00655a.pdf
*
* By Gary Hill
* Adapted from a script by Michael Smith <michael@hurts.ca>
*/
#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#define SAMPLE_RATE 13000// 16 ksps
/*
* The sinewave data needs to be unsigned, 8-bit
*
* sinewavedata.h should look like this:
* const int sinewave_length=256;
*
* const unsigned char sinewave_data[] PROGMEM = {0x80,0x83, ...
*
*/
/* Sinewave table
* Reference:
* http://www.scienceprog.com/generate-sine-wave-modulated-pwm-with-avrmicrocontroller/
*/
const int sinewave_length=256;
const unsigned char sinewave_data[] PROGMEM = {
0x80,0x83,0x86,0x89,0x8c,0x8f,0x92,0x95,0x98,0x9c,0x9f,0xa2,0xa5,0xa8,0xab,0xae,
0xb0,0xb3,0xb6,0xb9,0xbc,0xbf,0xc1,0xc4,0xc7,0xc9,0xcc,0xce,0xd1,0xd3,0xd5,0xd8,
0xda,0xdc,0xde,0xe0,0xe2,0xe4,0xe6,0xe8,0xea,0xec,0xed,0xef,0xf0,0xf2,0xf3,0xf5,
0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfc,0xfd,0xfe,0xfe,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xfe,0xfe,0xfd,0xfc,0xfc,0xfb,0xfa,0xf9,0xf8,0xf7,
0xf6,0xf5,0xf3,0xf2,0xf0,0xef,0xed,0xec,0xea,0xe8,0xe6,0xe4,0xe2,0xe0,0xde,0xdc,
0xda,0xd8,0xd5,0xd3,0xd1,0xce,0xcc,0xc9,0xc7,0xc4,0xc1,0xbf,0xbc,0xb9,0xb6,0xb3,
0xb0,0xae,0xab,0xa8,0xa5,0xa2,0x9f,0x9c,0x98,0x95,0x92,0x8f,0x8c,0x89,0x86,0x83,
0x80,0x7c,0x79,0x76,0x73,0x70,0x6d,0x6a,0x67,0x63,0x60,0x5d,0x5a,0x57,0x54,0x51,
0x4f,0x4c,0x49,0x46,0x43,0x40,0x3e,0x3b,0x38,0x36,0x33,0x31,0x2e,0x2c,0x2a,0x27,
0x25,0x23,0x21,0x1f,0x1d,0x1b,0x19,0x17,0x15,0x13,0x12,0x10,0x0f,0x0d,0x0c,0x0a,
0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x03,0x02,0x01,0x01,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x02,0x03,0x03,0x04,0x05,0x06,0x07,0x08,
0x09,0x0a,0x0c,0x0d,0x0f,0x10,0x12,0x13,0x15,0x17,0x19,0x1b,0x1d,0x1f,0x21,0x23,
0x25,0x27,0x2a,0x2c,0x2e,0x31,0x33,0x36,0x38,0x3b,0x3e,0x40,0x43,0x46,0x49,0x4c,
0x4f,0x51,0x54,0x57,0x5a,0x5d,0x60,0x63,0x67,0x6a,0x6d,0x70,0x73,0x76,0x79,0x7c};
int outputPin = 6; // (PCINT22/OC0A/AIN0)PD6, Arduino Digital Pin 6
volatile uint16_t sample;
// This is called at SAMPLE_RATE kHz to load the next sample.
ISR(TIMER1_COMPA_vect) {
if (sample >= sinewave_length) {
sample = -1;
}else {
OCR0A = pgm_read_byte(&sinewave_data[sample]);
}
++sample;
}
void startPlayback()
{
pinMode(outputPin, OUTPUT);
// Set Timer 0 Fast PWM Mode (Section 14.7.3)
// WGM = 0b011 = 3 (Table 14-8)
// TOP = 0xFF, update OCR0A register at BOTTOM
TCCR0A |= _BV(WGM01) | _BV(WGM00);
TCCR0B &= ~_BV(WGM02);
// Do non-inverting PWM on pin OC0A, arduino digital pin 6
// COM0A = 0b10, clear OC0A pin on compare match,
// set 0C0A pin at BOTTOM (Table 14-3)
TCCR0A = (TCCR0A | _BV(COM0A1)) & ~_BV(COM0A0);
// COM0B = 0b00, OC0B disconnected (Table 14-6)
TCCR0A &= ~(_BV(COM0B1) | _BV(COM0B0));
// No prescaler, CS = 0b001 (Table 14-9)
TCCR0B = (TCCR0B & ~(_BV(CS02) | _BV(CS01))) | _BV(CS00);
// Set initial pulse width to the first sample.
OCR0A = pgm_read_byte(&sinewave_data[0]);
// Set up Timer 1 to send a sample every interrupt.
cli(); // disable interrupts
// Set CTC mode (Section 15.9.2 Clear Timer on Compare Match)
// WGM = 0b0100, TOP = OCR1A, Update 0CR1A Immediate (Table 15-4)
// Have to set OCR1A *after*, otherwise it gets reset to 0!
TCCR1B = (TCCR1B & ~_BV(WGM13)) | _BV(WGM12);
TCCR1A = TCCR1A & ~(_BV(WGM11) | _BV(WGM10));
// No prescaler, CS = 0b001 (Table 15-5)
TCCR1B = (TCCR1B & ~(_BV(CS12) | _BV(CS11))) | _BV(CS10);
// Set the compare register (OCR1A).
// OCR1A is a 16-bit register, so we have to do this with
// interrupts disabled to be safe.
OCR1A = F_CPU / SAMPLE_RATE; // 16e6 / 8000 = 2000
// Enable interrupt when TCNT1 == OCR1A (p.136)
TIMSK1 |= _BV(OCIE1A);sample = 0;
sei(); // enable interrupts
}
void setup()
{
startPlayback();
}
void loop()
{
while (true);
}

