// eventTracker.h
// this file defines a data structure for handling events that indicate status or state
// from components of the sensor system.
// "events" are defined as integer states (often 0 or 1) related to sensor states, buttons, etc:
// examples include:
//  - state of a button (pressed = 1, released = 0)
//  - sensor reading releative to a threshold (beyond threshold = 1, inside threshold = 0)
//  - mode of operation (e.g. collecting baseline, idle, waiting, climb, descent) used for control
//  - phase of incoming events

// configure setting for labels (char arrays) for data streams
#define EVENT_NAME_MAX 40
#define EVENT_NAME_SHORT 15
#define EVENT_STATES_MAX 20

struct eventTracker
{
    int state;       // current state of control event (0 = passive, 1 = active, other integers correspond to states)
    int priorState;  // state of event on prior check
    int justUpdated; // flag to indicate if state has just changed = 1

    int eventType;                        // code indicating what type of event
    char eventName[EVENT_NAME_MAX];       // name data stream name
    char eventNickName[EVENT_NAME_SHORT]; // short name of data stream
    // char passiveStateName[EVENT_NAME_SHORT];                 // label for when this event is passive
    // char activeStateName[EVENT_NAME_SHORT];                  // label for when this event is active
    char eventStateName[EVENT_STATES_MAX][EVENT_NAME_SHORT]; // labels for the different possible event states
    //char *eventStateName[EVENT_STATES_MAX]; // labels for the different possible event states
    int numStates; // number of different states possible for this event type

    int actionTaken; // flag to indicate that the event has caused an action (and accumulates number of actions)

    int stateCount[EVENT_STATES_MAX];                 // a counter of how many times this event state has been entered
    unsigned long stateTimeStarted[EVENT_STATES_MAX]; // the time at which this state was most recently initiated

    //    int numActivate;  // number of times the event has switched to active
    //    int numPassivate; // number of times the event has switched to passive
    unsigned long timeLastChange;
    //    unsigned long timeLastPassivate;
    //    unsigned long timeLastActivate;
    unsigned long stateDuration; // to store the length of time in the previous state

    unsigned long resetInterval; // time at which continuous indication from event will cause action to repeat

    int pin; // pin number associated with this event

    int thresholdType;      // 0 = not a threshold, 1 = active if greater than, -1 = active if less than
    int thresholdDataIndex; // index for dataStream that is evaluated by threshold check
    int thresholdDataType;  // what type of data stream states used for check (0 = current, 1 = average, 2 = standard deviation)
    float thresholdValue;   // state of data used for threshold check
    float breakpointValue[EVENT_STATES_MAX]; // set breakpoints between event states
};

#define MAX_EVENTS 20
int nEvents = 0;
eventTracker events[MAX_EVENTS];

