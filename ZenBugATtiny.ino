#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <EEPROM.h>

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif
volatile int buttonPress = 0;
volatile byte wdtCount = 0;
//byte durations[] = {37, 74, 112, 150};//10, 20, 30, 40 minutes
byte durations[] = {1, 2, 3, 4}; //for testing, 8*n seconds
byte ledPin = 0;
boolean timer = false;
unsigned int lastPress = 0;
int slow = 300;
int fast = 120;

void setup(){
//Serial.begin(9600);
  //Defines the interruptor
//  sbi(GIMSK,INT0); //Attiny
//  sbi(EIMSK, INT0); //AtMega328
  //Activates interrupt at raising
//  MCUCR |= (_BV(ISC01) | _BV(ISC00)); //Attiny
//  EICRA |= (_BV(ISC01) | _BV(ISC00));  //AtMega328

  
//WDT rotine
  //Clear reset Flag
  MCUSR &= ~(1<<WDRF);
  //Set WDT PreScaler
  //Habilitate for changing pre-scaler
  WDTCR |= (1<<WDCE) | (1<<WDE);//Attiny
  //Change pre-scaler
  WDTCR = 1<<WDP0 | 1<<WDP3;  //Attiny 8s
  //Desable WD Interrupt
//  cbi(WDTCSR, WDIE); ATMEGA
  cbi(WDTCR, WDIE); //ATTiny
  
  if (EEPROM.read(0)>3){
    EEPROM.write(0,3);
  }
  
  pinMode(ledPin,OUTPUT);
  pinMode(1,INPUT);
//  attachInterrupt(0,interrupt, CHANGE);
  sbi(GIMSK,PCIE); // Turn on Pin Change interrupt
  sbi(PCMSK,PCINT1); // Which pins are affected by the interrupt

//  Serial.println("Engine Started"); 
//  delay(100);
}

void loop(){
  if (buttonPress)
  {
    buttonPress = false;
    lastPress = millis();
//    Serial.println("Button pressed, going to meditate");
    meditate();
  }
  delay(50); //For the first cycle to complete
  enterSleep();
}

void meditate(){
  //Enable WDT
  wdtCount = 1;
  sbi(WDTCR, WDIE);
//  Serial.print("WDT = ");
//  Serial.println(wdtCount);
//  Serial.println(EEPROM.read(0));
  ledGlow();
  if(!buttonPress){
    enterSleep();
    //Desable WDT
//    Serial.println("Meditation begin");
    while(wdtCount<(EEPROM.read(0)+1))
    {
//      Serial.println(wdtCount);
      enterSleep();
    }
    if(buttonPress)
    {
//      Serial.println("Meditation Canceled");
      WDTOff();
      ledFlash(2,fast);
    }
    else
    {
      WDTOff();
      while (!buttonPress){
        ledGlow();
        delay(800);
      }
//      Serial.println("User Reconized end");
      ledFlash(1,slow);
    }
  }
  else
  {
    setTimer();
  }
}

void setTimer()
{
    WDTOff();
//    Serial.println("Set timer");
    delay(1500);
    buttonPress = false;
    for(int i = 1; i < 5; i++){
      ledFlash(i,slow);
//      Serial.println(i);
      for(int i = 1; i <25;i++)
      {
        delay(50);
        if(buttonPress)break;
      };
      if(buttonPress){
        EEPROM.write(0,i);
//        Serial.println("New time set");
        buttonPress = false;
        ledGlow();
        ledFlash(2,fast);
        break;
      }
    }
}

void WDTOff(){
  wdtCount = 0;
  cbi(WDTCR, WDIE);
}

void enterSleep()
{
//  Serial.println("Sleeping");
//  delay(100);
//  power_adc_disable();
  cbi(ADCSRA,ADEN);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  delay(150); //Dont know why, but this delay is a must have for code to work
//  sleep_enable(); 
  sleep_mode();
//  sleep_disable();

//  power_adc_enable();
  sbi(ADCSRA,ADEN);

//  Serial.println("Awaking");
//  delay(100);


  //Religa conversor analalogico



}

void ledFlash(int n, int s){
  for(int i = 0; i < n; i++){
  analogWrite(ledPin, 254);
  delay(60);
  analogWrite(ledPin, 0);
  delay(s);
  }
}

void ledGlow()
{
//  Serial.println("led glow");
  for(int x = 0; x < 240; x+=24){ //led fading in
    analogWrite(ledPin,x);
    delay(1000/30);
    if (buttonPress){
      break;
    }
  }
  for(int x = 250; x > 0; x-=10){ //led fading out
    analogWrite(ledPin,x);
    delay(1000/30);
    if (buttonPress){
      break;
    }
  }
  analogWrite(ledPin, 0);
}

//void interrupt()
ISR(PCINT0_vect) //Interrupt INT0 function
 {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 200)
  {
    buttonPress = true;
    if(wdtCount)
   {   
    wdtCount = 250;
    cbi(WDTCR, WDIE);
   }
  }
  last_interrupt_time = interrupt_time;
 }
 
ISR (WDT_vect) //WatchDog WakeUp rotine
{
  wdtCount++;
}
