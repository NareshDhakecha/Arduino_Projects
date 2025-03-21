#include <Arduino.h>
#include <LiquidCrystal.h> //Libraries
#include <EEPROM.h>
#include <Wire.h> // i2C Conection Library

LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

#define bt_set A0
#define bt_next A1
#define bt_up A2
#define bt_down A3

#define relay 8
#define buzzer 13

// RTC DS3231
#define DS3231_I2C_ADDRESS 0x68 // address of DS3231 module

int current_hh = 0, current_mm = 0, current_ss = 0, current_day = 0; // variables to store real time from RTC module
int bell_duration = 0;                                               // seconds for the bell to ring
int bell_runtime = 0;                                                // remaining seconds for ringing
int setting_day = 0;                                                 // set bell for
int StartHH = 0, StartMM = 0;                                        // variables to store alarm starting time
int Alarm = 0;                                                       // storing total number of alarms for a day
int weekend_day = 0;                                                 // storing the weekend day
int set = 0;                                                         // setting block
int setMode = 0;                                                     // 1=day, 2=clock, 3=bell time, 4=weekend, 5=week, 6=alarm
int max = 30;                                                        // Max number of alarms
int flag0 = 2;                                                       // alarm is already run 0 = not run, 1 = run , 2 = not run
int flag1 = 0;                                                       // SET button 0 =not pressed, 1=pressed
int flag2 = 0;                                                       // NEXT button 0 =not pressed, 1=pressed
int flag3 = 1;                                                       // last alarm 0 = not last, 1 = last
int flash = 0;                                                       // flag for flashing the setting block

word MilliSecond = 0;
bool timerStart = false; // flag for starting the timer

