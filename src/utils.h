#pragma once

#include <windows.h>

TCHAR *AllocTCHARFromUTF8( const char *pszSrc );
void InitConsole( void );
double GenerateGaussianNoise( double dMu, double dSigma );
double GenerateSample( const TCHAR *szDist, double dMean, double dStd );
int GetYFromPos( int nPos, int nTopY, int nBottomY );
