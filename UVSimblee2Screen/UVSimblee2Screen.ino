


#include <SimbleeNonBLE.h>
#include <SimbleeForMobile.h>
#include "YellowDot.h"
#define YELLOW_DOT_IMAGE 1
#define DRAKGREEN rgb(6,39,54)
//
// The ID of the button which displays screen 2
//
int toScreen2ButtonID;
void printEvent(event_t &event);
int UVpin = 3;
int LEDpin = 13;
//
// The ID of the button which displays screen 1
//
int toScreen1ButtonID;


int toScreen3ButtonID;

//
// The ID of the current screen being displayed
//
int currentScreen;

uint8_t uvtext;
uint8_t uvTitle;
uint8_t airtext;
uint8_t airTitle;
uint8_t tempText;
uint8_t UVadvice1;
uint8_t UVadvice2;
uint8_t uvacuText;
unsigned long lastUpdated = 0;
unsigned long updateRate = 1000; // second
asm(".global _printf_float");
int UVstate = 0;
float acu = 0.0;
float timelim = 0;
float totaltime = 0;
float intensitylim = 0.5;
int interval = 0;
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max);
/*
   Traditional Arduino setup routine

   Initialize the SimbleeForMobile environment.
*/
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Setup beginning");

  SimbleeForMobile.deviceName = "Exposia v1.1";
  SimbleeForMobile.advertisementData = "UVandAir";

  // use a shared cache
  SimbleeForMobile.domain = "temp.simblee.com";

  // Begin Simble UI
  SimbleeForMobile.begin();
  pinMode(13,OUTPUT);

  Serial.println("Setup completed");
}

/*
   The traditional Arduino loop method

   Enable SimbleeForMobile functionality by calling the process method
   each time through the loop. This method must be called regularly for
   functionality to work.
*/
void loop() {
  unsigned long loopTime = millis();


  if (updateRate < (loopTime - lastUpdated))
  {
    lastUpdated = loopTime;

    float temp = analogRead(4) / 1023 * 3.3 ;
    float uvIntensity = mapfloat(temp, 0.99, 2.8, 0.0, 15.0) + Simblee_temperature(CELSIUS) / 3.6;
    if(uvIntensity < 0) uvIntensity = 0;

    // requires newlib printf float support
    char buf[16];
    sprintf(buf, "%.02f", uvIntensity);

    temp = analogRead(5) / 1023 * 3.3;
    float airmass = temp;

    char buf2[16];
    sprintf(buf2, "%.02f", airmass);

    temp = Simblee_temperature(CELSIUS);
    char buf3[16];
    sprintf(buf3, "%.02f", temp);


    char buf4[16];
    sprintf(buf4, "%.02f", acu);


    // base everything on the first sample / ambient temperature
    if (uvIntensity > 0.4 || interval > 1) {
      Serial.println("The UV is too strong");
      UVstate = 1;
      timelim = -12.9736 * uvIntensity * uvIntensity * uvIntensity + 85.2522 * uvIntensity * uvIntensity - 190.9085 * uvIntensity + 175.7416;
      acu = acu + 5 * 1 / timelim;

      sprintf(buf4, "%.02f", acu);
      if (uvIntensity > intensitylim && acu > 0.5 || uvIntensity > intensitylim * 3) {   //Testing
        // turn LED on:
        interval = 5;
        UVstate = 2;
        digitalWrite(13,HIGH);
      }


      if (uvIntensity < intensitylim && acu > 0.5)
      {
        interval = interval - 1;
      }
    }

    if (uvIntensity <= 0.4) UVstate = 0;

    if (SimbleeForMobile.updatable && currentScreen == 1)
    {
      // update the text first with the actual temp
      SimbleeForMobile.updateText(uvtext, buf);
      SimbleeForMobile.updateText(airtext, buf2);
      SimbleeForMobile.updateText(tempText, buf3);
      SimbleeForMobile.updateText(uvacuText, buf4);
      Serial.print("Update\n");
    }

    if (SimbleeForMobile.updatable && currentScreen == 2)
    {
      switch (UVstate) {
        case 1:
          SimbleeForMobile.updateText(UVadvice2, "UV seems strong!");
          SimbleeForMobile.updateText(UVadvice1, "Be Protected!");
          break;
        case 2:
          SimbleeForMobile.updateText(UVadvice2, "UV seems strong!");
          SimbleeForMobile.updateText(UVadvice1, "Get Indoor!");
          SimbleeForMobile.updateColor(UVadvice1, RED);
          break;
        default:
          Serial.print("ui: Uknown screen requested: ");
      }
    }
  }
  // process must be called in the loop for SimbleeForMobile
  SimbleeForMobile.process();
}


