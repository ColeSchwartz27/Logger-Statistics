
// LoggerStatistics Arduino Sketch
//  enables datalogging of sensor data with arbitrary number of sensor
//    and recording events (integers representing states of various buttons or thresholds)
//
//  INSTRUCTIONS:
//  ********************** FOR ADDING A NEW DATA STREAM ****************************
//  (could come directly from a sensor or be a calculated value)
//  STEPS or adding a new data stream:
//  DATA_1 - define pins, device parameters, and compiler macro definitions in "deviceConfig[NAME].h" file
//  DATA_2 - include any libraries needed for sensors (or other devices) with suitable macro logic as needed
//  DATA_3 - declare int variable (i[SHORTNAME]) for storing index of each data stream
//  DATA_4 - initialize and configure data streams and initialize sensors
//  DATA_5 - update data stream with current sensor or calculated values
//  data stream statistics are automatically output to Serial or written to SD File periodically through settings
//
//  ********************** FOR ADDING A NEW EVENT *********************************
//  STEPS for adding a new event:
//  EVENT_1 - define event pins, event parameters, and compiler macro definitions in "deviceConfig[NAME].h" file
//  EVENT_2 - include any libraries related to event with suitable macro logic as needed
//  EVENT_3 - declare int variable (j[SHORTNAME]) for storing index of each event
//  EVENT_4 - initialize and configure event
//  EVENT_5 - update event
//  EVENT_6 - EVENT if event relates to an EVENT, output event to Serial and/or Event log SD File
//  EVENT_7 - ACTION if event and event can cause an action, implement action
//
// comment out next line to eliminate debug messages
#define ENABLEDEBUG
#include "quickDebugMessages.h"
// use DEBUG_LEVEL to control how verbose debugging messages are
#define DEBUG_LEVEL 4

// *************************************
// include files that define device configuration
#include "secretsGeneric.h"
// choose correct device configuration
//#include "deviceConfigGeneric.h"
#include "deviceConfigAAdalogger.h"
//#include "deviceConfigBEInk.h"

// *************** DATA_2: SENSOR LIBRARIES ********************************************
// * INCLUDE sensor library include files below (with suitable macro logic as needed)
// * also declare variables if needed for using the sensor
// * ***********************************************************************************
// sensor LIBRARIES for ADAFRUIT FEATHER BLUEFRUIT SENSE
//#include <Adafruit_APDS9960.h>
#ifdef ENABLE_SENSE_ALTIM
#include <Adafruit_BMP280.h>
#endif
#ifdef ENABLE_SENSE_MAG
#include <Adafruit_LIS3MDL.h>
#endif
#ifdef ENABLE_SENSE_ACCEL
#include <Adafruit_LSM6DS33.h>
#endif
#ifdef ENABLE_SENSE_HUMID
#include <Adafruit_SHT31.h>
#endif
#include <Adafruit_Sensor.h>
//#include <PDM.h>
// VARIABLES Adafruit Feather Sense Sensors:
//Adafruit_APDS9960 apds9960; // proximity, light, color, gesture
#ifdef ENABLE_SENSE_ALTIM
Adafruit_BMP280 bmp280; // temperature, barometric pressure
#endif
#ifdef ENABLE_SENSE_MAG
Adafruit_LIS3MDL lis3mdl; // magnetometer
#endif
#ifdef ENABLE_SENSE_ACCEL
Adafruit_LSM6DS33 lsm6ds33; // accelerometer, gyroscope
#endif
#ifdef ENABLE_SENSE_HUMID
Adafruit_SHT31 sht30; // humidity
#endif
//float temperature, pressure, altitude;
//float altitudeBaseline, altitudeBaselineStDev;
//float magnetic_x, magnetic_y, magnetic_z;
//float accel_x, accel_y, accel_z;
//float gyro_x, gyro_y, gyro_z;
//float humidity;

// *************** EVENT_2: EVENT LIBRARIES ********************************************
// * INCLUDE library include files below (with suitable macro logic as needed)
// * also declare variables if needed for using the event
// * ***********************************************************************************
#ifdef ENABLE_NEOPIXEL
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel pixels(1, SENSE_NEO, NEO_GRB + NEO_KHZ800);
int mode = 0;
int pixModeMax = 7;
#endif

// include routines for datalogging to SD card
#include "logSD.h"

// ********************************************************************
// data structure for storing data samples and calculating statistics
#include "sampleStats.h"
// array of data structures for storing data from sensors

