// *******************************
// simulatedSensor.h 
// *******************************
float simulatedSensor(float intercept, float slope, float range)
{
  // function that simulates a sensor that creates random sensor readings
  // that increase linearly with time, but vary randomly (according to uniform
  // distribution within range)

  float time = ((float)millis()) / 1000.;                      // time in seconds since program started
  float randomness = ((float)random(0, 2001) - 1000.) / 1000.; // create a random float between -1. and 1.

  float output = intercept + slope * time + randomness * range;
  //DEBUG(output)
  return (output);
}

float simulatedSensorSine(float intercept, float slope, float range, float amplitude, float period)
{
  // function that simulates a sensor that creates random sensor readings
  // that increase linearly with time, but vary randomly (according to uniform
  // distribution within range)
  // intercept and slope define the baseline signal as a linear function baseline(t) = intercept + slope * time(in s)
  // range defines the magnitude of random error: currently a uniform random error +- that adjusts the calculated value
  // amplitude an period define a sinusoidal signal that is added to the baseline

  float time = ((float)millis()) / 1000.;                      // time in seconds since program started
  float randomness = ((float)random(0, 2001) - 1000.) / 1000.; // create a random float between -1. and 1.
  float sine = amplitude * sin(2 * PI * time / period);        // sinusoidal function

  float output = intercept + slope * time + randomness * range + sine;
  //DEBUG(output)
  return (output);
}