/*
   Callback when a Central connects to this device

   Reset the current screen to non being displayed
*/
void SimbleeForMobile_onConnect()
{
  currentScreen = -1;
}

/*
   Callback when a Central disconnects from this device

   Not used in this sketch. Could clean up any resources
   no longer required.
*/
void SimbleeForMobile_onDisconnect()
{
}

/*
   SimbleeForMobile ui callback requesting the user interface

   Check which screen is being requested.
   If that is the current screen, simply return.
   If it is a different screen, create the requested screen.
   A screen request which is out of range, is logged and ignored.
*/
void ui()
{
  if (SimbleeForMobile.screen == currentScreen) return;

  currentScreen = SimbleeForMobile.screen;
  switch (SimbleeForMobile.screen)
  {
    case 1:
      createScreen1();
      break;

    case 2:
      createScreen2();
      break;

    case 3:
      createScreen3();
      break;

    default:
      Serial.print("ui: Uknown screen requested: ");
      Serial.println(SimbleeForMobile.screen);
  }
}

/*
   SimbleeForMobile event callback method

   An event we've registered for has occur. Process the event.
   The only events of interest are EVENT_RELEASE which signify a button press.
   If the button to open the screen not being displayed is pressed, request
   to show that screen.
*/
void ui_event(event_t &event)
{
  printEvent(event);
  if (event.id == toScreen1ButtonID && event.type == EVENT_RELEASE && currentScreen == 2)
  {
    SimbleeForMobile.showScreen(1);
  } else if (event.id == toScreen2ButtonID && event.type == EVENT_RELEASE && currentScreen == 1)
  {
    SimbleeForMobile.showScreen(2);

  } else if (event.id == toScreen3ButtonID && event.type == EVENT_RELEASE && currentScreen == 1)
  { SimbleeForMobile.showScreen(3);
  }
  else if (event.id == toScreen1ButtonID && event.type == EVENT_RELEASE && currentScreen == 3)
  {
    SimbleeForMobile.showScreen(1);
  }
}