String show_day[8] = {"", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat", "Sun"};

void setup()
{
  Wire.begin(); // start I2C communication

  pinMode(bt_set, INPUT_PULLUP);
  pinMode(bt_next, INPUT_PULLUP);
  pinMode(bt_up, INPUT_PULLUP);
  pinMode(bt_down, INPUT_PULLUP);

  pinMode(relay, OUTPUT);
  pinMode(buzzer, OUTPUT);

  digitalWrite(relay, 1);

  lcd.begin(16, 2);    // Configura lcd numero columnas y filas
  lcd.setCursor(0, 0); // Show "TIME" on the LCD
  lcd.print("   Welcome To   ");
  lcd.setCursor(0, 1);
  lcd.print("Auto School Bell");

  if (EEPROM.read(0) == 0)
  {
  }
  else
  {
    for (int x = 1; x < max; x++)
    {
      WriteEeprom(1, x);
      delay(2);
      WriteEeprom(2, x);
      delay(2);
      WriteEeprom(3, x);
      delay(2);
      WriteEeprom(4, x);
      delay(2);
      WriteEeprom(5, x);
      delay(2);
      WriteEeprom(6, x);
      delay(2);
      WriteEeprom(7, x);
      delay(2);
    }
    EEPROM.write(0, 0);
    EEPROM.write(10, 0); // bell duration
    EEPROM.write(11, 0); // weekend day
  }

  bell_duration = EEPROM.read(10);
  weekend_day = EEPROM.read(11);

  delay(2000);
  lcd.clear();

  noInterrupts(); // disable all interrupts
  TCCR1A = 0;     // set entire TCCR1A register to 0  //set timer1 interrupt at 1kHz  // 1 ms
  TCCR1B = 0;     // same for TCCR1B
  TCNT1 = 0;      // set timer count for 1khz increments
  OCR1A = 1999;   // = (16*10^6) / (1000*8) - 1
  // had to use 16 bit timer1 for this bc 1999>255, but could switch to timers 0 or 2 with larger prescaler
  //  turn on CTC mode
  TCCR1B |= (1 << WGM12); // Set CS11 bit for 8 prescaler
  TCCR1B |= (1 << CS11);  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  interrupts(); // enable
}

void loop()
{
  GetRtc(); // Read seconds, minutes, hours, and day from RTC module

  // --- Check if the alarm is to be run ---
  if (current_ss == 0) // Check if seconds is 0
  {
    if (flag0 == 0 || flag0 == 2)
    {
      flag0 = 0;
      Alarm = EEPROM.read(current_day);   // Read the number of alarms for the day
      for (int x = 1; x < Alarm + 1; x++) // Loop through the alarms
      {
        ReadEeprom(current_day, x); // Read the alarm time and store in StartHH and StartMM
        if (StartHH == current_hh && StartMM == current_mm)
        {
          bell_runtime = bell_duration; // set the seconds for the bell to ring
          // --- Ring the bell ---
          digitalWrite(relay, 0);
          timerStart = true;

          // --- Read the next alarm ---
          if (Alarm == x) // Check if the alarm is the last one
            flag3 = 1;
          else
          {
            flag3 = 0;
            ReadEeprom(current_day, x + 1); // Read the next alarm time and store in StartHH and StartMM
          }
          x = 100; // Exit the loop
        }
      }
    }
  }
  else
  {
    flag0 = 0;
  }

  // --- Check if the SET button is pressed ---
  if (digitalRead(bt_set) == 0) // SET btn pressed
  {
    digitalWrite(buzzer, 1);
    if (flag1 == 0) // SET btn press not processed
    {
      flag1 = 1;             // SET btn pressed
      setMode = setMode + 1; // Set mode level increase
      if (setMode == 6)      // last mode
      {
        if (setting_day == 0) // Week days
          Alarm = 1;
        else
        {
          Alarm = EEPROM.read(setting_day); // Read the number of alarms for the day of setting
          ReadEeprom(setting_day, Alarm);
        }
      }
      if (setMode > 6) // exit setting mode
        setMode = 0, setting_day = 0;
      lcd.clear();
    }
  }
  else
  {
    flag1 = 0; // SET btn not pressed
  }

  if (digitalRead(bt_next) == 0) // NEXT btn pressed
  {
    digitalWrite(buzzer, 1);
    if (flag2 == 0) // NEXT btn press not processed
    {
      flag2 = 1;        // NEXT btn press processing
      if (setMode == 2) // Clock setting mode
      {
        set = !set; // set = 0 or 1
      }
      if (setMode == 6)
      {
        set = set + 1; // set = 0, 1, 2
        if (set > 2)
        {
          set = 0;
          if (setting_day == 0) // setting for all days till weekend
          {
            for (int x = 1; x < weekend_day + 1; x++)
            {
              WriteEeprom(x, Alarm);
            }
          }
          else
          {
            // setting for the selected day
            WriteEeprom(setting_day, Alarm);
          }
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("   Ok Stored");
          delay(500);
          lcd.clear();
          if (Alarm < max)
            Alarm = Alarm + 1; // increase the alarm number
        }
      }
    }
  }
  else
  {
    flag2 = 0; // NEXT btn press process completed
  }

  if (digitalRead(bt_up) == 0) // UP btn pressed
  {
    digitalWrite(buzzer, 1);
    if (setMode == 1) // Day setting mode
    {
      current_day = current_day + 1;
      if (current_day > 7)
        current_day = 1;

      // Set the day in RTC module
      SetRtc(current_ss, current_mm, current_hh, current_day);
    }

    if (setMode == 2) // Clock setting mode
    {
      if (set == 0) // Hour setting
        current_hh = current_hh + 1;
      else
        // Minute setting
        current_mm = current_mm + 1;
      if (current_hh > 23)
        current_hh = 0;
      if (current_mm > 59)
        current_mm = 0;

      // Set the time in RTC module
      SetRtc(current_ss, current_mm, current_hh, current_day);
    }

    if (setMode == 3) // Bell time setting mode
    {
      bell_duration = bell_duration + 1;
      if (bell_duration > 99)
        bell_duration = 1;
      EEPROM.write(10, bell_duration);
    }

    if (setMode == 4) // Weekend setting mode
    {
      weekend_day = weekend_day + 1;
      if (weekend_day > 7)
        weekend_day = 1;
      EEPROM.write(11, weekend_day);
    }

    if (setMode == 5) // WeekDays or single day setting mode
    {
      setting_day = setting_day + 1;
      if (setting_day > 7)
        setting_day = 0;
    }

    if (setMode == 6) // Alarm setting mode
    {
      if (set == 0)
        Alarm = Alarm + 1;
      if (set == 1)
        StartHH = StartHH + 1;
      if (set == 2)
        StartMM = StartMM + 1;

      if (Alarm > max)
        Alarm = 1;
      if (set == 0 && setting_day > 0)
        ReadEeprom(setting_day, Alarm);

      if (StartHH > 23)
        StartHH = 0;
      if (StartMM > 59)
        StartMM = 0;
    }

    delay(50);
  }

  if (digitalRead(bt_down) == 0) // DOWN btn pressed
  {
    digitalWrite(buzzer, 1);
    if (setMode == 1) // Day setting mode
    {
      current_day = current_day - 1;
      if (current_day < 1)
        current_day = 7;
      SetRtc(current_ss, current_mm, current_hh, current_day);
    }

    if (setMode == 2) // Clock setting mode
    {
      if (set == 0)
        current_hh = current_hh - 1;
      else
        current_mm = current_mm - 1;
      if (current_hh < 0)
        current_hh = 23;
      if (current_mm < 0)
        current_mm = 59;
      SetRtc(current_ss, current_mm, current_hh, current_day);
    }

    if (setMode == 3) // Bell time setting mode
    {
      bell_duration = bell_duration - 1;
      if (bell_duration < 1)
        bell_duration = 99;
      EEPROM.write(10, bell_duration);
    }

    if (setMode == 4) // Weekend setting mode
    {
      weekend_day = weekend_day - 1;
      if (weekend_day < 1)
        weekend_day = 7;
      EEPROM.write(11, weekend_day);
    }

    if (setMode == 5) // WeekDays or single day setting mode
    {
      setting_day = setting_day - 1;
      if (setting_day < 0)
        setting_day = 7;
    }

    if (setMode == 6) // Alarm setting mode
    {
      if (set == 0)
        Alarm = Alarm - 1;
      if (set == 1)
        StartHH = StartHH - 1;
      if (set == 2)
        StartMM = StartMM - 1;

      if (Alarm < 1)
        Alarm = max;
      if (set == 0 && setting_day > 0)
        ReadEeprom(setting_day, Alarm);

      if (StartHH < 0)
        StartHH = 23;
      if (StartMM < 0)
        StartMM = 59;
    }

    delay(50);
  }

  if (setMode > 0)  // Setting mode
    flash = !flash; // Flash the LCD
  Display();        // Display the data on the LCD
  delay(150);
  digitalWrite(buzzer, LOW); // Turn off the buzzer
}

void Display()
{
  if (setMode == 0) // Normal mode
  {
    // Display the day and time
    lcd.setCursor(0, 0);
    lcd.print(show_day[current_day]);
    lcd.print("    ");
    lcd.setCursor(8, 0);
    lcd.print((current_hh / 10) % 10);
    lcd.print(current_hh % 10);
    lcd.print(":");
    lcd.print((current_mm / 10) % 10);
    lcd.print(current_mm % 10);
    lcd.print(":");
    lcd.print((current_ss / 10) % 10);
    lcd.print(current_ss % 10);

    // Display the bell time
    lcd.setCursor(0, 1);
    if (timerStart == true) // Bell is ringing
    {
      lcd.print("Bell On  ");
      lcd.print((bell_runtime / 10) % 10);
      lcd.print(bell_runtime % 10);
      lcd.print(" Sec  ");
    }
    else
    {
      lcd.print("Next Bell=");
      if (flag3 == 0) // not last alarm
      {
        lcd.print((StartHH / 10) % 10);
        lcd.print(StartHH % 10);
        lcd.print(":");
        lcd.print((StartMM / 10) % 10);
        lcd.print(StartMM % 10);
      }
      else
      {
        lcd.print("##:##");
      }
    }
  }

  if (setMode == 1) // Day setting mode
  {
    lcd.setCursor(0, 0);
    lcd.print("SET Day");
    lcd.setCursor(1, 1);
    if (flash)
      lcd.print(show_day[current_day]);
    else
      lcd.print("   ");
  }

  if (setMode == 2) // Clock setting mode
  {
    lcd.setCursor(0, 0);
    lcd.print("SET Clock");
    lcd.setCursor(1, 1);
    if (flash)
    {
      // Display full time
      lcd.print((current_hh / 10) % 10);
      lcd.print(current_hh % 10);
      lcd.print(":");
      lcd.print((current_mm / 10) % 10);
      lcd.print(current_mm % 10);
    }
    else
    {
      if (set == 0)
        lcd.setCursor(1, 1);
      if (set == 1)
        lcd.setCursor(4, 1);
      lcd.print("  "); // Display blank the hour or minute for flashing effect
    }
  }

  if (setMode == 3)
  {
    lcd.setCursor(0, 0);
    lcd.print("SET Bell Time");
    lcd.setCursor(1, 1);
    if (flash)
    {
      lcd.print((bell_duration / 10) % 10);
      lcd.print(bell_duration % 10);
    }
    else
      lcd.print("  ");

    lcd.print(" Sec");
  }

  if (setMode == 4)
  {
    lcd.setCursor(0, 0);
    lcd.print("SET Weekend");
    lcd.setCursor(1, 1);
    if (flash)
    {
      lcd.print(show_day[weekend_day]);
    }
    else
      lcd.print("   ");
  }

  if (setMode == 5)
  {
    lcd.setCursor(0, 0);
    lcd.print("SET Bell for");
    lcd.setCursor(1, 1);
    if (flash)
    {
      if (setting_day == 0)
        lcd.print("Week Days");
      else
        lcd.print(show_day[setting_day]);
    }
    else
      lcd.print("         ");
  }

  if (setMode == 6)
  {
    lcd.setCursor(0, 0);
    lcd.print("SET Bell");
    if (flash)
    {
      lcd.setCursor(1, 1);
      lcd.print((Alarm / 10) % 10);
      lcd.print(Alarm % 10);
      lcd.print("/");
      lcd.print((max / 10) % 10);
      lcd.print(max % 10);

      lcd.setCursor(8, 1);
      lcd.print("T=");
      lcd.print((StartHH / 10) % 10);
      lcd.print(StartHH % 10);
      lcd.print(":");
      lcd.print((StartMM / 10) % 10);
      lcd.print(StartMM % 10);
    }
    else
    {
      if (set == 0)
        lcd.setCursor(1, 1);
      else if (set == 1)
        lcd.setCursor(10, 1);
      else if (set == 2)
        lcd.setCursor(13, 1);
      lcd.print("  ");
    }
  }
}

// Set RTC
void SetRtc(byte second, byte minute, byte hour, byte dayOfWeek)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0);                   // set 0 to first register
  Wire.write(decToBcd(second));    // set second
  Wire.write(decToBcd(minute));    // set minutes
  Wire.write(decToBcd(hour));      // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=su, 2=mo, 3=tu)
  Wire.endTransmission();
}

