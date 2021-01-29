#ifdef ENABLEDEBUG
// this compiler macro is a quick way to print out data for debugging
//    when enabled, the line:
//    DBG([variable name])
//
//    for example:
//    DBG(current_time)
//
//    expands to (prior to compiling and DEBUG_LEVEL will be replaced by the value indicated):
//    if (7 < DEBUG_LEVEL) {
//      Serial.print("DEBUG: var: ");
//      Serial.print("current_time");
//      Serial.print(" = ");
//      Serial.println(current_time);
//    }

// DEBUG will always print variable = value when debug is enabled
#define DEBUG(b)                  \
    Serial.print("DEBUG: var: "); \
    Serial.print(#b);             \
    Serial.print(" = ");          \
    Serial.println(b);

// DEBUG1 through DEBUG4 will print if their level is less than or equal to DEBUG_LEVEL
#define DEBUG1(b)                     \
    if (0 < DEBUG_LEVEL)              \
    {                                 \
        Serial.print("DEBUG1: var: "); \
        Serial.print(#b);             \
        Serial.print(" = ");          \
        Serial.println(b);            \
    }
#define DEBUG2(b)                     \
    if (1 < DEBUG_LEVEL)              \
    {                                 \
        Serial.print("DEBUG2: var: "); \
        Serial.print(#b);             \
        Serial.print(" = ");          \
        Serial.println(b);            \
    }
#define DEBUG3(b)                     \
    if (2 < DEBUG_LEVEL)              \
    {                                 \
        Serial.print("DEBUG3: var: "); \
        Serial.print(#b);             \
        Serial.print(" = ");          \
        Serial.println(b);            \
    }
#define DEBUG4(b)                     \
    if (3 < DEBUG_LEVEL)              \
    {                                 \
        Serial.print("DEBUG4: var: "); \
        Serial.print(#b);             \
        Serial.print(" = ");          \
        Serial.println(b);            \
    }
//
// also create  messages
#define MESSAGE(a, b)                                               \
    Serial.println("");  \
    Serial.println("-----------------------------------------"); \
    Serial.print(a);                                             \
    Serial.print(" for variable: ");                             \
    Serial.print(#b);                                            \
    Serial.print(" = ");                                         \
    Serial.println(b);                                           \
    Serial.println("-----------------------------------------"); \
    Serial.println("");

// also create warning messages
#define WARN(a, b)                                               \
    Serial.println("");  \
    Serial.println("WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW"); \
    Serial.print("WARNING: ");                                   \
    Serial.print(a);                                             \
    Serial.print(" for variable: ");                             \
    Serial.print(#b);                                            \
    Serial.print(" = ");                                         \
    Serial.println(b);                                           \
    Serial.println("WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW"); \
    Serial.println("");
#else
#define DEBUG(b)  // do not include in code
#define DEBUG1(b) // do not include in code
#define DEBUG2(b) // do not include in code
#define DEBUG3(b) // do not include in code
#define DEBUG4(b) // do not include in code
#define WARN(a,b) // do not include in code
#define MESSAGE(a,b) // do not include in code
#endif