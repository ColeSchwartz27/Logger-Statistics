// code to test creation of a structure for storing sensor data
// *******************************
// move to sampleStats.h after building and testing
// *******************************
// structure used for storing data as it is collected and calculating statistics of the data
// before output. in this structure, a SAMPLE of data is collected over a time interval,
// with n (SAMPLE SIZE) data points collected at time increments (that are not necessarily equal)

#ifdef ENABLE_DATA_CHUNKS
// MAX_RAW_DATA used for storing array of raw data during collection
//  set MAX_RAW_DATA to 1 to only store most recent value
#define MAX_RAW_DATA 1000
#endif

// configure setting for labels (char arrays) for data streams
#define DATA_NAME_MAX 50
#define DATA_NAME_SHORT 10

struct sampleStats
{
  /* data */
  int n;            // count of number of data values in this sample
  float currentVal; // most recent value entered into sample

#ifdef ENABLE_DATA_CHUNKS
  float rawData[MAX_RAW_DATA]; // optional use during development: array to store raw data
#endif

  // rawData[n-1] is the most recently collected data point
  float sumX;                         // sum of all values entered into sample
  float sumX2;                        // sum of all values squared (before adding to sum)
  float sumXT;                        // sum of value multiplied by time for trendline
  float sumT;                         // sum of time values for trendline
  float sumT2;                        // sum of time squared for trendline
  char dataName[DATA_NAME_MAX];       // name data stream name
  char dataNickName[DATA_NAME_SHORT]; // short name of data stream
  char dataUnits[DATA_NAME_MAX];      // string with units of measurement for data stream

  float average;
  float standardDeviation;

  int baselineType; // enable calculating a baseline to subtract: 0 = none, 1 = input, 2 = calculate
  float baseline;
  float baselineSum;
  float baselineSumX2;
  int baselineCount;

  int calcTrendline; // flag indicating calculation of a trendline

  int eventIndex; // associated event to track thresholds (if used)

  int outputStats; // variable indicating what stats to output to spreadsheets
  // -1 = no output (just a variable for internal calculations)
  // 0  = only output current value (no statistics)
  // 1  = only output average
  // 2  = output average and current
  // 3  = output average and sample size
  // 4  = output average and standard deviation
  // 5  = output all info (including current and sample size)

  // lots more information to be added later
  // * max value of data in sample
  // * min value of data in sample
  // * average of data in sample
  // ....
};

#define MAX_SAMPLES 20
int nSamples = 0;
sampleStats data[MAX_SAMPLES];
// data[2].currentVal; // contains current sensor value

// functions that will manipulate information in sampleStats structure
// will be moved to sampleStats.cpp later
// may need to declare extern variables

// addDataStream: creates a new data stream and returns the index of that dataStream in the sampleStat
//        structure array
int addDataStream(sampleStats *localData, int *numSamples, char *dataName, char *dataNickName, char *dataUnits, int outputType)
//int addDataStream(char *dataName, char *dataNickName, char *dataUnits)
{
  int newSampleIndex = *numSamples;
  if (newSampleIndex == MAX_SAMPLES)
  {
    WARN("too many data streams", newSampleIndex)
    return -1; // return error code
  }
  //nSamples++;
  *numSamples = *numSamples + 1;

  // initialize key values in data structure
  localData[newSampleIndex].n = 0;      // initialize the time sample
  localData[newSampleIndex].sumX = 0.;  // initialize the sum of data
  localData[newSampleIndex].sumX2 = 0.; // initialie the sum of the squared data
  localData[newSampleIndex].sumT = 0.;
  localData[newSampleIndex].sumXT = 0.;               // initialie the sum of the data multiplied by time for trendline
  localData[newSampleIndex].sumT2 = 0.;               // initialie the sum of the squared time
  localData[newSampleIndex].outputStats = outputType; // variable indicating what stats to output to spreadsheets
                                                      // -1 = no output (just a variable for internal calculations)
                                                      // 0  = only output current value (no statistics)
                                                      // 1  = only output average
                                                      // 2  = output average and current
                                                      // 3  = output average and sample size
                                                      // 4  = output average and standard deviation
                                                      // 5  = output all info (including current and sample size)

  localData[newSampleIndex].average = 0.;
  localData[newSampleIndex].standardDeviation = 0.;

  localData[newSampleIndex].baselineType = 0; // default to not subtracting off baseline
  localData[newSampleIndex].baseline = 0.;
  localData[newSampleIndex].baselineSum = 0.; // default to baseline = 0
  localData[newSampleIndex].baselineCount = 0;
  localData[newSampleIndex].baselineSumX2 = 0.;

  localData[newSampleIndex].calcTrendline = 0; // default to not calculating trendline
  localData[newSampleIndex].eventIndex = -1;   // set to -1 as default (no event tracker)

  // perform checks on string lengths
  int nameLength = strlen(dataName);
  if (nameLength > (DATA_NAME_MAX - 2))
  {
    MESSAGE("name", dataName)
    WARN("Data name too long", nameLength)
  }
  else
  {
    strcpy(localData[newSampleIndex].dataName, dataName); // need to add check on length!
  }

  nameLength = strlen(dataNickName);
  if (nameLength > (DATA_NAME_MAX - 2))
  {
    MESSAGE("name", dataNickName)
    WARN("Data nickname too long", nameLength)
  }
  else
  {
    strcpy(localData[newSampleIndex].dataNickName, dataNickName); // need to add check on length!
  }

  nameLength = strlen(dataUnits);
  if (nameLength > (DATA_NAME_MAX - 2))
  {
    MESSAGE("name", dataUnits)
    WARN("data units name too long", nameLength)
  }
  else
  {
    strcpy(localData[newSampleIndex].dataUnits, dataUnits); // need to add check on length!
  }

  return newSampleIndex;
}

