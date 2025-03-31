//******YOU MAY NEED TO CHANGE SOME PARAMETERS FOR ACCURATE RESULTS*******//
//*****************PLEASE READ THE COMMENTS DOWN BELOW********************//


#include <LiquidCrystal.h>
LiquidCrystal lcd(12, 11, 5, 4, 3, 2); //LCD


#define pinBat    A0  // Shunt resistor pin voltage (higher V ref. to GND)
#define pinShunt  A1  // Shunt resistor other pin voltage (lower V ref. to GND) 
#define pinMosfet 10  // Digital pin 10 connected to mosfet
#define pinLED    9   // Digital pin 9 connected to LED

void setup() {
  // LCD
  lcd.begin(16, 2);
  lcd.print("Batt. mAh Tester");
  delay(1000);
  lcd.clear();
  lcd.print("No Battery...");
  delay(1000);
  //lcd.clear();
  
  // Pin Configure
  pinMode(pinMosfet, OUTPUT);
  digitalWrite(pinMosfet, LOW);

  pinMode(pinLED, OUTPUT);
  digitalWrite(pinLED, LOW);
}

int     freq      = 1000;   // Every 1 second (1000 milliseconds) mAh will be calculated

float  rShunt    = 0.99;    // Shunt resistor resistance (if possible instead of marked value on resistor, use multimeter to get a accurate value)
float  vArd      = 5.10;   // Use a multimeter and check the voltage accros arduino +5V & GND pins
float  vMin      = 2.90;    // Minimum battery voltage (Use 2.9 to 3.0V) - discharge will be stopped at this voltage 

float  iShunt    = 0.0;     // Current through the shunt resister (1 ohm)
float  vBat      = 0.0;     // Battery voltage
float  vShunt    = 0.0;     // Voltage accros the shunt resister
float  mAh       = 0.0;     
boolean stop_cal = false;

unsigned long   tPrv  = 0;
unsigned long   tPas  = 0;

void loop() {
  
  vBat = analogRead(pinBat) * (vArd / 1023);

  if(stop_cal==false){  
    if (vBat < vMin){
      lcd.clear() ;
      lcd.setCursor(0, 0);
      lcd.print("Low Battery...");
      lcd.setCursor(0, 1);
      lcd.print(vBat);
      lcd.print("V");
      delay(500);
    }
    else{
      lcd.clear() ;
      lcd.setCursor(0, 0);
      lcd.print("Battery Voltage:");
      lcd.setCursor(0, 1);
      lcd.print(vBat);
      lcd.print("V");
      delay(500);
    }
  }
  
  while(vBat >= vMin && stop_cal==false ){
      stop_cal==false;
      digitalWrite(pinMosfet, HIGH);
      
      vBat = analogRead(pinBat) * (vArd / 1023);
      vShunt = analogRead(pinShunt) * (vArd / 1023);
        
      tPas = millis() - tPrv;
      iShunt = (vBat - vShunt) / rShunt;
      mAh = mAh + (iShunt * tPas / 3600.0);
      tPrv = millis();
  
      lcd.clear() ;
      lcd.setCursor(0, 0);
      lcd.print(vBat);
      lcd.print("V");
      lcd.print("  ");
      lcd.print(iShunt*1000);
      lcd.print("mA");
      
      lcd.setCursor(0, 1);
      lcd.print((int)mAh);
      lcd.print("mAh");
      
      digitalWrite(pinLED, HIGH); //LED will blink while discharging
      delay(5);
      digitalWrite(pinLED, LOW);

      if(vBat < vMin){
        stop_cal = true;
        digitalWrite(pinLED, HIGH); // LED will continuously lightup after done discharging up to given min. voltage
        digitalWrite(pinMosfet, LOW);
      }
      
      delay(freq);
  }        
}
