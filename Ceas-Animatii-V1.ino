//---------------------------------------------------------------------------
// base info from GeoMar - http://hobby-geomar.blogspot.com/
// adapted by Nicu FLORICA (niq_ro) - https://nicuflorica.blogspot.com/
// ver.0 - test or real (DS18B20 & RTC 0 DS1307), common cathode or common anode
// Vers V1, 28.01.2019 - Mereni
// --------------------------------------------------------------------------

#include <DallasTemperature.h> // http://www.milesburton.com/?title=Dallas_Temperature_Control_Library
#include <OneWire.h> // http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <SimpleTimer.h>  // http://www.arduino.cc/playground/Code/SimpleTimer.html
#include <Wire.h>  //Included with Arduino IDE
#include "RTClib.h"

#define DS1307_ADDRESS 0x68  //RTC Module Address
#define ONE_WIRE_BUS A1      //DS18B20 temperature sensor connected to analog pin 0.
#define precizie 9

byte digits[21] = {  //Bit pattern for 7 segment display
  // -ABCDEFG            Segments labelled as per datasheet for Kingbright DA56-11EW dual common anode LED display module.
  B10000001,  // 0     1 represents segment OFF, 0 is ON
  B11001111,  // 1     the MSB is not used.
  B10010010,  // 2
  B10000110,  // 3
  B11001100,  // 4
  B10100100,  // 5
  B10100000,  // 6
  B10001111,  // 7
  B10000000,  // 8
  B10000100,  // 9
  B10011100,  // Degree symbol
  B10110001,  // Letter C
  B11111110,  // - Symbol
  B10011111,  //  Letter E
  B11100011,   // u
  B11111111,   // Blank (OFF)
  B11110000,   // t
  B11101010,   // n
  B11000010,   // d
  B11100010,   // a
  B11111110,   // -
};

int te[4] = { -23, -6, 4, 34};

int anodes[4] = {11, 10, 9, 6}; //Common anodes for 7 segment displays connected to these digital outputs via NPN transistors
//Pin 11 is Left digit, 6 is Right digit.
int cathodes[8] = {8, 7, 12, 5, 4, 3, 2, 13}; //Cathodes for each segment all tied together and connected to these digital outputs.
//Segment to pin assignment
//A=2,B=3,C=4,D=5,E=12,F=7,G=8,DP=13 segments
int data[4];  //4 byte array stores value to be displayed.
int startAddress = 0; //Data start address on the DS1307
int rtc[4];
int ora, minut, zi, luna, an;
byte maxday = 0;

RTC_DS1307 ds1307;
SimpleTimer timer;               //Set up timer instance.
OneWire oneWire(ONE_WIRE_BUS);   //Start the oneWire bus
DallasTemperature sensors(&oneWire);  //Create Dallas Temperature library instance.
// arrays to hold device addresses
DeviceAddress sensors1, sensors2;

unsigned long starttime;
unsigned long endtime;

boolean bitState;
float temp1;  // value for read temperature (as float);
byte semn = 1;  // 1 for + and  for -
int t1 = 0;  // temperaturea as integer
int t2 = 0;  // temperaturea as integer
byte x = 0;  // for test value
byte p = 2; // brigthness
//int has;  //value for humidity

int maxim = 3000; // maxim brigghtness
int DISPLAY_BRIGHTNESS = 2000;
//Display brightness
//Each digit is on for a certain amount of microseconds
//Then it is off until we have reached a total of 20ms for the function call
//Let's assume each digit is on for 1000us
//Each digit is on for 1ms, there are 4 digits, so the display is off for 16ms.
//That's a ratio of 1ms to 16ms or 6.25% on time (PWM).
//Let's define a variable called brightness that varies from:
//5000 blindingly bright (15.7mA current draw per digit)
//2000 shockingly bright (11.4mA current draw per digit)
//1000 pretty bright (5.9mA)
//500 normal (3mA)
//200 dim but readable (1.4mA)
//50 dim but readable (0.56mA)
//5 dim but readable (0.31mA)
//1 dim but readable in dark (0.28mA)