// read RTC
void GetRtc()
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // write "0"
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7); // request - 7 bytes from RTC
  current_ss = bcdToDec(Wire.read() & 0x7f);
  current_mm = bcdToDec(Wire.read());
  current_hh = bcdToDec(Wire.read() & 0x3f);
  current_day = bcdToDec(Wire.read());
}

// conversion Dec to BCD
byte decToBcd(byte val)
{
  return ((val / 10 * 16) + (val % 10));
}

// conversion BCD to Dec
byte bcdToDec(byte val)
{
  return ((val / 16 * 10) + (val % 16));
}

void ReadEeprom(int _day, int _alarm)
{
  int eeprom = (_day * 100) + _alarm * 3;
  StartHH = EEPROM.read(eeprom);
  StartMM = EEPROM.read(eeprom + 1);
}

void WriteEeprom(int _day, int _alarm)
{
  int eeprom = (_day * 100) + _alarm * 3;
  EEPROM.write(eeprom, StartHH);
  EEPROM.write(eeprom + 1, StartMM);
  EEPROM.write(_day, Alarm);
}

// Automatic turn off the bell after the set time
//  Timer1 interrupt service routine for every 1ms to count the seconds
ISR(TIMER1_COMPA_vect)
{
  if (timerStart == true)
  {
    MilliSecond++;
    if (MilliSecond >= 1000)
    {
      MilliSecond = 0;
      bell_runtime = bell_runtime - 1;
      if (bell_runtime <= 0)
      {
        timerStart = false;
        digitalWrite(relay, 1);
      }
    }
  }
}
