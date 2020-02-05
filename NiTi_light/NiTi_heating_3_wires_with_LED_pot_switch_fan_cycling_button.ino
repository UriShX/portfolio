/*  based on several examples from the arduino IDE, and:
     http://playground.arduino.cc/Learning/ArduinoSleepCode 
     https://spaghettionastick.wordpress.com/2011/10/17/nitinol-flexinol-muscle-wire-shape-memory-alloy/
     http://www.talkingelectronics.com/projects/Nitinol/Nitinol-2.html
     http://www.righto.com/2009/07/secrets-of-arduino-pwm.html
     http://playground.arduino.cc/Main/TimerPWMCheatsheet
     http://www.electroschematics.com/9540/arduino-fan-speed-controlled-temperature/
    */

// avr libraries included to facilitate use of switch    
#include <avr/sleep.h> 
#include <avr/power.h> 
#include <avr/interrupt.h> 

    const int switchPin = 2; // switch for the interrupt service routine
    const int potPin = A0; // potentiometer for LED intensity
    const int LED = 10; // LED PWM pin
//    const int tempPin[] = {A5}; // input pins of LM35 temp sensor
//    const int NiTi[] = {3}; // PWM pins for NiTi heating
    const int tempPin[] = {A5, A4, A3}; // input pins of LM35 temp sensor
    const int NiTi[] = {3, 11, 9}; // PWM pins for NiTi heating
    const int NiTi_pwm = 72; // provides duty cycle for NiTi heating as in n/256
    const int fanPin = 6; // PWM pin for fan
    const int buttonPin = 5;
    const int index = (sizeof(NiTi)/sizeof(int)); // number of entries in above arrays
    int localIndex = index;
    int minValue = NiTi_pwm-5; // min duty cycle for use when close to maximum allowable temp
    int maxValue = NiTi_pwm/2; // max duty cycle for use when slightly over maximum allowable temp heat
    int fan_pwm = 192; // 75% of maximum fan speed, so it will not burn out
    int potValue = 0;
    int potOutputValue = 0;
    int i = 0;
    int n = 0;
    int j = 0;
    int switchVal = 0;
    int buttonPushCounter = 1;   // counter for the number of button presses
    int buttonState = 0;         // current state of the button
    int lastButtonState = 0;     // previous state of the button
//    int temp[] = {0}; // must have the same no. of initializing params. as index
    int temp[] = {0, 0, 0}; // must have the same no. of initializing params. as index
    const int tempMin = 68; // the temperature to start control loop of heating the wire
    const int tempMax = 75; // the maximum temperature which the wire must not exceed
    int NiTiHeat[index];
