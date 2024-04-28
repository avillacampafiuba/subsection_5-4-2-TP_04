//=====[Libraries]=============================================================

#include "mbed.h"
#include "arm_book_lib.h"

#include "pc_serial_com.h"

#include "siren.h"
#include "fire_alarm.h"
#include "code.h"
#include "date_and_time.h"
#include "temperature_sensor.h"
#include "gas_sensor.h"
#include "event_log.h"

//=====[Comentarios de los cambios]============================================

/*
El comando para setear fecha y hora era bloqueante porque se quedaba esperando a que el usuario ingrese todos los dígitos.
Para hacerlo no bloqueante agrandamos la máquina de estados pcSerialComMode con un nuevo modo: PC_SERIAL_SET_DATE_AND_TIME

El comando commandSetDateAndTime() ahora hace entrar a la máquina en este estado. Además se creó la máquina de estados
dateAndTimeSettingMode para que el usuario ingrese los datos de la misma forma que lo hace para ingresar el código
de desbloqueo de la alarma o para cambiar dicho código. Es decir, que cuando el usuario ingresa un dígito se entra en
esta máquina de estados y se guarda dicho dígito donde corresponda. El programa continúa funcionando normalmente hasta
que el usuario ingresa el siguiente dígito. Cuando se ingresan todos los dígitos se setea la fecha y hora y la
máquina de estados pcSerialComMode vuelve al modo PC_SERIAL_COMMANDS
*/

//=====[Declaration of private defines]========================================

#define TEST_ORIGINAL 0
#define TEST_1 1
#define TEST_X TEST_1

//=====[Declaration of private data types]=====================================

#if TEST_X == TEST_ORIGINAL
typedef enum{
    PC_SERIAL_COMMANDS,
    PC_SERIAL_GET_CODE,
    PC_SERIAL_SAVE_NEW_CODE,
} pcSerialComMode_t;
#endif  // TEST_ORIGINAL

#if TEST_X == TEST_1
typedef enum{
    PC_SERIAL_COMMANDS,
    PC_SERIAL_GET_CODE,
    PC_SERIAL_SAVE_NEW_CODE,
    PC_SERIAL_SET_DATE_AND_TIME,
} pcSerialComMode_t;

typedef enum{
    SET_YEAR,
    SET_MONTH,
    SET_DAY,
    SET_HOUR,
    SET_MINUTE,
    SET_SECOND,
} dateAndTimeSettingMode_t;
#endif  // TEST_1

//=====[Declaration and initialization of public global objects]===============

UnbufferedSerial uartUsb(USBTX, USBRX, 115200);

//=====[Declaration of external public global variables]=======================

//=====[Declaration and initialization of public global variables]=============

char codeSequenceFromPcSerialCom[CODE_NUMBER_OF_KEYS];

//=====[Declaration and initialization of private global variables]============

static pcSerialComMode_t pcSerialComMode = PC_SERIAL_COMMANDS;
static bool codeComplete = false;
static int numberOfCodeChars = 0;

#if TEST_X == TEST_1
    static dateAndTimeSettingMode_t dateAndTimeSettingMode;

    static int numberOfYearChars = 0;
    static int numberOfMonthChars = 0;
    static int numberOfDayChars = 0;
    static int numberOfHourChars = 0;
    static int numberOfMinuteChars = 0;
    static int numberOfSecondChars = 0;
#endif  // TEST_1


//=====[Declarations (prototypes) of private functions]========================

static void pcSerialComStringRead( char* str, int strLength );

static void pcSerialComGetCodeUpdate( char receivedChar );
static void pcSerialComSaveNewCodeUpdate( char receivedChar );
#if TEST_X == TEST_1
static void pcSerialComSetDateAndTimeUpdate( char receivedChar );
#endif  // TEST_1

static void pcSerialComCommandUpdate( char receivedChar );

static void availableCommands();
static void commandShowCurrentAlarmState();
static void commandShowCurrentGasDetectorState();
static void commandShowCurrentOverTemperatureDetectorState();
static void commandEnterCodeSequence();
static void commandEnterNewCode();
static void commandShowCurrentTemperatureInCelsius();
static void commandShowCurrentTemperatureInFahrenheit();
static void commandSetDateAndTime();
static void commandShowDateAndTime();
static void commandShowStoredEvents();

//=====[Implementations of public functions]===================================