// *************** DATA_3: DATA STREAM INDICES ********************************************
// * DECLARE int variables for storing index of each data stream (it is OK if it is not used)
// * ***********************************************************************************
// sensor indices to store where sensor data is stored in the array
// FAST sensors are probed every time through the loop() function
int iTime = -1;     // time at which data values are recorded (store in seconds)
int iLoopTime = -1; // time spent during one pass through loop() function (store in ms)
int iSimX = -1;     // simulated data value
int iSimY = -1;     // simulated data value (sine function)
int iAx = -1;       // acceleration in x direction (long dimension of feather)
int iAy = -1;       // acceleration in y direction (short dimension of feather)
int iAz = -1;       // acceleration in z direction (perpendicular to feather surface)
int iGx = -1;       // gyro in x direction (long dimension of feather)
int iGy = -1;       // gyro in y direction (short dimension of feather)
int iGz = -1;       // gyro in z direction (perpendicular to feather surface)
int iMx = -1;       // magnetic field in x direction (long dimension of feather)
int iMy = -1;       // magnetic field in y direction (short dimension of feather)
int iMz = -1;       // magnetic field in z direction (perpendicular to feather surface)
int iAlt = -1;      // altitude from barometric pressure
int iTemp = -1;     // temperature (from humidity sensor)
int iHumid = -1;    // humidity

// data structure for tracking control events (from buttons, thresholds of data values, etc)
#include "eventTracker.h"
// *************** EVENT_3: EVENT STREAM INDICES ********************************************
// * DECLARE int variables for storing index of each event (it is OK if it is not used)
// * ***********************************************************************************
// define events to be used by the code for controlling various actions
int jUserButton = -1; // user button on Adafruit Sense
int jTopSwitch = -1;  // top switch on ling climber
int jBotSwitch = -1;  // bottom switch on line climber
int jPitch = -1;      // threshold on Ax
int jRoll = -1;       // threshold on Ay
int jTimer = -1;      // labels on CPU time
//int jNoseUp = -1;     // threshold on Ax
//int jNoseDown = -1;   // threshold on Ax
#ifdef ENABLE_NEOPIXEL
int jNeoPixel = -1; // neopixel state
#endif

int countSDLine = 0; // number of lines in SD data file
int countEvents = 0; // number of lines in SD event file

// ********************************************************************
// functions that simulate sensors with randomness
#include "simulatedSensor.h"
// *****************************

// variables for tracking time spent in functions
unsigned long endTime = 0;
unsigned long lastEndTime = 0;
unsigned long startTime = 0;
unsigned long startTimeMicros = 0;

unsigned long serialInterval = SERIAL_OUTPUT_INTERVAL; // time interval between serial output
unsigned long nextSerialOutput = 0;                    // time at which to end sampling and write out data
unsigned long samplingInterval = SAMPLING_PERIOD;      // time interval for collecting samples before next SampleOutput
unsigned long nextSampleOutput = 0;                    // time at which to end sampling and write out data
unsigned long slowDataInterval = SLOW_DATA_INTERVAL;   // time interval between updating slow data streams
unsigned long nextSlowDataUpdate = 0;                  // time at which next set of slow data streams will be updated

// varibales for controlling LED signals
unsigned long LEDPhaseInterval = 2000; // long toggle between colors to indicate phase of operation and signal status
unsigned long LEDSDInterval = 100;     // blink quickly to indicate that SD was accessed
int LEDPhaseState = 0;                 // toggle 0 or 1
int LEDPhaseUp = 1;                    // code for color when in active state (1)
int LEDPhaseDown = -1;                 // code for color when in passive state (0)
int LEDSDActive = 0;
int LEDSDColor = 5; // code for color when SD is activated
int LEDLevel = 40;  // brightness of LED
unsigned long nextLEDPhaseChange = 0;
unsigned long nextSDPhaseChange = 0;

// timing variables for calculating trendline slope
unsigned long timeReference = 0; // for trendline, subtract this time to calculate relative time
unsigned long timeRelative = 0;  // relative time (in millis) for calculating trendline

