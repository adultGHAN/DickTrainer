#include <windows.h>
#include <math.h>
#include <stdlib.h>
#include "types.h"
#include "audio.h"

#pragma comment( lib, "winmm.lib" )

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SAMPLE_RATE 44100

static void PlayBeep( int nFrequencyHz, int nDurationMs, int nVolume )
{
    short *psBuffer = NULL;         /* sample buffer */
    HWAVEOUT hWave = NULL;          /* wave output handle */
    WAVEFORMATEX stWfx = { 0 };     /* wave format */
    WAVEHDR stWh = { 0 };           /* wave header */
    int nNumSamples = 0;            /* total sample count */
    int nIdx = 0;                   /* sample loop index */
    double dAmplitude = 0.0;        /* sample amplitude */

    if( nVolume < 0 )
    {
        nVolume = 0;
    }
    if( nVolume > 100 )
    {
        nVolume = 100;
    }

    nNumSamples = ( SAMPLE_RATE * nDurationMs ) / 1000;
    psBuffer = ( short * )malloc( nNumSamples * sizeof( short ) );
    if( !psBuffer )
    {
        return;
    }

    dAmplitude = ( nVolume / 100.0 ) * 32767.0;
    for( nIdx = 0; nIdx < nNumSamples; nIdx++ )
    {
        psBuffer[ nIdx ] = ( short )( dAmplitude * sin( 2.0 * M_PI * nFrequencyHz * nIdx / SAMPLE_RATE ) );
    }

    stWfx.wFormatTag      = WAVE_FORMAT_PCM;
    stWfx.nChannels       = 1;
    stWfx.nSamplesPerSec  = SAMPLE_RATE;
    stWfx.wBitsPerSample  = 16;
    stWfx.nBlockAlign     = ( WORD )( stWfx.nChannels * stWfx.wBitsPerSample / 8 );
    stWfx.nAvgBytesPerSec = stWfx.nSamplesPerSec * stWfx.nBlockAlign;

    if( waveOutOpen( &hWave, 0, &stWfx, 0, 0, CALLBACK_NULL ) != MMSYSERR_NOERROR )
    {
        free( psBuffer );
        return;
    }

    waveOutSetVolume( hWave, 0xFFFFFFFF );

    stWh.lpData         = ( LPSTR )psBuffer;
    stWh.dwBufferLength = ( DWORD )( nNumSamples * sizeof( short ) );

    waveOutPrepareHeader( hWave, &stWh, sizeof( WAVEHDR ) );
    waveOutWrite( hWave, &stWh, sizeof( WAVEHDR ) );

    Sleep( ( DWORD )nDurationMs );
    while( !( stWh.dwFlags & WHDR_DONE ) )
    {
        Sleep( 10 );
    }

    waveOutUnprepareHeader( hWave, &stWh, sizeof( WAVEHDR ) );
    waveOutClose( hWave );
    free( psBuffer );
}

DWORD WINAPI BeepHigh( LPVOID pParam )
{
    while( 1 )
    {
        WaitForSingleObject( g_hHighBeepEvent, INFINITE );
        PlayBeep( 2000, 250, g_nBeepVolume );
    }

    return 0;
}

DWORD WINAPI BeepLow( LPVOID pParam )
{
    while( 1 )
    {
        WaitForSingleObject( g_hLowBeepEvent, INFINITE );
        PlayBeep( 1000, 250, g_nBeepVolume );
    }

    return 0;
}

DWORD WINAPI BeepMid( LPVOID pParam )
{
    while( 1 )
    {
        WaitForSingleObject( g_hMidBeepEvent, INFINITE );
        PlayBeep( 500, 250, g_nBeepVolume );
    }

    return 0;
}