#define SW0 A2   // pin for MENU/change
#define SW1 A3   // pin for increase value (+)

byte meniu = 0;   // 0 = usual state (clack, thetmometer & higrometer)
// 1 - read clock and data
// 2 - adjust hour
// 3 - adjust minutes
// 4 - adjus year
// 5 - adjust month
// 6 - adjust day of month
// 7 - store data

int luminamax = 3000;  // for control the brightness (maxim is 3000 for not see the flickering effect)
int lumina = 0;
#define senzorlumina A4 //  pin for photoresitor: VCC (+5VDC) ---|= 10k =|------ Analog pin (A3) ------|= LDR =|------| (GND)


byte test = 0; // 1 - test if work ok
//  0 - RTC clock and thermometer

byte tip = 1; // 0 = common cathode
// 1 = common anode

//--------------------------------------------------------------------------------------------------------

void setup()
{
  for (int i = 0; i < 4; i++)  //Set anode pins mode to output and put them LOW.
  {
    pinMode(anodes[i], OUTPUT);
    if (tip == 1) digitalWrite(anodes[i], LOW);
    else  digitalWrite(anodes[i], HIGH);
  }

  for (int i = 0; i < 8; i++)  //Set cathode pins mode to output and put them HIGH.
  {
    pinMode(cathodes[i], OUTPUT);
    if (tip == 1) digitalWrite(cathodes[i], HIGH);
    else digitalWrite(cathodes[i], LOW);
  }

  pinMode(SW0, INPUT);  // for this use a slide switch
  pinMode(SW1, INPUT);  // N.O. push button switch
  digitalWrite(SW0, HIGH); // pull-ups on
  digitalWrite(SW1, HIGH);

  Wire.begin();  // Start the I2C bus.
  ds1307.begin(); //start RTC Clock
  //ds1307.adjust(DateTime(__DATE__, __TIME__));  // sets the RTC to the date & time this sketch was compiled
  DateTime now = ds1307.now();

  sensors.begin();  //Start the Dallas Temperature library.
  // set the resolution to 9 bit per device
  sensors.setResolution(sensors1, precizie);
  sensors.setResolution(sensors2, precizie);

  if (!sensors.getAddress(sensors1, 0));
  if (!sensors.getAddress(sensors2, 1));
  readTime();
  lumina = 3 * analogRead(senzorlumina); // for control the brigntness
}
// --------------------------------------------------------------------------------------------