// **************************************************************************
// **************************************************************************
// * SETUP: start of setup() function
// **************************************************************************
void setup()
{

#ifdef ENABLE_NEOPIXEL
  pixels.begin();                 // INITIALIZE NeoPixel strip object (REQUIRED)
  pixelSet(LEDPhaseUp, LEDLevel); // SET to red LED for startup Stage
#endif

  Serial.begin(115200);
  // remove following while block for field testing!!
#ifdef WAIT_FOR_SERIAL
  while (!Serial)
  {
    delay(10); // wait for serial monitor to open
  }
#endif

  Serial.println("Starting DataloggerStats test program");
  Serial.print("Filename = ");
  Serial.println(__FILE__);
  Serial.print("Date and Time Compiled = ");
  Serial.print(__DATE__);
  Serial.print(" : ");
  Serial.println(__TIME__);

  // -----------------------------------------------------------
  // - setup the files for output to SD cards
  // -----------------------------------------------------------
#ifdef USE_SD
  // determine a directory for storing files by date using "dYYMMDD"
  // e.g. "d201222" for December 22, 2020
  initializeSDFileDirectory();

  // naming convention for files = "Dtype##.suffix" where "D" is the device code (1 char),
  //    "type" is the type of file (3-4 char), "##" is the file number, and ".suffix" is the appropriate file suffix
  // Zlog01.txt = create a log file (for mirroring messages to serial)
  int statusSD = setup_SD_file(deviceCode, "log", ".txt", logFileName);
  statusSD = setup_SD_file(deviceCode, "data", ".csv", dataFileName);
  statusSD = setup_SD_file(deviceCode, "evnt", ".csv", eventFileName);

  pinMode(SENSE_BLUE, OUTPUT);
  digitalWrite(SENSE_BLUE, LOW);
  if (statusSD == 1)
  {
    LEDSDActive = 1;
    // update neopixel LED
    nextSDPhaseChange = millis() + LEDSDInterval;
    pixelSet(LEDSDColor, LEDLevel);

    // update status LED
    //pinMode(SENSE_BLUE, OUTPUT);
    digitalWrite(SENSE_BLUE, HIGH);
  }
  else
  {
    LEDSDActive = 1;
    // update neopixel LED
    nextSDPhaseChange = millis() + LEDSDInterval;
    pixelSet(1, LEDLevel); // turn to red
  }

  // Zdata01.csv = create a data file (streaming sample data at regular time intervals)

  // Zevnt01.txt = create an event file (recording time and messages for specific events to guide data analysis)

#endif
  // ---END of SD setup---------------------------------------------------

  // *************** DATA_4: INITIALIZE DATA STREAMS ********************************************
  // * INITIALIZE and configure each data stream
  // * also make sure to initialize the sensor if needed
  // * ***********************************************************************************
  // ----------------------------------------------------------------
  // -  INITIALIZE data streams that will be collected and recorded
  // -  should also initialize the sensors as needed
  // ----------------------------------------------------------------
  // initialize a new data source (*** need to move this to a function to automate)
  // iTime is index for the time of data point collection increments (n increments make the interval for the sample)
  // variable indicating what stats to output to spreadsheets
  // -1 = no output (just a variable for internal calculations)
  // 0  = only output current value (no statistics)
  // 1  = only output average
  // 2  = output average and current
  // 3  = output average and sample size
  // 4  = output average and standard deviation
  // 5  = output all info (including current and sample size)

  iTime = addDataStream(data, &nSamples, "CPUTimeInms", "CPUt", "s", 2);
  DEBUG(iTime)

  // iLoopTime is index for the time spent in the loop function
  iLoopTime = addDataStream(data, &nSamples, "LoopTimeInterval", "loopt", "ms", 5);
  DEBUG(iLoopTime)

#ifdef ENABLE_SIMULATED_DATA
  // iSimX is index for the simulated sensor variable
  iSimX = addDataStream(data, &nSamples, "SimulatedSensor", "xSim", "arb", 4);
  data[iSimX].calcTrendline = 1;
  DEBUG(iSimX)

  // iSimY is index for the simulated sensor variable
  iSimY = addDataStream(data, &nSamples, "SimulatedSensorSine", "ySim", "arb", 4);
  data[iSimY].calcTrendline = 1;
  DEBUG(iSimY)
#endif

#ifdef ENABLE_SENSE_ACCEL
  // iAx, iAy, iAz accelerometer sensor readings
  lsm6ds33.begin_I2C(); // initialize accelerometer / gyro
  iAx = addDataStream(data, &nSamples, "Accel in x", "Ax", "m/s^2", 4);
  //data[iAx].calcTrendline = 1;
  DEBUG(iAx)
  iAy = addDataStream(data, &nSamples, "Accel in y", "Ay", "m/s^2", 4);
  //data[iAy].calcTrendline = 1;
  DEBUG(iAy)
  iAz = addDataStream(data, &nSamples, "Accel in z", "Az", "m/s^2", 4);
  DEBUG(iAz)
  //data[iAz].calcTrendline = 1;
#ifdef ENABLE_SENSE_GYRO
  iGx = addDataStream(data, &nSamples, "Gyro in x", "Gx", "rad/s", 4);
  DEBUG(iGx)
  iGy = addDataStream(data, &nSamples, "Gyro in y", "Gy", "rad/s", 4);
  DEBUG(iGy)
  iGz = addDataStream(data, &nSamples, "Gyro in z", "Gz", "rad/s", 4);
  DEBUG(iGz)
#endif
#endif

#ifdef ENABLE_SENSE_HUMID
  // humidity and temperature
  sht30.begin();
  iTemp = addDataStream(data, &nSamples, "Temperature in C from humid sensor", "TC", "C", 0);
  DEBUG(iTemp)
  iHumid = addDataStream(data, &nSamples, "Humidity", "RH", "percent", 0);
  DEBUG(iHumid)
#endif

#ifdef ENABLE_SENSE_ALTIM
  bmp280.begin(); // altitude, temp, pressure
  // NOTE altimeter varies slowly; so do not collect statistics
  iAlt = addDataStream(data, &nSamples, "Altitude barometric", "AOG", "m", 0);
  //data[iAlt].calcTrendline = 1;
  data[iAlt].baselineType = 2; // calculate baseline and subtract from data
  DEBUG(iAlt)
#endif

#ifdef ENABLE_SENSE_MAG
  lis3mdl.begin_I2C(); // magnetometer
  iMx = addDataStream(data, &nSamples, "Magnetic Field in x", "Mx", "uT", 4);
  DEBUG(iGx)
  iMy = addDataStream(data, &nSamples, "Magnetic Field in y", "My", "uT", 4);
  DEBUG(iGy)
  iMz = addDataStream(data, &nSamples, "Magnetic Field in z", "Mz", "uT", 4);
  DEBUG(iGz)
#endif

  timeReference = millis(); // initialize the reference time for trendline calculations

  MESSAGE("Number of data streams ", nSamples)

  //
  // ********************* Need to add function for printing data stream summary to log file
  //

  // print headers to file for spreadsheet datalogging
  int status = printSampleStatSpreadsheetToFile(dataFileName, data, nSamples, ",", countSDLine, 1); // use commas to separate columns in table (8 char width)

  // *************** EVENT_4: INITIALIZE EVENTS ********************************************
  // * INITIALIZE and configure each event
  // * also make sure to initialize any devices or libraries if needed
  // * event types:
  // *  0 = button or switch (boolean: 0 = passive and 1 = active) associated with a pin
  // *  1 = threshold indicator (boolean: 0 = inside of threshold limit and 1 = outside of threshold limit)
  // *  2 = state (integer correponding to preset states) using some other type of user-coded logic
  // * ***********************************************************************************
#ifdef SENSE_BUTTON
  //char eventStatesTemp[EVENT_STATES_MAX] [EVENT_NAME_SHORT] = {"PRESS", "RELEASE"};
  //jUserButton = addEvent(events, &nEvents, "Sense User Button", "ButS", 2, eventStatesTemp, 0, 1);
  jUserButton = addEvent(events, &nEvents, "Sense User Button", "ButS", 0, 1, 2, "PRESS", "RELEASE");
  // note the User Button on the Adafruit Sense is 0 when pressed
  linkEventToPin(events, jUserButton, SENSE_BUTTON);
  DEBUG(jUserButton)
#endif

  if (iAx != -1) // create thresholds indicating that the device is pitched nose up or nose down
  {
    jPitch = addEvent(events, &nEvents, "Pitch Angle States", "Pitch", 1, 1, 3, "NOSEDWN", "NOSELVL", "NOSEUP");
    setEventBreakpoints(events, jPitch, iAx, -8.0, 8.0);
    jRoll = addEvent(events, &nEvents, "Roll Angle States", "Roll", 1, 1, 3, "LEFTUP", "ROLLLVL", "RIGHTUP");
    setEventBreakpoints(events, jRoll, iAy, -8.0, 8.0);
  }

  if (iTime != -1) // create thresholds for time periods
  {
    jTimer = addEvent(events, &nEvents, "Timer for session", "Timer", 1, 0, 4, "INIT", "BASELN", "COLLECT", "SHUTDOWN");
    setEventBreakpoints(events, jTimer, iTime, 5., 35., 3600.);
  }

  status = reportEventToFile(eventFileName, events, nEvents, 0, ",", countEvents, 1); // print event header

  //
  // ********************* Need to add function for printing event tracker summary to log file
  //

  // initialize LED signals
  nextLEDPhaseChange = millis() + LEDPhaseInterval;
  LEDPhaseState = 1;
  pixelSet(LEDPhaseUp, LEDLevel); // SET to red LED for startup Stage

  // ---------------------------------------------------------------------
  // - set up timing variables
  // ---------------------------------------------------------------------
  endTime = millis(); // time at end of setup function in millis
  lastEndTime = endTime;
  nextSerialOutput = endTime + serialInterval;
  nextSampleOutput = endTime + samplingInterval;
  nextSlowDataUpdate = endTime + slowDataInterval;
  startTime = millis(); // time at start of loop function in millis
  DEBUG(startTimeMicros)
  startTimeMicros = micros(); // time at start of loop function in millis
  DEBUG(startTimeMicros)
}
// ** end of SETUP function
// **************************************************************************

