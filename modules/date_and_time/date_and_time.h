//=====[#include guards - begin]===============================================

#ifndef _DATE_AND_TIME_H_
#define _DATE_AND_TIME_H_

//=====[Declaration of public defines]=========================================

#define YEAR_NUMBER_OF_KEYS 4
#define MONTH_NUMBER_OF_KEYS 2
#define DAY_NUMBER_OF_KEYS 2
#define HOUR_NUMBER_OF_KEYS 2
#define MINUTE_NUMBER_OF_KEYS 2
#define SECOND_NUMBER_OF_KEYS 2

//=====[Declaration of public data types]======================================

//=====[Declarations (prototypes) of public functions]=========================

char* dateAndTimeRead();

void dateAndTimeWrite( int year, int month, int day, 
                       int hour, int minute, int second );

//=====[#include guards - end]=================================================

#endif // _DATE_AND_TIME_H_