/*
   Create the first screen.

   The screen simply consists of a label displaying the screen number
   and a button which swaps screens. Register for events on the button
   such that we receive notifications when it is pressed.
*/
void createScreen1()
{
  //
  // Portrait mode is the default, so that isn't required
  // here, but shown for completeness. LANDSCAPE could be
  // used for that orientation.
  //
  SimbleeForMobile.beginScreen(CLEAR, PORTRAIT);
  SimbleeForMobile.drawRect(0, 0, 320, 568, rgba(112, 154, 170, 0), rgba(205, 140, 149, 0));

  SimbleeForMobile.drawRect(0, 310, 320, 5, DRAKGREEN);
  SimbleeForMobile.drawRect(0, 322 - 5, 320, 2, DRAKGREEN);
  SimbleeForMobile.drawRect(50, 107, 220, 60, DRAKGREEN);
  SimbleeForMobile.drawRect(50, 387, 220, 60, DRAKGREEN);

  SimbleeForMobile.drawRect(10, 40, 300, 2, DRAKGREEN);
  SimbleeForMobile.drawRect(10, 254, 300, 2, DRAKGREEN);
  SimbleeForMobile.drawRect(10, 40, 2, 216, DRAKGREEN);
  SimbleeForMobile.drawRect(308, 40, 2, 216, DRAKGREEN);

  SimbleeForMobile.drawRect(10, 338, 300, 2, DRAKGREEN);
  SimbleeForMobile.drawRect(10, 468, 300, 2, DRAKGREEN);
  SimbleeForMobile.drawRect(10, 338, 2, 132, DRAKGREEN);
  SimbleeForMobile.drawRect(308, 338, 2, 132, DRAKGREEN);


  int textID = SimbleeForMobile.drawText(106, 247, "Current", DRAKGREEN, 70);
  toScreen2ButtonID = SimbleeForMobile.drawButton(50, 510, 100, "Advice", DRAKGREEN);
  toScreen3ButtonID = SimbleeForMobile.drawButton(170, 510, 100, "Setting", DRAKGREEN);
  if (SimbleeForMobile.remoteDeviceType == REMOTE_DEVICE_OS_IOS) {
    SimbleeForMobile.drawText(200, 130, "mV/cm2", WHITE);
  } else if (SimbleeForMobile.remoteDeviceType == REMOTE_DEVICE_OS_ANDROID) {
    SimbleeForMobile.drawText(270, 248, "\xc2\xb0" "C", BLUE);
  } else if (SimbleeForMobile.remoteDeviceType == REMOTE_DEVICE_OS_UNKNOWN) {
    SimbleeForMobile.drawText(265, 248, "C", BLUE);
  }

  SimbleeForMobile.drawText(80, 295, "\xb0" "C", BLACK);


  uvtext = SimbleeForMobile.drawText(110, 110, "", WHITE, 50);
  uvacuText = SimbleeForMobile.drawText(142, 210, "", rgb(106, 58, 59), 25);
  uvTitle = SimbleeForMobile.drawText(52, 52, "UVIntensity", DRAKGREEN, 47);
  SimbleeForMobile.drawText(67, 190, "UV DAMAGE DOSE", rgb(106, 58, 59), 25);

  airtext = SimbleeForMobile.drawText(110, 390, "", WHITE, 50);
  airTitle = SimbleeForMobile.drawText(13, 333, "AirPollution", DRAKGREEN, 40);

  tempText = SimbleeForMobile.drawText(0, 283 - 5, "", DRAKGREEN, 35);

  SimbleeForMobile.setEvents(toScreen2ButtonID, EVENT_RELEASE);
  SimbleeForMobile.setEvents(toScreen3ButtonID, EVENT_RELEASE);
  SimbleeForMobile.endScreen();
}

/*
   Create the second screen.

   The screen simply consists of a label displaying the screen number
   and a button which swaps screens. Register for events on the button
   such that we receive notifications when it is pressed.
*/
void createScreen2()
{
  //
  // Default to Portrait orientation
  //
  SimbleeForMobile.beginScreen(WHITE);
  SimbleeForMobile.drawRect(0, 0, 320, 568, rgba(112, 154, 170, 0), rgba(205, 140, 149, 0));
  SimbleeForMobile.imageSource(YELLOW_DOT_IMAGE, YellowDot_Type, YellowDot, YellowDot_Len);
  int titleLeftMargin = 45;
  int titleTopMargin = 60;

  int leftMargin = 25;
  int topMargin = 120;
  int deltaY = 40;
  int repeatX = 7;
  int repeatY = 7;

  SimbleeForMobile.drawText(titleLeftMargin - 20, titleTopMargin, "Advice Screen", DRAKGREEN, 50);


  SimbleeForMobile.drawImage(YELLOW_DOT_IMAGE, leftMargin, topMargin, 7, 7);


  UVadvice1 = SimbleeForMobile.drawText(10, 430, "Feel Free to go outdoor~", BLUE, 20);
  UVadvice2 = SimbleeForMobile.drawText(10, 400, "UV seems GOOD", BLUE, 20);

  //int textID = SimbleeForMobile.drawText(50, 30, "Image", BLACK, 40);
  toScreen1ButtonID = SimbleeForMobile.drawButton(170, 510, 100, "Current", DRAKGREEN);

  SimbleeForMobile.setEvents(toScreen1ButtonID, EVENT_RELEASE);
  SimbleeForMobile.endScreen();
}