void pcSerialComInit()
{
    availableCommands();
}

char pcSerialComCharRead()
{
    char receivedChar = '\0';
    if( uartUsb.readable() ) {
        uartUsb.read( &receivedChar, 1 );
    }
    return receivedChar;
}

void pcSerialComStringWrite( const char* str )
{
    uartUsb.write( str, strlen(str) );
}

void pcSerialComUpdate()
{
    char receivedChar = pcSerialComCharRead();
    if( receivedChar != '\0' ) {
        switch ( pcSerialComMode ) {
            case PC_SERIAL_COMMANDS:
                pcSerialComCommandUpdate( receivedChar );
            break;

            case PC_SERIAL_GET_CODE:
                pcSerialComGetCodeUpdate( receivedChar );
            break;

            case PC_SERIAL_SAVE_NEW_CODE:
                pcSerialComSaveNewCodeUpdate( receivedChar );
            break;
            #if TEST_X == TEST_1
            case PC_SERIAL_SET_DATE_AND_TIME:
                pcSerialComSetDateAndTimeUpdate( receivedChar );
            break;
            #endif
            default:
                pcSerialComMode = PC_SERIAL_COMMANDS;
            break;
        }
    }    
}

bool pcSerialComCodeCompleteRead()
{
    return codeComplete;
}

void pcSerialComCodeCompleteWrite( bool state )
{
    codeComplete = state;
}

//=====[Implementations of private functions]==================================

static void pcSerialComStringRead( char* str, int strLength )
{
    #if TEST == TEST_OG 
    int strIndex;
    for ( strIndex = 0; strIndex < strLength; strIndex++) {
        uartUsb.read( &str[strIndex] , 1 );
        uartUsb.write( &str[strIndex] ,1 );
    }
    str[strLength]='\0';
    
    #endif

    #if TEST == TEST_NB
    #endif

}

static void pcSerialComGetCodeUpdate( char receivedChar )
{
    codeSequenceFromPcSerialCom[numberOfCodeChars] = receivedChar;
    pcSerialComStringWrite( "*" );
    numberOfCodeChars++;
   if ( numberOfCodeChars >= CODE_NUMBER_OF_KEYS ) {
        pcSerialComMode = PC_SERIAL_COMMANDS;
        codeComplete = true;
        numberOfCodeChars = 0;
    } 
}

static void pcSerialComSaveNewCodeUpdate( char receivedChar )
{
    static char newCodeSequence[CODE_NUMBER_OF_KEYS];

    newCodeSequence[numberOfCodeChars] = receivedChar;
    pcSerialComStringWrite( "*" );
    numberOfCodeChars++;
    if ( numberOfCodeChars >= CODE_NUMBER_OF_KEYS ) {
        pcSerialComMode = PC_SERIAL_COMMANDS;
        numberOfCodeChars = 0;
        codeWrite( newCodeSequence );
        pcSerialComStringWrite( "\r\nNew code configured\r\n\r\n" );
    } 
}