//    int NiTiHeat[3];
    int afterSleep = 1;
    long lastDebounceTime = 0;  // the last time the output pin was toggled
    long debounceDelay = 50;    // the debounce time; increase if the output flickers
     
    void setup() {
      pinMode(switchPin, INPUT);
      pinMode(buttonPin,INPUT);
      digitalWrite(switchPin, HIGH);
      pinMode(potPin, INPUT);
      pinMode(LED, OUTPUT);
      for(n = 0; n<index; n++) {
        pinMode(NiTi[n], OUTPUT);
        pinMode(tempPin[n], INPUT);
      }
      pinMode(fanPin, OUTPUT);
      //Serial.begin(9600); // turn serial protocol ON
//    TCCR2B = TCCR2B & 0b11111000 | 0x05; // Hexadecimal number changes clock cycle duration for clock #2
                                           // Setting             Divisor     Frequency
                                           // 0x01          1          31372.55
                                           // 0x02          8          3921.16
                                           // 0x03          32          980.39
                                           // 0x04          64          490.20   <--DEFAULT
                                           // 0x05          128          245.10
                                           // 0x06          256          122.55
                                           // 0x07          1024          30.64
// since no discernible difference when used was evidenced, and clock #3 is used also, not used here - but kept for reference
    }

  
  void sleepNow() { // the arduino sleep function
      attachInterrupt(0, wakeUp, LOW); // use interrupt 0 (pin 2) and run function wakeUp when pin 2 gets LOW
      delay(50);
      set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here. PWR_DOWN is the most efficient.
      sleep_enable(); // enables the sleep bit in the mcucr register so sleep is possible. just a safety pin
      for(i; i>0; i--) {             // here all the periphirals are phased down, and their indexae initialized
        analogWrite(LED, i);         // first fade the LEDs
        delay(5);
      }
      digitalWrite(LED,LOW);
      for(n = 0; n<index; n++) {         // then make sure NiTi heating is off
        digitalWrite(NiTi[n], LOW);
      }
      for(j = fan_pwm; j>0; j--) {
        analogWrite(fanPin, j);
        delay(80);
      }
      digitalWrite(fanPin, LOW);
      //Serial.print("fan state:(off) ");
      sleep_mode(); // here the device is actually put to sleep!! THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP
      sleep_disable(); // first thing after waking from sleep: disable sleep...       
      detachInterrupt(0); // disables interrupt 0 on pin 2 so the wakeUpNow code will not be executed during normal running time.
   }

  void wakeUp() { //ISR
    afterSleep = 1;
  }
       
  void loop() {
    switchVal = digitalRead(switchPin); // check what is the switch state...
    delay(50);
    if (switchVal==HIGH) {              // and if it's high
      sleepNow();                 // send the arduino to sleep
    }
/*    Serial.println("\t sleep end");
    Serial.print("switch value: "); // this is just for debugging purposes
    Serial.println(switchVal);
    delay(5);*/
    analogWrite(fanPin, fan_pwm);
//    Serial.print("fan state:(on) ");
//    Serial.println(fanPin);
    potValue = analogRead(potPin); // read the analog in value, which defines the LED PWM
    potOutputValue = map(potValue, 0, 1023, 64, 255); // map it to the range of the analog out: potValue = analogRead(potPin);
    if (afterSleep == 1) {
    for (i = 0; i<potOutputValue; i++) { // fade in of LED, up to pot specified value
      analogWrite(LED, i);
      delay(10);
//      continue;
    }
    afterSleep = 0;
    }
    analogWrite(LED, potOutputValue); // redundant, just to be on the safe side
    buttonState = digitalRead(buttonPin);      // read the pushbutton input pin:
    delay(100);
//    if (buttonState != lastButtonState) {  // compare the buttonState to its previous state
//      lastDebounceTime = millis();
//    }
//    if ((millis() - lastDebounceTime) > debounceDelay) {
      if (buttonState != lastButtonState) {  // compare the buttonState to its previous state
       if (buttonState == HIGH) {    // if the state has changed, increment the counter
        if (buttonPushCounter < 3) {                            // if the current state is HIGH then the button went from off to on:
        buttonPushCounter++;
        } else {
          buttonPushCounter = 1;
        }
//      Serial.println("on");
//      Serial.print("number of button pushes:  ");
//      Serial.println(buttonPushCounter);
      } 
/*      else {
      // if the current state is LOW then the button went from on to off:
      Serial.println("off"); 
      }*/
    }
    lastButtonState = buttonState;  // save the current state as the last state, for next time through the loop
    if(buttonPushCounter % 3 == 0) {  // turns on the LED every four button pushes by 
      localIndex = 2;
    } else {                           // the modulo function gives you the remainder of the division of two numbers:
      if(buttonPushCounter %2 == 0) {    // checking the modulo of the button push counter.
        localIndex = 1;
      } else {
        localIndex = index;
      }
    }
    //Serial.print("button state: \t");
    //Serial.print(buttonState);
    //Serial.print("\t button presses: \t");
    //Serial.print(buttonPushCounter);
    //Serial.print("\t index: \t");
    //Serial.print(index);
    //Serial.print("\t local index: \t");
    //Serial.println(localIndex);
/*    Serial.print("pot value: ");
    Serial.print(potOutputValue);
    Serial.print("\t Temp: ");*/
    for(n = 0; n<index; n++) { // here every temp sensor is checked, and it's corresponding NiTi wire set with the appropriate PWM duty cycle
      if(n<localIndex) {
        temp[n] = readTemp(); // get the temperature
        if(temp[n] < tempMin) { // if temp is lower than minimum temp
          analogWrite(NiTi[n], NiTi_pwm);
        }
        if((temp[n] >= tempMin) && (temp[n] <= tempMax)) { // if temperature is higher than minimum temp and lower than maximum temp
          NiTiHeat[n] = map(temp[n], tempMin, tempMax, minValue, maxValue); // specifies duty cycles for min and max temp
          analogWrite(NiTi[n], NiTiHeat[n]); // PWM the NiTi in the specified duty cycle
        }
        if(temp[n] > tempMax) { // while temp is higher than tempMax
          analogWrite(NiTi[n], 25); // a very low duty cycle to allow for some cooling
        }
  /*      Serial.print("\t");
        Serial.print(n);
        Serial.print(": ");
        if(n<localIndex-1) {
          Serial.print(temp[n]);
        } else {
          Serial.println(temp[n]);
        }*/
      } else {
        digitalWrite(NiTi[n], LOW);
      }
      delay(1);
    }
    delay(20);
  }
     
    int readTemp() { // get the temperature and convert it to celsius
    temp[n] = analogRead(tempPin[n]);
    return temp[n] * 0.48828125;
    }