void createScreen3() {

  Serial.println("creating Slider Stepper screen");

  int titleLeftMargin = 30;
  int titleTopMargin = 60;

  int leftMargin = 30;
  int topMargin = 120;
  int sliderLeftMargin = 50;
  int sliderWidth = 150;
  int deltaY = 50;
  int textSize = 20;
  int stepperWidth = 100;
  int stepperLeftMargin = 50;

  unsigned long backgroundColor = 0xffffffff;

  //
  // Default to Portrait orientation
  //
  SimbleeForMobile.beginScreen(backgroundColor);
  SimbleeForMobile.drawRect(0, 0, 320, 568, rgba(112, 154, 170, 0), rgba(205, 140, 149, 0));
  SimbleeForMobile.drawText(titleLeftMargin, titleTopMargin, "Setting", BLACK, 50);

  SimbleeForMobile.drawText(leftMargin - 20, topMargin, "Ages: ");
  uint8_t xSlider = SimbleeForMobile.drawSlider(sliderLeftMargin, topMargin, sliderWidth, 0, 100);
  uint8_t xSliderText = SimbleeForMobile.drawText(leftMargin + sliderLeftMargin + sliderWidth, topMargin, "0");
  topMargin += deltaY;

  SimbleeForMobile.drawText(leftMargin - 20, topMargin, "Skin: ");
  uint8_t ySlider = SimbleeForMobile.drawSlider(sliderLeftMargin, topMargin, sliderWidth, 0, 100, YELLOW);
  uint8_t ySliderText = SimbleeForMobile.drawText(leftMargin + sliderLeftMargin + sliderWidth, topMargin, "0");

  topMargin += 2 * deltaY;
  SimbleeForMobile.drawText(leftMargin, topMargin, "Stepper range (0 - 10)");
  topMargin += deltaY;
  uint8_t stepper1 = SimbleeForMobile.drawStepper(leftMargin, topMargin, stepperWidth, 0, 10);
  SimbleeForMobile.updateValue(stepper1, 5);
  uint8_t stepper1Text = SimbleeForMobile.drawText(leftMargin + stepperLeftMargin + stepperWidth, topMargin, "5");

  topMargin += deltaY;
  SimbleeForMobile.drawText(leftMargin, topMargin, "Stepper range (-10 - 10)");
  topMargin += deltaY;
  uint8_t stepper2 = SimbleeForMobile.drawStepper(leftMargin, topMargin, stepperWidth, -10, 10, YELLOW);
  uint8_t stepper2Text = SimbleeForMobile.drawText(leftMargin + stepperLeftMargin + stepperWidth, topMargin, "0");

  toScreen1ButtonID = SimbleeForMobile.drawButton(170, 510, 100, "Home", DRAKGREEN);

  SimbleeForMobile.setEvents(toScreen1ButtonID, EVENT_RELEASE);
  SimbleeForMobile.endScreen();


  Serial.println("Slider Stepper screen created");
}

/*
   Utility method to print information regarding the given event
*/
void printEvent(event_t &event)
{
  Serial.println("Event:");
  Serial.print("  ID: ");
  Serial.println(event.id);

  Serial.print("   Type: ");
  Serial.println(event.type);

  Serial.print("   Value: ");
  Serial.println(event.value);

  Serial.print("   Text: ");
  Serial.println(event.text);

  Serial.print("   Coords: ");
  Serial.print(event.x);
  Serial.print(",");
  Serial.println(event.y);
}

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