// **************************************************************************
// * LOOP: start of loop function
// **************************************************************************
void loop()
{
  unsigned long loopStartTime = millis();

  // *************** DATA_5: UPDATE DATA STREAM ********************************************
  // * UPDATE each data stream with sensor values or calculated values
  // * ***********************************************************************************
  // ---------------------------------------------------------------------
  // - UPDATE data streams
  // -      FAST data streams update on every pass through loop()
  // -      SLOW data streams update when the appropriate time increment has passes
  // -  ** NOTE: the updating actions should be moved to a function to simplify this section
  // ---------------------------------------------------------------------

  // FAST DATA update values of all data that is collected as fast as possible
  // current time
  float currentTime = ((float)millis()) / 1000.;
  timeRelative = millis() - timeReference;            // find relative time for trendline slope
  float relativeTime = ((float)timeRelative) / 1000.; // time used for trendline slope

  int status = updateDataSample(data, iTime, currentTime); // current CPU time in seconds

  // simulated data
  float currentSimX = simulatedSensor(1., 5., 1.);                   // simulated sensor with mean 10.0, range 2.0 and slope = 1
  status = updateDataSample(data, iSimX, currentSimX, relativeTime); // current simulated data

  // simulated data
  float currentSimY = simulatedSensorSine(0., 0., 1., 10., 10.);     // simulated sensor
  status = updateDataSample(data, iSimY, currentSimY, relativeTime); // current simulated data

#ifdef ENABLE_SENSE_ACCEL
  // Accelerometer data
  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t temp;
  lsm6ds33.getEvent(&accel, &gyro, &temp);
  // accel_x = accel.acceleration.x;
  // accel_y = accel.acceleration.y;
  // accel_z = accel.acceleration.z;
  //gyro_x = gyro.gyro.x;
  //gyro_y = gyro.gyro.y;
  //gyro_z = gyro.gyro.z;
  status = updateDataSample(data, iAx, accel.acceleration.x, relativeTime);
  status = updateDataSample(data, iAy, accel.acceleration.y, relativeTime);
  status = updateDataSample(data, iAz, accel.acceleration.z, relativeTime);
#ifdef ENABLE_SENSE_GYRO
  status = updateDataSample(data, iGx, gyro.gyro.x, relativeTime);
  status = updateDataSample(data, iGy, gyro.gyro.y, relativeTime);
  status = updateDataSample(data, iGz, gyro.gyro.z, relativeTime);
#endif
#endif

  // #ifdef ENABLE_SENSE_ALTIM
  //   float altitude = bmp280.readAltitude(1013.25);
  //   status = updateDataSample(data, iAlt, altitude, relativeTime);
  // #endif

#ifdef ENABLE_SENSE_MAG
  lis3mdl.read();
  status = updateDataSample(data, iMx, lis3mdl.x, relativeTime);
  status = updateDataSample(data, iMy, lis3mdl.y, relativeTime);
  status = updateDataSample(data, iMz, lis3mdl.z, relativeTime);
#endif

#ifdef ENABLE_RANDOM_DELAY
  // add short random delay here to avoid serendipitous synchronization of sensor variations
  delayMicroseconds(random(10, 2000)); // random delay of 0.01 to 2 ms
#endif
  //delay(5);
  unsigned long currentTimeMicros = micros();
  float currentLoopTime = ((float)(currentTimeMicros - startTimeMicros)) / 1000.; // convert to ms
  if (currentTimeMicros >= startTimeMicros)
  {
    // do nothing
  }
  else
  {
    // unsigned long int has rolled over
    unsigned long deltaTime = (unsigned long) - 1;
    WARN("unsigned long Micros() has rolled over! ", currentTimeMicros)
    deltaTime = deltaTime - startTimeMicros + currentTimeMicros;
    currentLoopTime = ((float)deltaTime) / 1000.; // convert to ms
  }
  status = updateDataSample(data, iLoopTime, currentLoopTime); // current loop time in ms

  // SLOW DATA update values of data if sufficient time has passed to probe the sensor again
  if (millis() > nextSlowDataUpdate)
  {
    // time to update the slow data sources
    // humidity and temperature from sht30
    //humidity = sht30.readHumidity();
    //temperatureSHT = sht30.readTemperature();
#ifdef ENABLE_SENSE_HUMID
    status = updateDataSample(data, iTemp, sht30.readTemperature()); // current loop time in ms
    status = updateDataSample(data, iHumid, sht30.readHumidity());   // current loop time in ms
#endif

#ifdef ENABLE_SENSE_ALTIM
    float altitude = bmp280.readAltitude(1013.25);
    status = updateDataSample(data, iAlt, altitude, relativeTime);
#endif

    nextSlowDataUpdate = millis() + slowDataInterval;
  }

  // *************** EVENT_5: UPDATE EVENTS ********************************************
  // * UPDATE each event
  // * ***********************************************************************************
  // ---------------------------------------------------------------------
  // - CHECK for state events (button press, switch change, thresholds)
  // -   to control change of
  // -       - STATE of the program (e.g. initialization, baseline, calibration, collection, shutdown, sleep, etc.)
  // -       - STATUS (warnings and error messages)
  // -       - MODE (e.g. standby, climb, descent, idle, etc.)
  // ---------------------------------------------------------------------
  // FAST UPDATES to events (checked on every pass through loop)
  // move to function: checkDigitalPins();
  for (int j = 0; j < nEvents; j++)
  {
    int stype = events[j].eventType;
    if (stype == 0)
    {
      // this event is a digital button/switch
      int pinState = digitalRead(events[j].pin);
      if (pinState != events[j].state)
      {
        // pin State has changed!
        updateEventState(events, j, pinState, loopStartTime);

        reportEventToSerial(events, nEvents, j);

        countEvents++;
#ifdef USE_SD
        reportEventToFile(eventFileName, events, nEvents, j, ",", countEvents, 0); // print event as CSV file
#endif
      }
      else
      {
        // pin State has not changed
        events[j].priorState = pinState;
        events[j].justUpdated = 0; // indicates a repeated state
      }
    }
    else if (stype == 1)
    {
      // this event is a threshold indicator
      // do nothing here - only evaluate thresholds after sample is complete (based on averages)
    }
    else
    {
      // this event is a state indicator
    }
  }

  // SLOW UPDATES to events (checked only when sampling time or other indicator is complete)
  if (millis() > nextSampleOutput)
  {

    //
    // calculate sample statistics from current data for specific variables as needed
    //
    updateSampleStats(data, nSamples); // already has been updated in prior block

    // loop through event and check their status
    for (int j = 0; j < nEvents; j++)
    {
      int stype = events[j].eventType;
      if (stype == 0)
      {
        // this event is a digital button/switch - do nothing here - digital pins are checked in the fast section
      }
      else if (stype == 1)
      {
        // this event is a threshold indicator: get the corresponding data(sensor) value
        int iThreshold = events[j].thresholdDataIndex;
        float dataValue = data[iThreshold].currentVal;
        if (data[iThreshold].n > 1)
        {
          dataValue = data[iThreshold].sumX / ((float)data[iThreshold].n); // use average if available
        }

        // check breakpoint thresholds
        int currentState = 0; // determine the state = less than one of the breakpoints
        // if (dataValue > events[j].breakpointValue[events[j].numStates])
        // {
        //   currentState = events[j].numStates; // currently data is in the highest state
        // }
        // else
        // {
        for (int k = 0; k < events[j].numStates - 1; k++)
        {
          if (dataValue < events[j].breakpointValue[k])
          {
            // currentState = k;
            // do nothing, current state is correct
          }
          else
          {
            currentState = k + 1; // update current state
          }
        }

        if (currentState != events[j].state)
        {
          // threshold State has changed!
          updateEventState(events, j, currentState, loopStartTime);

          reportEventToSerial(events, nEvents, j);

          countEvents++;
          reportEventToFile(eventFileName, events, nEvents, j, ",", countEvents, 0); // print event as CSV file
        }
        else
        {
          // pin State has not changed
          events[j].priorState = currentState;
          events[j].justUpdated = 0; // indicates a repeated value
        }
      }
      else
      {
        // this event is a state indicator
      }
    } // DO NOT RESET nextSampleOutput time here! It is updated when the sample data is
    //    communicated to Serial and/or File in later section

    if (events[jTimer].justUpdated == 1)
    {
      if (events[jTimer].state == 1)
      {
        LEDPhaseUp = 2; // yellow
      }
      else if (events[jTimer].state == 2)
      {
        LEDPhaseUp = 3; // green
      }
      else if (events[jTimer].state == 3)
      {
        LEDPhaseUp = 1; // red
      }
    }

    // handle baseline collection
    if (events[jTimer].state == 1)
    {
      // add current sample averages into baseline
      MESSAGE("Adding samples into baseline", events[jTimer].state)
      MESSAGE("Adding samples into baseline", events[jTimer].eventStateName[events[jTimer].state])

      for (int i = 0; i < nSamples; i++)
      {
        if (data[i].n > 0) // only add info into baseline if sample size is 1 or more
        {
          data[i].baselineCount++;
          data[i].baselineSum += data[i].average;
          data[i].baselineSumX2 += data[i].average * data[i].average;
        }
      }
    }
    else if ((events[jTimer].state == 2) && (events[jTimer].justUpdated == 1))
    {
      // calculate baseline and print to serial and log file
      MESSAGE("Done calculating Baselines", events[jTimer].state)
      for (int i = 0; i < nSamples; i++)
      {
        float average = 0.;
        float variance = 0.;
        float standardDeviation = 0.;
        float sampleSize = (float)data[i].baselineCount;

        if (data[i].baselineCount == 0)
        {
          WARN("BASELINE SAMPLE EMPTY!", data[i].baselineCount)
        }
        else
        {
          average = data[i].baselineSum / sampleSize;
          variance = (data[i].baselineSumX2 - data[i].baselineSum * data[i].baselineSum / sampleSize) / (sampleSize - 1.);
          if (variance < 0.)
          {
            WARN("negative variance in baseline var = ", variance)
            WARN("negative variance in baseline data = ", i)
            standardDeviation = 0.;
          }
          else
          {
            standardDeviation = sqrt(variance);
          }

          if (data[i].baselineType == 2)
          {
            MESSAGE("updating baseline for variable", i)
            MESSAGE("updated baseline = ", average)
            data[i].baseline = average;
          }
          // print out baseline stats
          Serial.print("BASELINE evaluated for variable: ");
          Serial.print(data[i].dataNickName);
          Serial.print(" avg = ");
          Serial.print(average);
          Serial.print(" stdev = ");
          Serial.print(standardDeviation);
          Serial.print(" size n = ");
          Serial.print(data[i].baselineCount);
          Serial.print(" baseline = ");
          Serial.println(data[i].baseline);

#ifdef USE_SD
          File tmpFile;
          tmpFile = SD.open(logFileName, FILE_WRITE);
          if (tmpFile)
          {
            // print out baseline stats to log file
            tmpFile.print("BASELINE evaluated for variable: ");
            tmpFile.print(data[i].dataNickName);
            tmpFile.print(" avg = ");
            tmpFile.print(average);
            tmpFile.print(" stdev = ");
            tmpFile.print(standardDeviation);
            tmpFile.print(" size n = ");
            tmpFile.print(data[i].baselineCount);
            tmpFile.print(" baseline = ");
            tmpFile.println(data[i].baseline);
            tmpFile.close();
          }
#endif
        }
      }
    }
  }

  // ---------------------------------------------------------------------
  // - ACTIONS (some actions may have been taken in the UPDATE section)
  // -
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // -  COMMUNICATION (if sufficient time has passed, prepare data for output and write it out)
  // ---------------------------------------------------------------------
  if (millis() > nextSampleOutput)
  {
    //
    // calculate sample statistics from current data for all data streams
    //
    // updateSampleStats(data, nSamples); // already has been updated in prior block

    if (millis() > nextSerialOutput) // if sufficient time has passed also write data to serial (and log file)
    {
      unsigned long startOutputTime = millis();
      //MESSAGE("time to write out data", nextSampleOutput)
      // create function call to automatically print out data in suitable formats
      //   - to Serial monitor (for debugging) which should be mirrored to a log file when using SD card
      //   - to CSV or JSON file on SC
      //   - messages transmitted via LoRa radio, BlueTooth, Meshtastic, etc.
      int status = printSampleStatTableToSerial(data, nSamples, "\t"); // use tabs to separate columns in table (8 char width)
#ifdef USE_SD
      status = printSampleStatTableToFile(logFileName, data, nSamples, "\t"); // use commas to separate columns in table (8 char width)
#endif
      //countSDLine++;                                                                                // increment counter on number of lines printed to SD data file
      //status = printSampleStatSpreadsheetToFile(dataFileName, data, nSamples, ",", countSDLine, 0); // use commas to separate columns in table (8 char width)

      // RESET samples!
      //resetSampleStats(data, nSamples);

      //   nextSampleOutput += samplingInterval;
      nextSerialOutput = millis() + serialInterval;
      //    DEBUG(nextSampleOutput)

      unsigned long endOutputTime = millis();
      Serial.print("Time Spent on Serial Output Communication = ");
      Serial.print(endOutputTime - startOutputTime);
      Serial.println(" ms");
    }

    unsigned long startOutputTime = millis();
    //MESSAGE("time to write out data", nextSampleOutput)
    // create function call to automatically print out data in suitable formats
    //   - to Serial monitor (for debugging) which should be mirrored to a log file when using SD card
    //   - to CSV or JSON file on SC
    //   - messages transmitted via LoRa radio, BlueTooth, Meshtastic, etc.
    //int status = printSampleStatTableToSerial(data, nSamples, "\t");                              // use tabs to separate columns in table (8 char width)
    //status = printSampleStatTableToFile(logFileName, data, nSamples, "\t");                       // use commas to separate columns in table (8 char width)
#ifdef USE_SD
    countSDLine++;                                                                                     // increment counter on number of lines printed to SD data file
    int status = printSampleStatSpreadsheetToFile(dataFileName, data, nSamples, ", ", countSDLine, 0); // use commas to separate columns in table (8 char width)
    if (status == 1)
    {
      // update neopixel LED
      LEDSDActive = 1;
      nextSDPhaseChange = millis() + LEDSDInterval;
      pixelSet(LEDSDColor, LEDLevel);

      // update status LED
      pinMode(SENSE_BLUE, OUTPUT);
      digitalWrite(SENSE_BLUE, HIGH);
    }
    else
    {
      LEDSDActive = 1;
      // update neopixel LED
      nextSDPhaseChange = millis() + LEDSDInterval;
      pixelSet(1, LEDLevel); // turn to red
    }
#endif
    // RESET samples!
    resetSampleStats(data, nSamples);
    timeReference = millis(); // initialize the reference time for trendline calculations

    //   nextSampleOutput += samplingInterval;
    nextSampleOutput = millis() + samplingInterval;
    //    DEBUG(nextSampleOutput)

    unsigned long endOutputTime = millis();
    Serial.print("Time Spent on DataFile Output Communication = ");
    Serial.print(endOutputTime - startOutputTime);
    Serial.println(" ms");
  }

  // ouput chunks of raw data if needed

  // update LED
  if (LEDSDActive && (millis() > nextSDPhaseChange))
  {
    //MESSAGE("Turn off blue SD LED", LEDSDActive)
    LEDSDActive = 0;
    digitalWrite(SENSE_BLUE, LOW);
    if (LEDPhaseState == 0)
    {
      pixelSet(LEDPhaseDown, LEDLevel);
    }
    else
    {
      pixelSet(LEDPhaseUp, LEDLevel);
    }
  }

#ifdef ENABLE_NEOPIXEL
  if (millis() > nextLEDPhaseChange)
  {
    nextLEDPhaseChange = millis() + LEDPhaseInterval;
    if (LEDPhaseState == 0)
    {
      LEDPhaseState = 1;
      pixelSet(LEDPhaseUp, LEDLevel);
    }
    else
    {
      LEDPhaseState = 0;
      pixelSet(LEDPhaseDown, LEDLevel);
    }
  }
#endif

  // ---------------------------------------------------------------------
  // - update some of the timing variables
  endTime = millis(); // time at end of loop function in millis
  lastEndTime = endTime;
  unsigned long int loopTime = endTime - startTime;
  // DEBUG(loopTime)

  startTime = millis();       // time at start of loop function in millis
  startTimeMicros = micros(); // time at start of loop function in millis
}
// ************************************************************************
// * end of LOOP
// ************************************************************************