void loop()
{

  if (meniu == 0)  // normal state
  {
    readTime();  // read clock
    lumina = 3 * analogRead(senzorlumina); // for control the brigntness
    scrollIn(data);

    for (byte i = 0; i <= 10; i++) // show clock
    {
      lumina = 3 * analogRead(senzorlumina); // for control the brigntness
      starttime = millis();
      endtime = starttime;                    //Store the internal timer counter value to make this loop run for a set period.
      while ((endtime - starttime) <= 500) // do this loop for 5000mS
      {
        outputDisplay2(data, lumina);
        endtime = millis();                  //Read internal timer counter to see how long this loop has been running.
        if (!(digitalRead(SW0)))
        {
          meniu = 1; //go to menu for change hour
          i = 10;  // force exit in next menu
        }
      }
      starttime = millis();
      endtime = starttime;
      while ((endtime - starttime) <= 500) // do this loop for 5000mS
      {
        outputDisplay3(data, lumina);
        endtime = millis();                  //Read internal timer counter to see how long this loop has been running.
        if (!(digitalRead(SW0)))
        {
          meniu = 1; //go to menu for change hour
          i = 10;  // force exit in next menu
        }
      }
    }
    scrollOut(data);
  }
  if (meniu == 0)  // normal state
  {
    readTemp();  // read temperature sensor
    lumina = 3 * analogRead(senzorlumina); // for control the brigntness
    scrollIn(data);
    starttime = millis();
    endtime = starttime;                    //Store the internal timer counter value to make this loop run for a set period.
    while ((endtime - starttime) <= 2500) // do this loop for 5000mS
    {
      outputDisplay2(data, lumina);
      endtime = millis();                  //Read internal timer counter to see how long this loop has been running.
      if (!(digitalRead(SW0)))
      {
        meniu = 1; //go to menu for change hour
      }
    }
    scrollOut(data);
  }

  if (meniu == 0)  // normal state
  {
    readData();  // read data
    lumina = 3 * analogRead(senzorlumina); // for control the brigntness
    scrollIn(data);
    starttime = millis();
    endtime = starttime;                    //Store the internal timer counter value to make this loop run for a set period.
    while ((endtime - starttime) <= 2500) // do this loop for 5000mS
    {
      outputDisplay3(data, lumina); //Send to diaplay.
      endtime = millis();                  //Read internal timer counter to see how long this loop has been running.
      if (!(digitalRead(SW0)))
      {
        meniu = 1; //go to menu for change hour
      }
    }
    scrollOut(data);
  } // end meniu = 0 - normal state

  if (meniu == 1)  // read clock and data
  {
    DateTime now = ds1307.now();
    an = now.year() - 2000;
    luna = now.month();
    zi = now.day();
    ora = now.hour();
    minut = now.minute();
    meniu = 2;  // go to adjusting mode
  } // end read data and clock

  if (meniu == 2)  // change the hour
  {
    if (!digitalRead(SW1)) // set hours ++
    {
      ora++;
      if (ora > 23) ora = 0;
      delay(100);
    }
    data[0] = (ora / 10);
    data[1] = (ora % 10);
    data[2] = (minut / 10);
    data[3] = (minut % 10);
    starttime = millis();
    endtime = starttime;                    //Store the internal timer counter value to make this loop run for a set period.
    while ((endtime - starttime) <= 50) // do this loop for 5000mS
    {
      outputDisplay3(data, lumina);
      endtime = millis();
    }
    data[0] = 15;
    data[1] = 15;
    data[2] = (minut / 10);
    data[3] = (minut % 10);
    starttime = millis();
    endtime = starttime;                //Store the internal timer counter value to make this loop run for a set period.
    while ((endtime - starttime) <= 50) // do this loop for 5000mS
    {
      outputDisplay2(data, lumina);
      endtime = millis();
    }
    if (!(digitalRead(SW0)))
    {
      meniu = 3; //go to menu for change minute
      delay(500);
    }
  } // end menu for change hour

  if (meniu == 3)  // change minutes
  {
    if (!digitalRead(SW1)) // set minutes ++
    {
      minut++;
      if (minut > 59) minut = 0;
      delay(100);
    }
    data[0] = (ora / 10);
    data[1] = (ora % 10);
    data[2] = (minut / 10);
    data[3] = (minut % 10);
    starttime = millis();
    endtime = starttime;                    //Store the internal timer counter value to make this loop run for a set period.
    while ((endtime - starttime) <= 50) // do this loop for 5000mS
    {
      outputDisplay3(data, lumina);
      endtime = millis();
    }
    data[0] = (ora / 10);
    data[1] = (ora % 10);
    data[2] = 15;
    data[3] = 15;
    starttime = millis();
    endtime = starttime;                //Store the internal timer counter value to make this loop run for a set period.
    while ((endtime - starttime) <= 50) // do this loop for 5000mS
    {
      outputDisplay2(data, lumina);
      endtime = millis();
    }
    if (!(digitalRead(SW0)))
    {
      meniu = 4; //go to menu for change year
      delay(500);
    }
  } // end menu for change minutes

  if (meniu == 4)  // change year
  {
    if (!digitalRead(SW1)) // set year ++
    {
      an++;
      if (an < 18) an = 18;
      if (an > 49) an = 18;
      delay(100);
    }
    data[0] = 2;
    data[1] = 0;
    data[2] = (an / 10);
    data[3] = (an % 10);
    starttime = millis();
    endtime = starttime;                //Store the internal timer counter value to make this loop run for a set period.
    while ((endtime - starttime) <= 50) // do this loop for 5000mS
    {
      outputDisplay2(data, lumina);
      endtime = millis();
    }
    data[0] = 15;
    data[1] = 15;
    data[2] = 15;
    data[3] = 15;
    starttime = millis();
    endtime = starttime;                //Store the internal timer counter value to make this loop run for a set period.
    while ((endtime - starttime) <= 50) // do this loop for 5000mS
    {
      outputDisplay2(data, lumina);
      endtime = millis();
    }
    if (!(digitalRead(SW0)))
    {
      meniu = 5; //go to menu for change month
      delay(500);
    }
  } // end menu for change year

  if (meniu == 5)  // change month
  {
    if (!digitalRead(SW1)) // set month ++
    {
      luna++;
      if (luna > 12) luna = 1;
      delay(100);
    }
    data[0] = 15;
    data[1] = 15;
    data[2] = (luna / 10);
    data[3] = (luna % 10);
    starttime = millis();
    endtime = starttime;                    //Store the internal timer counter value to make this loop run for a set period.
    while ((endtime - starttime) <= 50) // do this loop for 5000mS
    {
      outputDisplay3(data, lumina);
      endtime = millis();
    }
    data[0] = 15;
    data[1] = 15;
    data[2] = 15;
    data[3] = 15;
    starttime = millis();
    endtime = starttime;                //Store the internal timer counter value to make this loop run for a set period.
    while ((endtime - starttime) <= 50) // do this loop for 5000mS
    {
      outputDisplay3(data, lumina);
      endtime = millis();
    }
    if (!(digitalRead(SW0)))
    {
      meniu = 6; //go to menu for change day
      delay(500);
    }
  } // end menu for change month

  if (meniu == 6)  // change the day
  {
    if (!digitalRead(SW1)) // set day ++
    {
      zi++;
      if (luna == 4 || luna == 6 || luna == 9 || luna == 11) { //30 days hath September, April June and November
        maxday = 30;
      }
      else {
        maxday = 31; //... all the others have 31
      }
      if (luna == 2 && an % 4 == 0) { //... Except February alone, and that has 28 days clear, and 29 in a leap year.
        maxday = 29;
      }
      if (luna == 2 && an % 4 != 0) {
        maxday = 28;
      }
      if (zi > maxday) zi = 1;
      delay(100);
    }
    data[0] = (zi / 10);
    data[1] = (zi % 10);
    data[2] = (luna / 10);
    data[3] = (luna % 10);
    starttime = millis();
    endtime = starttime;                    //Store the internal timer counter value to make this loop run for a set period.
    while ((endtime - starttime) <= 50) // do this loop for 5000mS
    {
      outputDisplay3(data, lumina);
      endtime = millis();
    }
    data[0] = 15;
    data[1] = 15;
    data[2] = (luna / 10);
    data[3] = (luna % 10);
    starttime = millis();
    endtime = starttime;                //Store the internal timer counter value to make this loop run for a set period.
    while ((endtime - starttime) <= 50) // do this loop for 5000mS
    {
      outputDisplay3(data, lumina);
      endtime = millis();
    }
    if (!(digitalRead(SW0)))
    {
      meniu = 7; //go to store datas
      delay(500);
    }
  } // end menu for change hour

  if (meniu == 7)  // store datas
  {
    ds1307.adjust(DateTime(2000 + an, luna, zi, ora, minut, 0));
    meniu = 0; //go to normal state
    delay(500);
  } // end menu for change hour



}  // end main loop