int updateDataSample(sampleStats *dataStream, int index, float inputValue, float relTime = 0.)
{
  // function for adding new data points to a sample
  // subtract baseline
  float value = inputValue - dataStream[index].baseline;

  // this function assumes the new data is a floating point number
  dataStream[index].currentVal = value;
  dataStream[index].sumX += value;
  dataStream[index].sumX2 += value * value;
#ifdef ENABLE_DATA_CHUNKS
  if ((dataStream[index].n + 1) < MAX_RAW_DATA)
  {
    dataStream[index].rawData[dataStream[index].n] = value;
  }
  else
  {
    WARN("Max sample size exceeded ", dataStream[index].n)
    return -1;
  }
#endif
  if (dataStream[index].calcTrendline == 1)
  {
    dataStream[index].sumT += relTime;
    dataStream[index].sumXT += value * relTime;
    dataStream[index].sumT2 += relTime * relTime;
  }

  dataStream[index].n++; // increment the sample size for time

  return 1;
}

int printSampleStatTableToSerial(sampleStats *dataStream, int nSamp, char *separator)
{
  // print header line
  Serial.println();
  Serial.print("---- Sample Data Summary for Device = ");
  Serial.print(deviceName);
  Serial.println(" ------------------------");
  //char separator[]="\t";
  // for tab separated formatted table, use row headings that are 9-15 characters long
  Serial.print("DataNames");
  for (int i = 0; i < nSamp; i++)
  {
    Serial.print(separator);
    //Serial.print(", ");
    Serial.print(dataStream[i].dataNickName);
  }
  Serial.println();
  Serial.print("DataUnits");
  for (int i = 0; i < nSamp; i++)
  {
    Serial.print(separator);
    Serial.print(dataStream[i].dataUnits);
  }
  Serial.println();
  Serial.print("CurrentData");
  for (int i = 0; i < nSamp; i++)
  {
    Serial.print(separator);
    Serial.print(dataStream[i].currentVal);
  }
  Serial.println();
  Serial.print("AverageData");
  for (int i = 0; i < nSamp; i++)
  {
    Serial.print(separator);
    if (dataStream[i].n == 0)
    {
      Serial.print(dataStream[i].currentVal);
    }
    else
    {
      Serial.print(dataStream[i].sumX / ((float)dataStream[i].n));
    }
  }
  Serial.println();

  //Serial.print("123456789012345"); // limit row header to 8-15 char
  Serial.print("StandardDev");
  for (int i = 0; i < nSamp; i++)
  {
    Serial.print(separator);
    if (dataStream[i].n < 2 || dataStream[i].outputStats < 4)
    {
      Serial.print("N/A");
    }
    else
    {
      // use computational formula for standard deviation
      float variance = (dataStream[i].sumX2 - dataStream[i].sumX * dataStream[i].sumX / ((float)dataStream[i].n)) / ((float)dataStream[i].n - 1.);
      if (variance < 0.)
      {
        WARN("negative variance!", variance)
        WARN("negative variance variable ", i)
        Serial.print("N/A");
      }
      else
      {
        Serial.print(sqrt(variance));
      }
      //      Serial.print(sqrt(variance));
    }
  }
  Serial.println();

  Serial.print("SampleSize");
  for (int i = 0; i < nSamp; i++)
  {
    Serial.print(separator);
    Serial.print(dataStream[i].n);
  }
  Serial.println();
  return 1;
}