#ifdef ENABLE_NEOPIXEL

void pixelSet(int pixMode, int pixLevel)
{
  pixels.clear(); // Set all pixel colors to 'off'
  switch (pixMode)
  { // Start the new animation...
    case 0:
      pixels.setPixelColor(0, pixels.Color(pixLevel, pixLevel, pixLevel)); // Blue-Red
      break;
    case 1:
      pixels.setPixelColor(0, pixels.Color(pixLevel, 0, 0)); // Red
      break;
    case 2:
      pixels.setPixelColor(0, pixels.Color(pixLevel, pixLevel, 0)); // Red-Green
      break;
    case 3:
      pixels.setPixelColor(0, pixels.Color(0, pixLevel, 0)); // Green
      break;
    case 4:
      pixels.setPixelColor(0, pixels.Color(0, pixLevel, pixLevel)); // Green-Blue
      break;
    case 5:
      pixels.setPixelColor(0, pixels.Color(0, 0, pixLevel)); // Blue
      break;
    case 6:
      pixels.setPixelColor(0, pixels.Color(pixLevel, 0, pixLevel)); // Blue-Red
      break;
    default:
      pixels.setPixelColor(0, pixels.Color(0, 0, 0)); // Black/off
      break;
  }

  pixels.show(); // Send the updated pixel colors to the hardware.
}
#endif
