#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "types.h"
#include "utils.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Convert UTF-8 char* (e.g. from cJSON) to a newly allocated TCHAR string */
TCHAR *AllocTCHARFromUTF8( const char *pszSrc )
{
    int nLen = 0;                   /* required buffer length in wide characters */
    TCHAR *pszDst = NULL;           /* allocated output TCHAR string */
    if( !pszSrc )
    {
        return NULL;
    }
#ifdef UNICODE
    nLen = MultiByteToWideChar( CP_UTF8, 0, pszSrc, -1, NULL, 0 );
    pszDst = ( TCHAR * )malloc( nLen * sizeof( TCHAR ) );
    if( pszDst )
    {
        MultiByteToWideChar( CP_UTF8, 0, pszSrc, -1, pszDst, nLen );
    }
#else
    pszDst = _strdup( pszSrc );
#endif
    return pszDst;
}

void InitConsole( void )
{
    setvbuf( stdout, NULL, _IONBF, 0 );
}

double GenerateGaussianNoise( double dMu, double dSigma )
{
    static double dZ1 = 0.0;        /* second Box-Muller sample stored for next call */
    static int nGen = 0;            /* toggle flag for alternating sample generation */
    double dU1 = 0.0;               /* uniform random value 1 */
    double dU2 = 0.0;               /* uniform random value 2 */
    double dZ0 = 0.0;               /* first Box-Muller sample */

    nGen = !nGen;
    if( !nGen )
    {
        return dZ1 * dSigma + dMu;
    }

    do
    {
        dU1 = rand() * ( 1.0 / RAND_MAX );
        dU2 = rand() * ( 1.0 / RAND_MAX );
    }
    while( dU1 <= 1e-7 );

    dZ0 = sqrt( -2.0 * log( dU1 ) ) * cos( 2.0 * M_PI * dU2 );
    dZ1 = sqrt( -2.0 * log( dU1 ) ) * sin( 2.0 * M_PI * dU2 );

    return dZ0 * dSigma + dMu;
}

int GetYFromPos( int nPos, int nTopY, int nBottomY )
{
    int nRange = 0;                 /* total Y coordinate range */
    int nOffset = 0;                /* offset based on position */

    nRange = nBottomY - nTopY;
    nOffset = ( nPos - 1 ) * ( nRange / 4 );
    return nBottomY - nOffset;
}

double GenerateSample( const TCHAR *szDist, double dMean, double dStd )
{
    double dU = 0.0;                /* uniform random value [0,1) */

    if( _tcscmp( szDist, TEXT( "uniform" ) ) == 0 )
    {
        dU = ( double )rand() / ( ( double )RAND_MAX + 1.0 );
        return ( dMean - dStd ) + dU * ( dStd * 2.0 );
    }
    return GenerateGaussianNoise( dMean, dStd );
}
