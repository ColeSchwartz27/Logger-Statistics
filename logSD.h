
// ---SD CARD SETUP----------------------------------------------------------
//  SD communication Based on on-line SD library example:
//    http://arduino.cc/en/Tutorial/Datalogger

// for datalogging to SD wtih real-time-clock
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>

// should move to deviceConfig
// SD chip select is pin 10 on Adalogger featherwing!
//#define SD_CS 10
// SD chip select for E-Ink featherwing
//#define SD_CS 5

//*********** SD Card Setup
// char logFileType[] = "log";
// char dataFileType[] = "data";
// char eventFileType[] = "evnt";
// char fileSuffix[] = ".txt";
char dirPath[30];      // create buffer to hold path to directory
char logFileName[40]; // create buffer to hold filename for datalogger
char dataFileName[40]; // create buffer to hold filename for datalogger
char eventFileName[40]; // create buffer to hold filename for datalogger

// File logFile;
// File dataFile;
// File eventFile;

#ifdef USE_RTC
RTC_PCF8523 rtc; // real time clock on Adafruit adalogger
//RTC_DS1307 rtc;
DateTime currentNow;
#endif

int setup_SD_file(char *fileCode, char *filePre, char *fileSuf, char *fullFileName)
{
    //String filePre = "DD3v";
    //String fileSuf = ".txt";
    int fileNum = 0;
    Serial.print("InitSD-");
    // make sure that the default chip select pin is set to
    // output, even if you don't use it:
    pinMode(SD_CS, OUTPUT);
    //pinMode(10, OUTPUT);
    // see if the card is present and can be initialized:
    if (!SD.begin(SD_CS))
    {
        Serial.println("SD fail");
        // don't do anything more:
        return -1;
    }
    Serial.println("done");

    // check to see if data file already exists - increment file number until you find
    //   a number that does not exist; using numbers from 00 to 99
    int nextLoop = 1;
    for (fileNum = 0; fileNum < 100 && nextLoop; fileNum++)
    {
        String tempFileName = "";
        if (dirPath[0])
            tempFileName += String(dirPath);
        tempFileName += String(fileCode);
        tempFileName += String(filePre);

        if (fileNum < 10)
            tempFileName += "0";
        tempFileName += String(fileNum);
        tempFileName += String(fileSuf);
        tempFileName.toCharArray(fullFileName, 40);
        Serial.print("filename [");
        Serial.print(fullFileName);
        Serial.println("]");

        if (SD.exists(fullFileName))
        {
            // filename already exists - try again
            nextLoop = 1;
        }
        else
        {
            // fileName does not exist - use this one!
            nextLoop = 0;
            Serial.print("SD OK <<");
            Serial.print(fullFileName);
            Serial.println(">>");
        }
    }
    // int SDstatus = setup_SD(dataFileName, fileSuffix, dataFileBase);
    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    File tmpFile;
    tmpFile = SD.open(fullFileName, FILE_WRITE);
    // if the file opened okay, write to it:
    if (tmpFile)
    {
        tmpFile.print("Kite Datlogger File: ");
        tmpFile.println((char *)fullFileName); // first line
        tmpFile.print("Kite Datlogger Code: ");
        tmpFile.println(__FILE__); // first line
        tmpFile.print("Kite Datlogger Date compiled: ");
        tmpFile.println(__DATE__); // first line
        tmpFile.print("Kite Datlogger time compiled: ");
        tmpFile.println(__TIME__); // first line
        //Serial.print("RTC Current Date (MM/DD/YYYY): ");
        //Serial.print(" and time (HH:MM:SS): ");
        tmpFile.print("Kite Datlogger Date file created: ");

#ifdef USE_RTC
        currentNow = rtc.now();
        tmpFile.print(currentNow.month());
        tmpFile.print("/");
        tmpFile.print(currentNow.day());
        tmpFile.print("/");
        tmpFile.println(currentNow.year());
        tmpFile.print("Kite Datlogger time file created: ");
        tmpFile.print(currentNow.hour());
        tmpFile.print(":");
        tmpFile.print(currentNow.minute());
        tmpFile.print(":");
        tmpFile.println(currentNow.second());
#else
        tmpFile.println("no RTC available");
#endif
        tmpFile.println("-------------------------------------------------------------");
        tmpFile.println(); // blank line
        //    dataFile.print("{\""); dataFile.print((char*)dataFileName); dataFile.println("\":["); // first line is start of Json structure
        tmpFile.close(); // close the file:
    }
    else
    {
        // if the file didn't open, print an error:
        Serial.println("error opening data file");
    }

    return 1;
} // END OF SD FILE SETUP ----------------------------------------------------

void initializeSDFileDirectory()
{
    // check to see if we have an RTC
#ifdef USE_RTC
    if (!rtc.begin())
    {
        Serial.println("Couldn't find RTC");
        while (1)
            ;
    }
    currentNow = rtc.now();
    Serial.print("RTC Current Date (YYMMDD)");
    Serial.print((currentNow.year()-2000) * 10000 + currentNow.month() * 100 + currentNow.day());
    Serial.print(" and time (HHMMSS) =");
    Serial.println(currentNow.hour() * 10000 + currentNow.minute() * 100 + currentNow.second());
    int dirDate((currentNow.year() - 2000) * 10000 + currentNow.month() * 100 + currentNow.day());
//int dirTime(currentNow.hour() * 10000 + currentNow.minute() * 100 + currentNow.second());
    char charDirDate[12];
    itoa(dirDate, charDirDate, 10);
#else
    char charDirDate[12];
    strcpy(charDirDate, dataFolder);
    //itoa(dirDate, charDirDate, 10);
#endif
    strcpy(dirPath, "/d");
    //char charDirTime[10];
    //itoa(dirTime, charDirTime, 10);
    strcat(dirPath, charDirDate);
    //strcat(dirPath, "/");
    //strcat(dirPath, charDirTime);
    Serial.print("Directory path = ");
    Serial.println(dirPath);
    Serial.print("Initializing SD card...");
    if (!SD.begin(SD_CS))
    {
        Serial.println("initialization failed!");
        while (1)
            ; // prevent code from processing any further if SD not functioning
    }
    Serial.println("initialization done.");
    if (SD.mkdir(dirPath))
    {
        Serial.println("new directory made successfully");
        strcat(dirPath, "/");
    }
    else
    {
        Serial.println("new directory failed");
        dirPath[0] = 0;
    }
    return;
}
