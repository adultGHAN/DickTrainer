#pragma once

#include "types.h"

DataSizes CalculateDataSizes( const TCHAR *szFilename );
int LoadAllDataFromJson( const TCHAR *szFilename );
void AppendMotionWithIntervals( int nMotionIdx, double dTargetLevel, int nCustomerIdx, int nTotalCustomers, int nCourseIdx, int nTotalCourses, const TCHAR *szShape, double dCustomerLevel, double *pdAccumulatedTime );
void GenerateSequences( int nCount );
void SetupNextCustomer( void );
double GetYAtTime( double dTime, int nTopY, int nBottomY );
void FreeAllData( void );