// addEvent: creates a new control event and returns the index of that event in eventTracker
//        structure array
//int addEvent(eventTracker *localEvent, int *numEvents, char *eventName, char *eventNickName, char *labelPassive, char *labelActive, int eventType, int initialState)
//int addEvent(eventTracker *localEvent, int *numEvents, char *eventName, char *eventNickName, int localNumStates, char * [EVENT_NAME_SHORT] localStateName, int eventType, int initialState)
int addEvent(eventTracker *localEvent, int *numEvents, char *eventName, char *eventNickName, int eventType = 0, int initialState = 0, int localNumStates = 2, char *ls0 = "OFF", char *ls1 = "ON", char *ls2 = NULL, char *ls3 = NULL, char *ls4 = NULL)
{
    char *listEventStates[EVENT_STATES_MAX]; // list holding names of possible event states
    listEventStates[0] = ls0;  // state 0 = PASSIVE
    listEventStates[1] = ls1;   // state 1 = ACTIVE
    listEventStates[2] = ls2;        // alternative state
    listEventStates[3] = ls3;        // alternative state
    listEventStates[4] = ls4;        // alternative state

    int newEventIndex = *numEvents;
    if (newEventIndex == MAX_EVENTS)
    {
        WARN("too many control event streams", newEventIndex)
        return -1; // return error code
    }

    *numEvents = *numEvents + 1;

    // initialize key states in data structure
    localEvent[newEventIndex].eventType = eventType; //
    localEvent[newEventIndex].state = initialState;
    localEvent[newEventIndex].priorState = initialState;
    localEvent[newEventIndex].justUpdated = initialState;

    localEvent[newEventIndex].numStates = localNumStates;
    if (localNumStates > EVENT_STATES_MAX)
    {
        WARN("too many event states!", localNumStates)
        localEvent[newEventIndex].numStates = EVENT_STATES_MAX;
    }

    localEvent[newEventIndex].actionTaken = 0; // flag to indicate that the event has caused an action (and accumulates number of actions)
                                               //    localEvent[newEventIndex].numActivate = 0; // number of times the event has switched to active
                                               //    localEvent[newEventIndex].numPassivate = 0;

    unsigned long currentTime = millis();
    localEvent[newEventIndex].timeLastChange = currentTime;
    //localEvent[newEventIndex].timeLastPassivate = currentTime;
    //localEvent[newEventIndex].timeLastActivate = currentTime;

    //  initialize the event state counts and times
    for (int k = 0; k < localEvent[newEventIndex].numStates; k++)
    {
        localEvent[newEventIndex].stateCount[k] = 0;
        localEvent[newEventIndex].stateTimeStarted[k] = 0;
    }
    localEvent[newEventIndex].stateTimeStarted[initialState] = currentTime;

    // perform checks on string lengths
    int nameLength = strlen(eventName);
    if (nameLength > (EVENT_NAME_MAX - 2))
    {
        MESSAGE("name", eventName)
        WARN("event name too long", nameLength)
    }
    else
    {
        strcpy(localEvent[newEventIndex].eventName, eventName); // need to add check on length!
    }

    nameLength = strlen(eventNickName);
    if (nameLength > (EVENT_NAME_SHORT - 2))
    {
        MESSAGE("name", eventNickName)
        WARN("event nickname too long", nameLength)
    }
    else
    {
        strcpy(localEvent[newEventIndex].eventNickName, eventNickName); // need to add check on length!
    }

    // put names of PASSIVE (0) and ACTIVE (1) event states into an array
    for (int k = 0; k < localEvent[newEventIndex].numStates; k++)
    {
        nameLength = strlen(listEventStates[k]);
        if (nameLength > (EVENT_NAME_SHORT - 2))
        {
            MESSAGE("name", listEventStates[k])
            WARN("Passive Label too long", nameLength)
        }
        else
        {
            strcpy(localEvent[newEventIndex].eventStateName[k], listEventStates[k]);
        }
    }

    // nameLength = strlen(labelPassive);
    // if (nameLength > (EVENT_NAME_SHORT - 2))
    // {
    //     MESSAGE("name", labelPassive)
    //     WARN("Passive Label too long", nameLength)
    // }
    // else
    // {
    //     strcpy(localEvent[newEventIndex].passiveStateName, labelPassive); // need to add check on length!
    // }

    // nameLength = strlen(labelActive);
    // if (nameLength > (EVENT_NAME_SHORT - 2))
    // {
    //     MESSAGE("name", labelActive)
    //     WARN("Active Label too long", nameLength)
    // }
    // else
    // {
    //     strcpy(localEvent[newEventIndex].activeStateName, labelActive); // need to add check on length!
    // }

    return newEventIndex;
}

void linkEventToPin(eventTracker *localEvent, int jEvent, int kPIN)
{
    localEvent[jEvent].pin = kPIN;
    pinMode(kPIN, INPUT);
    localEvent[jEvent].state = digitalRead(kPIN);

    return;
}

void setEventBreakpoints(eventTracker *localEvent, int jEvent, int iData, float BP1, float BP2 = 0., float BP3 = 0., float BP4 = 0., float BP5 = 0., float BP6 = 0., float BP7 = 0., float BP8 = 0., float BP9 = 0., float BP10 = 0.)
{
    localEvent[jEvent].thresholdDataIndex = iData;

    localEvent[jEvent].breakpointValue[0] = BP1; // breakpoint between state 0 and state 1
    localEvent[jEvent].breakpointValue[1] = BP2;
    localEvent[jEvent].breakpointValue[2] = BP3;
    localEvent[jEvent].breakpointValue[3] = BP3;
    localEvent[jEvent].breakpointValue[4] = BP4;
    localEvent[jEvent].breakpointValue[5] = BP5;
    localEvent[jEvent].breakpointValue[6] = BP6;
    localEvent[jEvent].breakpointValue[7] = BP7;
    localEvent[jEvent].breakpointValue[8] = BP8;
    localEvent[jEvent].breakpointValue[9] = BP9;
    localEvent[jEvent].breakpointValue[10] = BP10; // breakpoint between state 9 and state 10
}

void updateEventState(eventTracker *localEvent, int jEvent, int newState, unsigned long loopTime)
{
    // Event State has changed!
    localEvent[jEvent].justUpdated = 1; // indicates the current state is a new state
    localEvent[jEvent].state = newState;
    localEvent[jEvent].stateDuration = loopTime - localEvent[jEvent].timeLastChange;
    DEBUG(localEvent[jEvent].stateDuration)
    localEvent[jEvent].stateCount[newState] = localEvent[jEvent].stateCount[newState] + 1;
    localEvent[jEvent].stateTimeStarted[newState] = loopTime;

    localEvent[jEvent].stateDuration = localEvent[jEvent].stateTimeStarted[newState] - localEvent[jEvent].stateTimeStarted[localEvent[jEvent].priorState];
    DEBUG(localEvent[jEvent].stateDuration)

    localEvent[jEvent].timeLastChange = loopTime;
    return;
}