#ifdef USE_SD
int printSampleStatTableToFile(char *fullFileName, sampleStats *dataStream, int nSamp, char *separator)
{
  // open file to log information
  File tmpFile;
  tmpFile = SD.open(fullFileName, FILE_WRITE);
  // if the file opened okay, write to it:
  if (tmpFile)
  {

    // print header line
    tmpFile.println();
    tmpFile.print("---- Sample Data Summary for Device = ");
    tmpFile.print(deviceName);
    tmpFile.println(" ------------------------"); //char separator[]="\t";
    // for tab separated formatted table, use row headings that are 9-15 characters long
    tmpFile.print("DataNames");
    for (int i = 0; i < nSamp; i++)
    {
      tmpFile.print(separator);
      //Serial.print(", ");
      tmpFile.print(dataStream[i].dataNickName);
    }
    tmpFile.println();
    tmpFile.print("DataUnits");
    for (int i = 0; i < nSamp; i++)
    {
      tmpFile.print(separator);
      tmpFile.print(dataStream[i].dataUnits);
    }
    tmpFile.println();
    tmpFile.print("CurrentData");
    for (int i = 0; i < nSamp; i++)
    {
      tmpFile.print(separator);
      tmpFile.print(dataStream[i].currentVal);
    }
    tmpFile.println();
    tmpFile.print("AverageData");
    for (int i = 0; i < nSamp; i++)
    {
      tmpFile.print(separator);
      if (dataStream[i].n == 0)
      {
        tmpFile.print(dataStream[i].currentVal);
      }
      else
      {
        tmpFile.print(dataStream[i].sumX / ((float)dataStream[i].n));
      }
    }
    tmpFile.println();

    //Serial.print("123456789012345"); // limit row header to 8-15 char
    tmpFile.print("StandardDev");
    for (int i = 0; i < nSamp; i++)
    {
      tmpFile.print(separator);
      if (dataStream[i].n < 2 || dataStream[i].outputStats < 4)
      {
        tmpFile.print("N/A");
      }
      else
      {
        // use computational formula for standard deviation
        float variance = (dataStream[i].sumX2 - dataStream[i].sumX * dataStream[i].sumX / ((float)dataStream[i].n)) / ((float)dataStream[i].n - 1.);
        if (variance < 0.)
        {
          WARN("negative variance!", variance)
          WARN("negative variance variable ", i)
          tmpFile.print("N/A");
        }
        else
        {
          tmpFile.print(sqrt(variance));
        }
        //tmpFile.print(sqrt(variance));
      }
    }
    tmpFile.println();

    tmpFile.print("SampleSize");
    for (int i = 0; i < nSamp; i++)
    {
      tmpFile.print(separator);
      tmpFile.print(dataStream[i].n);
    }
    tmpFile.println();
    tmpFile.close();
  }
  else
  {
    // if the file didn't open, print an error:
    Serial.println("error opening data file");
    return 0;
  }

  return 1;
}
#endif