// ----------------------------------------------------------------------------------

void outputDigit(int seg)    //Outputs segment data for an individual digit.
{
  for (int s = 0; s < 7; s++)  //Read a bit at a time from the selected digit in the digits array and output it to the correct
  { //pin
    if (tip == 1) bitState = bitRead(digits[seg], s);  //Read the current bit.
    else bitState = 1 - bitRead(digits[seg], 6 - s); //Read the current bit.
    digitalWrite(cathodes[s], bitState);         //and output it.
  }
}


void readTemp()
{
  if (test == 0)  // real
  {
    sensors.requestTemperatures();  //Request temperature from the sensor.
    temp1 = sensors.getTempC(sensors1);  //Read temperature from the first (only) sensor on the bus.
    t1 = (int) temp1;                         //Convert the temperature to int for display
    Interior();
    int starttime3 = millis();
    int endtime3 = starttime3;
    scrollIn(data);

    while ((endtime3 - starttime3) <= 800) // do this loop for 5000mS
    {
      outputDisplay2(data, lumina);          //Send to diaplay.
      endtime3 = millis();                  //Read internal timer counter to see how long this loop has been running.
    }
    scrollOut(data);
  }
  else  // test
  {
    x = x + 1;
    if (x > 3) x = 0;
    temp1 = te[x];
  }

  if (temp1 >= 0) semn = 1;  // if temperature is positive
  else
  {
    semn = 0;
    temp1 = -temp1;  // made positive value
  }
  t1 = temp1;                         //Convert the temperature to int for display

  if (semn == 1)
  {
    if (t1 >= 10)
    {
      data[0] = (t1 / 10);                  //Calculate and store temperature 10s value
      data[1] = (t1 % 10);                  //Calculate and store temperature 1s value.
      data[2] = 10;                        //Degree symbol
      data[3] = 11;                        //Letter C for Celcius
    }
    else
    {
      data[0] = 15;                        //none
      data[1] = t1;                       //number.
      data[2] = 10;                        //Degree symbol
      data[3] = 11;                        //Letter C for Celcius
    }
  }

  if (semn == 0)
  {
    if (t1 >= 10)
    {
      data[0] = 12;                        // - (negative sign)
      data[1] = (t1 / 10);                  //Calculate and store temperature 10s value
      data[2] = (t1 % 10);                  //Calculate and store temperature 1s value.
      data[3] = 10;                        //Degree symbol (Celcius)
    }
    else
    {
      data[0] = 12;                        //negative sign
      data[1] = t1;                       //number.
      data[2] = 10;                        //Degree symbol
      data[3] = 11;                        //Letter C for Celsius)
    }
  }
}