#if TEST_X == TEST_1
static void pcSerialComSetDateAndTimeUpdate( char receivedChar )
{
    static char newYear[YEAR_NUMBER_OF_KEYS];
    static char newMonth[MONTH_NUMBER_OF_KEYS];
    static char newDay[DAY_NUMBER_OF_KEYS];
    static char newHour[HOUR_NUMBER_OF_KEYS];
    static char newMinute[MINUTE_NUMBER_OF_KEYS];
    static char newSecond[SECOND_NUMBER_OF_KEYS];

    switch ( dateAndTimeSettingMode ) {
        case SET_YEAR:
            newYear[numberOfYearChars] = receivedChar;
            numberOfYearChars++;
            if ( numberOfYearChars >= YEAR_NUMBER_OF_KEYS ) {
                dateAndTimeSettingMode = SET_MONTH;
                numberOfYearChars = 0;
                pcSerialComStringWrite("\r\nType two digits for the current month (01-12): ");
            }
        break;
        case SET_MONTH:
            newMonth[numberOfMonthChars] = receivedChar;
            numberOfMonthChars++;
            if ( numberOfMonthChars >= MONTH_NUMBER_OF_KEYS ) {
                dateAndTimeSettingMode = SET_DAY;
                numberOfMonthChars = 0;
                pcSerialComStringWrite("\r\nType two digits for the current day (01-31): ");
            }
        break;
        case SET_DAY:
            newDay[numberOfDayChars] = receivedChar;
            numberOfDayChars++;
            if ( numberOfDayChars >= DAY_NUMBER_OF_KEYS ) {
                dateAndTimeSettingMode = SET_HOUR;
                numberOfDayChars = 0;
                pcSerialComStringWrite("\r\nType two digits for the current hour (00-23): ");
            }
        break;
        case SET_HOUR:
            newHour[numberOfHourChars] = receivedChar;
            numberOfHourChars++;
            if ( numberOfHourChars >= HOUR_NUMBER_OF_KEYS ) {
                dateAndTimeSettingMode = SET_MINUTE;
                numberOfHourChars = 0;
                pcSerialComStringWrite("\r\nType two digits for the current minutes (00-59): ");
            }
        break;
        case SET_MINUTE:
            newMinute[numberOfMinuteChars] = receivedChar;
            numberOfMinuteChars++;
            if ( numberOfMinuteChars >= MINUTE_NUMBER_OF_KEYS ) {
                dateAndTimeSettingMode = SET_SECOND;
                numberOfMinuteChars = 0;
                pcSerialComStringWrite("\r\nType two digits for the current seconds (00-59): ");
            }
        break;
        case SET_SECOND:
            newSecond[numberOfSecondChars] = receivedChar;
            numberOfSecondChars++;
            if ( numberOfSecondChars >= SECOND_NUMBER_OF_KEYS ) {
                numberOfSecondChars = 0;
                pcSerialComStringWrite("\r\nDate and time has been set\r\n");
                dateAndTimeWrite( atoi(newYear), atoi(newMonth), atoi(newDay), 
                    atoi(newHour), atoi(newMinute), atoi(newSecond) );
                pcSerialComMode = PC_SERIAL_COMMANDS;
            }
        break;
        default:
        break;
    }
}
#endif  // TEST_1

static void pcSerialComCommandUpdate( char receivedChar )
{
    switch (receivedChar) {
        case '1': commandShowCurrentAlarmState(); break;
        case '2': commandShowCurrentGasDetectorState(); break;
        case '3': commandShowCurrentOverTemperatureDetectorState(); break;
        case '4': commandEnterCodeSequence(); break;
        case '5': commandEnterNewCode(); break;
        case 'c': case 'C': commandShowCurrentTemperatureInCelsius(); break;
        case 'f': case 'F': commandShowCurrentTemperatureInFahrenheit(); break;
        case 's': case 'S': commandSetDateAndTime(); break;
        case 't': case 'T': commandShowDateAndTime(); break;
        case 'e': case 'E': commandShowStoredEvents(); break;
        default: availableCommands(); break;
    } 
}

static void availableCommands()
{
    pcSerialComStringWrite( "Available commands:\r\n" );
    pcSerialComStringWrite( "Press '1' to get the alarm state\r\n" );
    pcSerialComStringWrite( "Press '2' to get the gas detector state\r\n" );
    pcSerialComStringWrite( "Press '3' to get the over temperature detector state\r\n" );
    pcSerialComStringWrite( "Press '4' to enter the code to deactivate the alarm\r\n" );
    pcSerialComStringWrite( "Press '5' to enter a new code to deactivate the alarm\r\n" );
    pcSerialComStringWrite( "Press 'f' or 'F' to get lm35 reading in Fahrenheit\r\n" );
    pcSerialComStringWrite( "Press 'c' or 'C' to get lm35 reading in Celsius\r\n" );
    pcSerialComStringWrite( "Press 's' or 'S' to set the date and time\r\n" );
    pcSerialComStringWrite( "Press 't' or 'T' to get the date and time\r\n" );
    pcSerialComStringWrite( "Press 'e' or 'E' to get the stored events\r\n" );
    pcSerialComStringWrite( "\r\n" );
}

static void commandShowCurrentAlarmState()
{
    if ( sirenStateRead() ) {
        pcSerialComStringWrite( "The alarm is activated\r\n");
    } else {
        pcSerialComStringWrite( "The alarm is not activated\r\n");
    }
}

static void commandShowCurrentGasDetectorState()
{
    if ( gasDetectorStateRead() ) {
        pcSerialComStringWrite( "Gas is being detected\r\n");
    } else {
        pcSerialComStringWrite( "Gas is not being detected\r\n");
    }    
}

