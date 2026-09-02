//SinWave PWM modulator for inverter 50Hz~60Hz
//N.Bastiayanov 08.2026
//ATtiny85
//MCU clock PLL 16MHz
//PWM ~8.1kHz 
//outputs PB0,PB1
//voltage feedback A3 PB3
//freqency A2 PB4
//fault, active LOW PB2

#define wdt_reset() __asm__ __volatile__ ("wdr")

byte p = 250;            //initial amplitude, max 255
unsigned int freq = 400; //initial freq ~50Hz

void setup() {
  CLKPR = 0x80; CLKPR = 0;  // CPU clock prescaler 0
  OSCCAL =B11111111;        // inrease MCU clock to max
  DDRB = B00000011;         // PB0,PB1 set to output
  GTCCR = 0;                // timer0 settings
  TCCR0A = B10100001;       // non invert, phase correct PWM, 
  TCCR0B = B00000010;       // prescaler 1
 
  MCUCR = 0;                //interrupt INT0 on PB2(pin7)
  GIMSK = B01000000;        //INT0, active LOW

  cli();                            //watchdog timer settings
  WDTCR = 0;                        // wdt enable
  WDTCR = (1 << WDCE) | (1 << WDE); // reenable wd
  WDTCR = B00001001;                // 16 msec
  sei();
}

ISR(INT0_vect) {                   //interrupt - fault input, active LOW
  PORTB = 0;
  TCCR0A = B00000001;              //PWM outputs go LOW
  for (int a = 0; a < 5000; a++) { //~1 sec fault delay
    wdt_reset();
    customDelay(1000);
  }
  if ((PINB & B00000100) == 0) {  //if fault continues after 1 sec, MCU will lock.
    for (;;) {
      wdt_reset();
    }
  }
}

void loop() {
  long y = 0;     //PWM value
  byte sf = 0;    //start flag
  unsigned int a; //analog input value for voltage feedback
  char b;         //counter value, actualy this is X axis,
                  //the time step depend by "customDelay(freq)" of the end of each cycle
  for (;;) {
    
//half period 1=============================
    wdt_reset();
    a = analogRead(A3);   //voltage feedback calculations
    if (a < 511) {
      if (p > 254) {
        p = 254;
      } p++;
    }
    else if (a >= 511) {
      if (p < 1) {
        p = 1;
      } p--;
    }

    freq = 100 + analogRead(A2) / 5; //frequency settings

//on start or after fault set counter value b=0 to start sine wawe from 0V
    if (TCCR0A == B00000001 || sf == 0) {
      b = 0;
      TCCR0A = B10100001;
      sf = 1;
    }
    else {
      b = -40;
    }

    for (; b < 41; b++) {       //(-40)~(0)~(+40) or (0)~(+40)
      char x = 40 - abs(b);     //inverse b value (0)~(+40)~(0) for normal operation
                                //or (+40)~(0) on start
      y = ( 4150 - x * x) / 9;  //sine calculations
      y = y * y / 1046 - 76;

      y = y * p / 255;          //amplitude correction, depending analog input A3

      if (b < 1) {
        OCR0A = 128 + y;
        OCR0B = 127 - y;
      }
      else {
        OCR0A = 127 - y;
        OCR0B = 128 + y;
      }
      customDelay(freq); // time step for X axis
    }
//half period 2=============================
    wdt_reset();
    a = analogRead(A3);
    if (a < 511) {
      if (p > 254) {
        p = 254;
      } p++;
    }
    else if (a >= 511) {
      if (p < 1) {
        p = 1;
      } p--;
    }
    freq = 100 + analogRead(A2) / 5;
    if (TCCR0A == B00000001 || sf == 0 ) {
      b = 0;
      TCCR0A = B10100001;
      sf = 1;
    }
    else {
      b = -40;
    }
    for (; b < 41; b++) {
      char x = 40 - abs(b);
      y = ( 4150 - x * x) / 9;
      y = y * y / 1046 - 76;
      y = y * p / 255;
      if (b < 1) {
        OCR0A = 127 - y;
        OCR0B = 128 + y;
      }
      else {
        OCR0A = 128 + y;
        OCR0B = 127 - y;
      }
      customDelay(freq);
    }
  }
}

void customDelay(unsigned int l) {
  for (unsigned int m = 0; m < l; m++) {
    __asm__("nop\n\t");   //empty MCU cycle
  }
}