void readTime()
{
  if (test == 0)  // real
  {
    DateTime now = ds1307.now();
    rtc[3] = now.hour();
    rtc[4] = now.minute();
    data[0] = (rtc[3] / 10);
    data[1] = (rtc[3] % 10);
    data[2] = (rtc[4] / 10);
    data[3] = (rtc[4] % 10);
  }
  else  // test
  {
    //  data[0] = 1;
    data[0] = 0;
    data[1] = 2;
    data[2] = 5;
    data[3] = 6;
  }
  if (data[0] == 0) data[0] = 15; // none

}

void readData()
{
  if (test == 0)  // real
  {
    DateTime now = ds1307.now();
    rtc[1] = now.month();
    rtc[2] = now.day();
    rtc[0] = now.year();
    data[0] = (rtc[2] / 10);
    data[1] = (rtc[2] % 10);
    data[2] = (rtc[1] / 10);
    data[3] = (rtc[1] % 10);
  }
  else
  {
    data[0] = 2;
    data[1] = 3;
    data[2] = 1;
    data[3] = 2;
  }
}


void Interior()
{ data[0] = 1;
  data[1] = 17;
  data[2] = 16;
  data[3] = 15;
}

void Exterior()
{ data[0] = 0;
  data[1] = 14;
  data[2] = 16;
  data[3] = 15;
}