static void commandShowCurrentOverTemperatureDetectorState()
{
    if ( overTemperatureDetectorStateRead() ) {
        pcSerialComStringWrite( "Temperature is above the maximum level\r\n");
    } else {
        pcSerialComStringWrite( "Temperature is below the maximum level\r\n");
    }
}

static void commandEnterCodeSequence()
{
    if( sirenStateRead() ) {
        pcSerialComStringWrite( "Please enter the four digits numeric code " );
        pcSerialComStringWrite( "to deactivate the alarm: " );
        pcSerialComMode = PC_SERIAL_GET_CODE;
        codeComplete = false;
        numberOfCodeChars = 0;
    } else {
        pcSerialComStringWrite( "Alarm is not activated.\r\n" );
    }
}

static void commandEnterNewCode()
{
    pcSerialComStringWrite( "Please enter the new four digits numeric code " );
    pcSerialComStringWrite( "to deactivate the alarm: " );
    numberOfCodeChars = 0;
    pcSerialComMode = PC_SERIAL_SAVE_NEW_CODE;

}

static void commandShowCurrentTemperatureInCelsius()
{
    char str[100] = "";
    sprintf ( str, "Temperature: %.2f \xB0 C\r\n",
                    temperatureSensorReadCelsius() );
    pcSerialComStringWrite( str );  
}

static void commandShowCurrentTemperatureInFahrenheit()
{
    char str[100] = "";
    sprintf ( str, "Temperature: %.2f \xB0 C\r\n",
                    temperatureSensorReadFahrenheit() );
    pcSerialComStringWrite( str );  
}

#if TEST_X == TEST_ORIGINAL
static void commandSetDateAndTime()
{
    char year[5] = "";
    char month[3] = "";
    char day[3] = "";
    char hour[3] = "";
    char minute[3] = "";
    char second[3] = "";
    
    pcSerialComStringWrite("\r\nType four digits for the current year (YYYY): ");
    pcSerialComStringRead( year, 4);
    pcSerialComStringWrite("\r\n");

    pcSerialComStringWrite("Type two digits for the current month (01-12): ");
    pcSerialComStringRead( month, 2);
    pcSerialComStringWrite("\r\n");

    pcSerialComStringWrite("Type two digits for the current day (01-31): ");
    pcSerialComStringRead( day, 2);
    pcSerialComStringWrite("\r\n");

    pcSerialComStringWrite("Type two digits for the current hour (00-23): ");
    pcSerialComStringRead( hour, 2);
    pcSerialComStringWrite("\r\n");

    pcSerialComStringWrite("Type two digits for the current minutes (00-59): ");
    pcSerialComStringRead( minute, 2);
    pcSerialComStringWrite("\r\n");

    pcSerialComStringWrite("Type two digits for the current seconds (00-59): ");
    pcSerialComStringRead( second, 2);
    pcSerialComStringWrite("\r\n");
    
    pcSerialComStringWrite("Date and time has been set\r\n");

    dateAndTimeWrite( atoi(year), atoi(month), atoi(day), 
        atoi(hour), atoi(minute), atoi(second) );
}
#endif  // TEST_ORIGINAL

#if TEST_X == TEST_1
static void commandSetDateAndTime()
{
    numberOfYearChars = 0;
    numberOfMonthChars = 0;
    numberOfDayChars = 0;
    numberOfHourChars = 0;
    numberOfMinuteChars = 0;
    numberOfSecondChars = 0;
    pcSerialComMode = PC_SERIAL_SET_DATE_AND_TIME;
    dateAndTimeSettingMode = SET_YEAR;
    pcSerialComStringWrite("\r\nType four digits for the current year (YYYY): ");
}
#endif  // TEST_1

static void commandShowDateAndTime()
{
    char str[100] = "";
    sprintf ( str, "Date and Time = %s", dateAndTimeRead() );
    pcSerialComStringWrite( str );
    pcSerialComStringWrite("\r\n");
}

static void commandShowStoredEvents()
{
    char str[EVENT_STR_LENGTH] = "";
    int i;
    for (i = 0; i < eventLogNumberOfStoredEvents(); i++) {
        eventLogRead( i, str );
        pcSerialComStringWrite( str );   
        pcSerialComStringWrite( "\r\n" );                    
    }
}