int reportEventToSerial(eventTracker *localEvents, int nEventsLocal, int jEvent)
{
    // print out event notification
    Serial.print("EVENT: millis = ");
    Serial.print(events[jEvent].timeLastChange);
    Serial.print(" event ");
    Serial.print(events[jEvent].eventName);
    Serial.println(" ----------------------------------------------");

    Serial.print("FROM state = ");
    Serial.println(events[jEvent].eventStateName[events[jEvent].priorState]);
    Serial.print("TO state = ");
    Serial.print(events[jEvent].eventStateName[events[jEvent].state]);
    Serial.print(" previous state duration = ");
    Serial.println(events[jEvent].stateDuration);
    return 1;
}

#ifdef USE_SD
int reportEventToFile(char *fullFileName, eventTracker *localEvents, int nEventsLocal, int jEvent, char *separator, int count, int headerFlag)
{
    // open file to log information
    File tmpFile;
    tmpFile = SD.open(fullFileName, FILE_WRITE);
    // if the file opened okay, write to it:
    if (tmpFile)
    {

        if (headerFlag == 1)
        {
            // for first column, print Device code
            tmpFile.print(deviceCode);

            // for second column, print count tracking number of lines
            tmpFile.print(separator);
            tmpFile.print("count");
            tmpFile.print(separator);
            tmpFile.print("EVENT");
            tmpFile.print(separator);
            tmpFile.print("eventName");
            tmpFile.print(separator);
            tmpFile.print("Direction");
            tmpFile.print(separator);
            tmpFile.print("State");
            tmpFile.print(separator);
            tmpFile.print("tStart");
            tmpFile.print(separator);
            tmpFile.print("tEnd");
            tmpFile.print(separator);
            tmpFile.print("Duration");
            tmpFile.print(separator);
            tmpFile.print("Count");
            tmpFile.print(separator);
            tmpFile.print("fullName");
            tmpFile.println();
        }
        else
        {
            // for first column, print Device code
            tmpFile.print(deviceCode);

            // for second column, print count tracking number of lines
            tmpFile.print(separator);
            tmpFile.print(count);
            tmpFile.print(separator);
            tmpFile.print("EVENT");
            // print line with info about the new state = "TO"
            tmpFile.print(separator);
            tmpFile.print(localEvents[jEvent].eventNickName);
            tmpFile.print(separator);
            tmpFile.print("FROM");
            tmpFile.print(separator);
            tmpFile.print(localEvents[jEvent].eventStateName[localEvents[jEvent].priorState]);
            tmpFile.print(separator);
            tmpFile.print(localEvents[jEvent].stateTimeStarted[localEvents[jEvent].priorState]);
            tmpFile.print(separator);
            tmpFile.print(localEvents[jEvent].stateTimeStarted[localEvents[jEvent].state]);
            tmpFile.print(separator);
            tmpFile.print(localEvents[jEvent].stateDuration);
            tmpFile.print(separator);
            tmpFile.print(localEvents[jEvent].stateCount[localEvents[jEvent].priorState]);
            tmpFile.print(separator);
            tmpFile.print(localEvents[jEvent].eventName);
            tmpFile.println();

            // for first column, print Device code
            tmpFile.print(deviceCode);

            // for second column, print count tracking number of lines
            tmpFile.print(separator);
            tmpFile.print(count);
            tmpFile.print(separator);
            tmpFile.print("EVENT");
            // print line with info about the prior state = "FROM"
            tmpFile.print(separator);
            tmpFile.print(localEvents[jEvent].eventNickName);
            tmpFile.print(separator);
            tmpFile.print("TO");
            tmpFile.print(separator);
            tmpFile.print(localEvents[jEvent].eventStateName[localEvents[jEvent].state]);
            tmpFile.print(separator);
            tmpFile.print(localEvents[jEvent].stateTimeStarted[localEvents[jEvent].state]);
            tmpFile.print(separator);
            tmpFile.print(localEvents[jEvent].stateTimeStarted[localEvents[jEvent].state]);
            tmpFile.print(separator);
            tmpFile.print(localEvents[jEvent].stateDuration);
            tmpFile.print(separator);
            tmpFile.print(localEvents[jEvent].stateCount[localEvents[jEvent].state]);
            tmpFile.print(separator);
            tmpFile.print(localEvents[jEvent].eventName);
            tmpFile.println();

            // print blank line
            tmpFile.println();
        }
        tmpFile.close();
    }
    else
    {
        return -1;
    }
    return 1;
}
#endif