void scrollIn(int sDig[4])    //Scrolls data on to the display from blank.
{
  int scrollBuffer[8];        //Stores the entire set of data that will be shifted along the display.
  scrollBuffer[0] = 15;       //15 represents a blank digit.
  scrollBuffer[1] = 15;
  scrollBuffer[2] = 15;
  scrollBuffer[3] = 15;
  scrollBuffer[4] = sDig[0];
  scrollBuffer[5] = sDig[1];
  scrollBuffer[6] = sDig[2];
  scrollBuffer[7] = sDig[3];

  for (int st = 0; st < 4; st++) //Loop 4 times and read from next entry in the array each time.
  {
    int sData[4] = {scrollBuffer[(0 + st)], scrollBuffer[(1 + st)], scrollBuffer[(2 + st)], scrollBuffer[(3 + st)]};
    for (int sc = 0; sc < (DISPLAY_BRIGHTNESS * 10) / (DISPLAY_BRIGHTNESS + lumina); sc++) //Refresh the display 10 times after each shift to give the illusion of scrolling.
    {
      outputDisplay2(sData, lumina);
    }
  }
}

void scrollOut(int sDig[4])   //Scrolls data off the display to blank
{
  int scrollBuffer[8];        //Stores the entire set of data that will be shifted along the display.
  scrollBuffer[0] = sDig[0];
  scrollBuffer[1] = sDig[1];
  scrollBuffer[2] = sDig[2];
  scrollBuffer[3] = sDig[3];
  scrollBuffer[4] = 15;        //15 represents a blank digit.
  scrollBuffer[5] = 15;
  scrollBuffer[6] = 15;
  scrollBuffer[7] = 15;

  for (int st = 0; st < 4; st++) //Loop 4 times and read from next entry in the array each time.
  {
    int sData[4] = {scrollBuffer[(0 + st)], scrollBuffer[(1 + st)], scrollBuffer[(2 + st)], scrollBuffer[(3 + st)]};
    for (int sc = 0; sc < (DISPLAY_BRIGHTNESS * 10) / (DISPLAY_BRIGHTNESS + lumina); sc++) //Refresh the display 10 times after each shift to give the illusion of scrolling.
    {
      outputDisplay2(sData, lumina);
    }
  }
}

void outputDisplay2(int dig[4], float dilei)        //Scan the display once with the 4 digit int array passed in.
{
  for (int d = 0; d < 4; d++)
  {
    outputDigit(dig[d]);                //Set up the segment pins with the correct data.
    if (tip == 1) digitalWrite(anodes[d], HIGH);      //Turn on the digit.
    else digitalWrite(anodes[d], LOW);      //Turn on the digit.
    //  delay(p);   //Hold it on for 2ms to improve brightness.
    if (tip == 1) digitalWrite(cathodes[7], 1);
    else digitalWrite(cathodes[7], 0);
    delayMicroseconds(DISPLAY_BRIGHTNESS);

    if (tip == 1) digitalWrite(anodes[d], LOW);       //And turn it off again before looping untl all digits have been displayed.
    else digitalWrite(anodes[d], HIGH);       //And turn it off again before looping untl all digits have been displayed.
    delayMicroseconds(dilei);
  }
}

void outputDisplay3(int dig[4], float dilei)        //Scan the display once with the 4 digit int array passed in.
{
  for (int d = 0; d < 4; d++)
  {
    outputDigit(dig[d]);                //Set up the segment pins with the correct data.
    if (d == 1)
    {
      if (tip == 1) digitalWrite(cathodes[7], 0);
      else digitalWrite(cathodes[7], 1);
    }
    else
    {
      if (tip == 1) digitalWrite(cathodes[7], 1);
      else digitalWrite(cathodes[7], 0);
    }
    if (tip == 1) digitalWrite(anodes[d], HIGH);      //Turn on the digit.
    else digitalWrite(anodes[d], LOW);      //Turn on the digit.
    //  delay(p);                           //Hold it on for 2ms to improve brightness.
    delayMicroseconds(DISPLAY_BRIGHTNESS);

    if (tip == 1)  digitalWrite(anodes[d], LOW);       //And turn it off again before looping untl all digits have been displayed.
    else digitalWrite(anodes[d], HIGH);       //And turn it off again before looping untl all digits have been displayed.
    delayMicroseconds(dilei);
  }
}
