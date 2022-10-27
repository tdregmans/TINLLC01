#include <avr/io.h>
#include <util/atomic.h>
#include <util/delay.h>

/**
 * Low Level Computing
 * Eindopdracht
 * Thijs Dregmans (1024272)
 * 20-01-2022
 */

void init() {
  DDRB |=(1<<PB0);
  DDRB |=(1<<PB1);
  DDRB |=(1<<PB2);
  // Slave Select: 49HC595
  DDRB |=(1<<PB3);
  // Slave Select: BMP280
  DDRB |=(1<<PB4);
  USICR |=(1<<USIWM0);
  USICR |=(1<<USICS1);
  USICR |=(1<<USICLK);
}

unsigned int T1;
signed int T2;
signed int T3;

uint8_t transfer(uint8_t teVersturenData) {
  USIDR = teVersturenData;
  // reset flag
  USISR |= (1 << USIOIF);
  ATOMIC_BLOCK (ATOMIC_RESTORESTATE) {
    do{
      // schuif/verstuur 1 bit
      USICR |= (1 << USITC);
    }while (!(USISR & (1 << USIOIF)));
  }
  return USIDR;
}

void writeToDisplay(int getal,bool punt) {
  ATOMIC_BLOCK (ATOMIC_RESTORESTATE) {
    int digits[21] = {
      0b11000000, //0
      0b11111001, //1
      0b10100100, //2
      0b10110000, //3
      0b10011001, //4
      0b10010010, //5
      0b10000010, //6
      0b11111000, //7
      0b10000000, //8
      0b10010000, //9
      0b11111111, //leeg
      // met decimal_point
      0b01000000, //0.
      0b01111001, //1.
      0b00100100, //2.
      0b00110000, //3.
      0b00011001, //4.
      0b00010010, //5.
      0b00000010, //6.
      0b01111000, //7.
      0b00000000, //8.
      0b00010000  //9.
    };
    // make latch low
    PORTB &= ~(1 << PB3);
    uint8_t bits = getal;
    if(punt) {
      // als decimaal punt gewenst is
      bits += 11;
    }
    transfer(digits[bits]);
    _delay_ms(100);
    // make latch high
    PORTB |= (1<<PB3);
  }
}

void setupSensor() {
  // make latch low
  PORTB &= ~(1 << PB4);
  // schuif instellingen naar sensor
  transfer(0x74);
  transfer(0b01000011);
  // make latch high
  PORTB |= (1<<PB4);
}

void kalibreren() {
  uint16_t tWaardes[3];
  // make latch low
  PORTB &= ~(1 << PB4);
  // stuur kalibratie commando
  transfer(0x88);
  for(int index = 0; index < 3; index++) {
    // haal waardes op
    uint8_t value1 = transfer(0);
    uint8_t value2 = transfer(0);
    uint16_t value3 = value2;
    // zet waardes achter elkaar
    value3 = (value3 << 8) | value1;
    tWaardes[index] = value3;
  }
  // make latch high
  PORTB |= (1<<PB4);
  // lees waardes uit array
  T1 = tWaardes[0];
  T2 = tWaardes[1];
  T3 = tWaardes[2];
}

float ReadTemp() {
  // make latch low
  PORTB &= ~(1 << PB4);
  // vraag om temperatuur
  transfer(0xFA);
  uint8_t bitSets[3];
  for(int index = 0; index < 3; index++) {
    // haal data op
    bitSets[index] = transfer(0);
  }
  // zet waardes weer achter elkaar
  long int adc_T = bitSets[0];
  adc_T = (adc_T << 8) | bitSets[1];
  adc_T = (adc_T << 4) | bitSets[2];
  // make latch high
  PORTB |= (1<<PB4);
  // gegeven code uit handleiding
  long  var1 = (((( adc_T  >> 3) - ((long)T1 << 1))) * ((long)T2)) >> 11;
  long  var2 = ((((( adc_T  >> 4) - ((long)T1)) * ((adc_T  >> 4) - ((long)T1))) >> 12) * ((long)T3)) >> 14;
  float  temp = (var1 + var2) / 5120.0;
  // temp nu bekend
  return temp;
}

int main() {
  //_delay_ms(1000);
  init();
  setupSensor();
  kalibreren();
  
  
  while(true) {    
    float temp = ReadTemp() * 0.1;
    // temp omzetten naar losse cijfers
    int tientallen = ((int) temp) % 10;
    int eenheden = ((int) (temp * 10)) % 10;
    int tienden = ((int) (temp * 100)) % 10;
    int honderdsten = ((int) (temp * 1000)) % 10;
    // schrijf getallen naar 7-Segment
    writeToDisplay(tientallen,false);
    _delay_ms(1000);
    writeToDisplay(eenheden,true);
    _delay_ms(1000);
    writeToDisplay(tienden,false);
    _delay_ms(1000);
    writeToDisplay(honderdsten,false);
    _delay_ms(1000);
    writeToDisplay(10,false);
    
    _delay_ms(1000);
    
  }
  return 1;
}