#ifdef USE_SD
int printSampleStatSpreadsheetToFile(char *fullFileName, sampleStats *dataStream, int nSamp, char *separator, int count, int headerFlag)
{
  // open file to log information
  File tmpFile;
  tmpFile = SD.open(fullFileName, FILE_WRITE);
  // if the file opened okay, write to it:
  if (tmpFile)
  {

    // for first column, print Device code
    tmpFile.print(deviceCode);

    // for second column, print count tracking number of lines
    tmpFile.print(separator);
    tmpFile.print(count);

    //char separator[]="\t";
    // for tab separated formatted table, use row headings that are 9-15 characters long
    //tmpFile.print("DataNames");
    for (int i = 0; i < nSamp; i++)
    {
      int outputStatValue = dataStream[i].outputStats; // variable indicating what stats to output to spreadsheets
      // -1 = no output (just a variable for internal calculations)
      // 0  = only output current value (no statistics)
      // 1  = only output average
      // 2  = output average and current
      // 3  = output average and sample size
      // 4  = output average and standard deviation
      // 5  = output all info (including current and sample size)

      if (outputStatValue != -1)
      {

        if (outputStatValue == 0 || outputStatValue == 2 || outputStatValue == 5)
        {
          // print CURRENT value
          tmpFile.print(separator);
          if (headerFlag == 1)
          {
            tmpFile.print(dataStream[i].dataNickName); // include short variable name
            tmpFile.print("_cv");                      // include tag indicating type of output
          }
          else
          {
            tmpFile.print(dataStream[i].currentVal);
          }
        }

        if (outputStatValue > 0)
        {
          // print AVERAGE value
          tmpFile.print(separator);
          if (headerFlag == 1)
          {
            tmpFile.print(dataStream[i].dataNickName); // include short variable name
            tmpFile.print("_av");                      // include tag indicating type of output
          }
          else
          {
            if (dataStream[i].n == 0)
            {
              tmpFile.print(dataStream[i].currentVal);
            }
            else
            {
              tmpFile.print(dataStream[i].sumX / ((float)dataStream[i].n));
            }
          }
        }

        if (outputStatValue > 3)
        {
          // print STANDARD DEVIATION value
          tmpFile.print(separator);
          if (headerFlag == 1)
          {
            tmpFile.print(dataStream[i].dataNickName); // include short variable name
            tmpFile.print("_sd");                      // include tag indicating type of output
          }
          else
          {
            if (dataStream[i].n < 2)
            {
              tmpFile.print("N//A");
            }
            else
            {
              // use computational formula for standard deviation
              float variance = (dataStream[i].sumX2 - dataStream[i].sumX * dataStream[i].sumX / ((float)dataStream[i].n)) / ((float)dataStream[i].n - 1.);
              if (variance < 0.)
              {
                WARN("negative variance!", variance)
                WARN("negative variance variable ", i)
                tmpFile.print("N//A");
              }
              else
              {
                tmpFile.print(sqrt(variance));
              }
            }
          }
        }

        if (outputStatValue == 3 || outputStatValue == 5)
        {
          // print SAMPLE SIZE
          tmpFile.print(separator);
          if (headerFlag == 1)
          {
            tmpFile.print(dataStream[i].dataNickName); // include short variable name
            tmpFile.print("_n");                       // include tag indicating type of output
          }
          else
          {
            tmpFile.print(dataStream[i].n);
          }
        }
      }

      if (dataStream[i].calcTrendline == 1)
      {
        // always print trendline slope and standard deviation if calculated ()

        float Sxx = dataStream[i].sumX2 - (dataStream[i].sumX * dataStream[i].sumX) / ((float)dataStream[i].n);
        float Sxt = dataStream[i].sumXT - (dataStream[i].sumX * dataStream[i].sumT) / ((float)dataStream[i].n);
        float Stt = dataStream[i].sumT2 - (dataStream[i].sumT * dataStream[i].sumT) / ((float)dataStream[i].n);
        float slope = Sxt / Stt; // slope from linear regression trendline

        float SSE = Sxx - Sxt * Sxt / Stt;
        //        float SSE = (dataStream[i].sumX2 - (dataStream[i].sumXT * dataStream[i].sumXT) / dataStream[i].sumT2);
        float sigma2 = SSE / ((float)dataStream[i].n - 2.); // should check to make sure n > 2!
        float stdErr = sqrt(sigma2 / Stt);

        tmpFile.print(separator);
        if (headerFlag == 1)
        {
          tmpFile.print(dataStream[i].dataNickName); // include short variable name
          tmpFile.print("_dt");                      // include tag indicating derivative with respect to time
          tmpFile.print(separator);
          tmpFile.print(dataStream[i].dataNickName); // include short variable name
          tmpFile.print("_re");                      // include tag indicating residual error
          tmpFile.print(separator);
          tmpFile.print(dataStream[i].dataNickName); // include short variable name
          tmpFile.print("_er");                      // include tag indicating standard error on slope
        }
        else
        {
          tmpFile.print(slope);
          tmpFile.print(separator);
          tmpFile.print(sqrt(sigma2));
          tmpFile.print(separator);
          tmpFile.print(stdErr);
        }
      }
    }

    tmpFile.println();
    tmpFile.close();
  }
  else
  {
    // if the file didn't open, print an error:
    Serial.println("error opening data file");
    return 0;
  }

  return 1;
}
#endif

int resetSampleStats(sampleStats *dataStream, int nSamp)
{

  for (int i = 0; i < nSamp; i++)
  {
    dataStream[i].n = 0;
    dataStream[i].sumX = 0.;
    dataStream[i].sumX2 = 0.;
    dataStream[i].sumT = 0.;
    dataStream[i].sumT2 = 0.;
    dataStream[i].sumXT = 0.;
  }

  return 1;
}

void updateSampleStats(sampleStats *dataStream, int nSamp)
{
  //  Serial.print("AverageData");
  for (int i = 0; i < nSamp; i++)
  {
    //Serial.print(separator);
    if (dataStream[i].n == 0)
    {
      dataStream[i].average = dataStream[i].currentVal;
    }
    else
    {
      dataStream[i].average = dataStream[i].sumX / ((float)dataStream[i].n);
    }
  }

  //Serial.print("123456789012345"); // limit row header to 8-15 char
  //Serial.print("StandardDev");
  for (int i = 0; i < nSamp; i++)
  {
    //Serial.print(separator);
    if (dataStream[i].n < 2 || dataStream[i].outputStats < 4)
    {
      dataStream[i].standardDeviation = 0.;
    }
    else
    {
      // use computational formula for standard deviation
      float variance = (dataStream[i].sumX2 - dataStream[i].sumX * dataStream[i].sumX / ((float)dataStream[i].n)) / ((float)dataStream[i].n - 1.);
      if (variance < 0.)
      {
        WARN("negative variance!", variance)
        WARN("negative variance variable ", i)
        //Serial.print("N/A");
      }
      else
      {
        dataStream[i].standardDeviation = sqrt(variance);
      }
      //      Serial.print(sqrt(variance));
    }
  }

  return;
}
