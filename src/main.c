#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "types.h"
#include "utils.h"
#include "logic.h"
#include "render.h"
#include "audio.h"
#include "version.h"

/* --- Global Variable Definitions --- */
/* Customer info */
CustomerInfo *g_pstCustomers = NULL;
int g_nCustomerCount = 0;
int g_nCustomerCapacity = 0;

/* Motion info */
Motion *g_pstMotions = NULL;
int g_nMotionCount = 0;
int g_nMotionCapacity = 0;

/* Size info */
SizeInfo *g_pstSizes = NULL;
int g_nSizeCount = 0;
int g_nSizeCapacity = 0;

/* Distribution info */
DistInfo g_stCustomerDist = { 0 };
DistInfo g_stSizeDist = { 0 };

/* Sequence info */
SequenceAct *g_pstSequence = NULL;
int g_nActCount = 0;
int g_nSequenceCapacity = 0;

/* Current execution state */
int g_nCurrentAct = 0;
double g_dCurrentTimeInAct = 0.0;
int g_bFirstBeepPlayed = 0;

/* Current customer range */
int g_nCurrentCustomerStartAct = 0;
int g_nCurrentCustomerEndAct = 0;
int g_nCurrentCustomerIdx = 0;
int g_bGraphStarted = 0;

/* GUI state */
int g_nGUIState = 0;
TCHAR g_szInputBuf[ 16 ] = { 0 };
int g_bWaitingForEnter = 0;
int g_bPaused = 0;
int g_bCompleted = 0;

/* Transparency */
int g_nTransparency = 0;

/* Volume */
int g_nBeepVolume = 0;
LARGE_INTEGER g_liVolumeChangeTime = { 0 };

/* Transparency change timestamp */
LARGE_INTEGER g_liTransparencyChangeTime = { 0 };

/* Transparency percentage (0~100) */
int g_nTransparencyPct = 0;

/* Last input timestamp */
LARGE_INTEGER g_liLastInputTime = { 0 };

/* Dick position */
TCHAR g_szHorizontal[ 20 ] = { 0 };

/* JSON file path */
TCHAR g_szJsonFilePath[ MAX_PATH ] = { 0 };

/* Vertical */
TCHAR g_szVertical[ 20 ] = { 0 };

/* Mode */
TCHAR g_szMode[ 20 ] = { 0 };

/* Log flag */
int g_bLog = 0;

/* Performance timing */
LARGE_INTEGER g_liFrequency = { 0 };
LARGE_INTEGER g_liStartTime = { 0 };
LARGE_INTEGER g_liLastTime = { 0 };

/* Event handles */
HANDLE g_hHighBeepEvent = NULL;
HANDLE g_hLowBeepEvent = NULL;
HANDLE g_hMidBeepEvent = NULL;

LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );

int ResetAllState( void )
{
    OPENFILENAME stOfn = { 0 };      /* open file dialog struct */
    TCHAR szFilePath[ MAX_PATH ] = { 0 }; /* selected file path */
    DataSizes stDataSizes = { 0 };  /* json data sizes */

    /* Open file dialog */
    memset( &stOfn, 0, sizeof( OPENFILENAME ) );
    stOfn.lStructSize = sizeof( OPENFILENAME );
    stOfn.lpstrFilter = TEXT( "JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0" );
    stOfn.lpstrFile   = szFilePath;
    stOfn.nMaxFile    = MAX_PATH;
    stOfn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    stOfn.lpstrDefExt = TEXT( "json" );
    if( !GetOpenFileName( &stOfn ) )
    {
        return 0;
    }
    _tcsncpy( g_szJsonFilePath, szFilePath, MAX_PATH - 1 );
    g_szJsonFilePath[ MAX_PATH - 1 ] = TEXT( '\0' );

    FreeAllData();

    stDataSizes = CalculateDataSizes( g_szJsonFilePath );
    if( stDataSizes.nCustomerCount == 0 || stDataSizes.nMaxSequenceCount == 0 )
    {
        _tprintf( TEXT( "JSON Calculation Failed!\n" ) );
        return 0;
    }

    g_pstCustomers = ( CustomerInfo * )malloc( stDataSizes.nCustomerCount * sizeof( CustomerInfo ) );
    g_pstMotions   = stDataSizes.nMotionCount > 0 ? ( Motion * )malloc( stDataSizes.nMotionCount * sizeof( Motion ) ) : NULL;
    g_pstSizes     = stDataSizes.nSizeCount > 0 ? ( SizeInfo * )malloc( stDataSizes.nSizeCount * sizeof( SizeInfo ) ) : NULL;
    g_pstSequence  = ( SequenceAct * )malloc( stDataSizes.nMaxSequenceCount * sizeof( SequenceAct ) );

    g_nCustomerCapacity = stDataSizes.nCustomerCount;
    g_nMotionCapacity   = stDataSizes.nMotionCount;
    g_nSizeCapacity     = stDataSizes.nSizeCount;
    g_nSequenceCapacity = stDataSizes.nMaxSequenceCount;

    if( !LoadAllDataFromJson( g_szJsonFilePath ) )
    {
        _tprintf( TEXT( "JSON Load Failed!\n" ) );
        FreeAllData();
        return 0;
    }

    g_dCurrentTimeInAct = 0.0;
    g_bFirstBeepPlayed  = 0;
    g_bGraphStarted     = 0;

    /* For random mode, reset customer range here.
       For sequence mode, LoadAllDataFromJson already set the correct range. */
    if( _tcscmp( g_szMode, TEXT( "sequence" ) ) != 0 )
    {
        g_nCurrentAct              = 0;
        g_nCurrentCustomerStartAct = 0;
        g_nCurrentCustomerEndAct   = 0;
        g_nCurrentCustomerIdx      = 0;
    }

    g_nGUIState = 0;
    memset( g_szInputBuf, 0, sizeof( TCHAR ) * 16 );
    g_szInputBuf[ 0 ]  = TEXT( '\0' );
    g_bWaitingForEnter = 0;
    g_bPaused          = 0;
    g_bCompleted       = 0;

    QueryPerformanceCounter( &g_liLastTime );

    return 1;
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int nShowCmd )
{
    static TCHAR szAppName[ 32 ] = { 0 }; /* app class name */
    HWND hwnd = NULL;               /* main window handle */
    MSG stMsg = { 0 };              /* window message */
    WNDCLASS stWc = { 0 };          /* window class */
    LARGE_INTEGER liNow = { 0 };    /* current time counter */
    double dElapsed = 0.0;          /* elapsed time */
    LONG nStyle = 0;                /* window style flags */

    _tcsncpy( szAppName, TEXT( "Dick Trainer" ), 31 );
    szAppName[ 31 ] = TEXT( '\0' );

    srand( ( unsigned int )time( NULL ) );

    g_nTransparency    = 255;
    g_nTransparencyPct = 0;
    g_nBeepVolume      = 50;
    _tcsncpy( g_szHorizontal, TEXT( "center" ), 19 );
    g_szHorizontal[ 19 ] = TEXT( '\0' );
    _tcsncpy( g_szVertical, TEXT( "bottom" ), 19 );
    g_szVertical[ 19 ] = TEXT( '\0' );

    if( strstr( szCmdLine, "-v" ) )
    {
        printf( "Dick Trainer v" VERSION_STRING "\n" );
        return 0;
    }

    if( strstr( szCmdLine, "-log" ) )
    {
        InitConsole();
        printf( "Dick Trainer v" VERSION_STRING "\n" );
        g_bLog = 1;
    }
    else
    {
        ShowWindow( GetConsoleWindow(), SW_HIDE );
    }

    if( !ResetAllState( ) )
    {
        return 0;
    }

    g_hHighBeepEvent = CreateEvent( NULL, 0, 0, NULL );
    g_hLowBeepEvent  = CreateEvent( NULL, 0, 0, NULL );
    g_hMidBeepEvent  = CreateEvent( NULL, 0, 0, NULL );

    CreateThread( NULL, 0, BeepHigh, NULL, 0, NULL );
    CreateThread( NULL, 0, BeepLow,  NULL, 0, NULL );
    CreateThread( NULL, 0, BeepMid,  NULL, 0, NULL );

    stWc.style         = CS_HREDRAW | CS_VREDRAW;
    stWc.lpfnWndProc   = WndProc;
    stWc.hInstance     = hInstance;
    stWc.hCursor       = LoadCursor( NULL, IDC_ARROW );
    stWc.lpszClassName = szAppName;
    RegisterClass( &stWc );

    QueryPerformanceFrequency( &g_liFrequency );
    QueryPerformanceCounter( &g_liStartTime );
    g_liLastTime = g_liStartTime;

    hwnd = CreateWindowEx( WS_EX_LAYERED, szAppName, TEXT( "Dick Trainer" ), WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, 900, 450, NULL, NULL, hInstance, NULL );
    ShowWindow( hwnd, nShowCmd );
    UpdateWindow( hwnd );

    SetLayeredWindowAttributes( hwnd, 0, ( BYTE )g_nTransparency, LWA_ALPHA );
    g_nTransparencyPct = ( int )( ( 255.0 - g_nTransparency ) / 255.0 * 100.0 + 0.5 );

    while( 1 )
    {
        if( PeekMessage( &stMsg, NULL, 0, 0, PM_REMOVE ) )
        {
            if( stMsg.message == WM_QUIT )
            {
                break;
            }
            TranslateMessage( &stMsg );
            DispatchMessage( &stMsg );
        }
        else
        {
            /* Auto hide/show title bar (2 seconds) */
            QueryPerformanceCounter( &liNow );
            dElapsed = ( g_liLastInputTime.QuadPart == 0 ) ? 10.0
                : ( double )( liNow.QuadPart - g_liLastInputTime.QuadPart ) / ( double )g_liFrequency.QuadPart;
            nStyle = GetWindowLong( hwnd, GWL_STYLE );
            if( dElapsed < 2.0 )
            {
                if( !( nStyle & WS_CAPTION ) )
                {
                    SetWindowLong( hwnd, GWL_STYLE, nStyle | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX );
                    SetWindowPos( hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED );
                }
            }
            else
            {
                if( nStyle & WS_CAPTION )
                {
                    SetWindowLong( hwnd, GWL_STYLE, nStyle & ~( WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX ) );
                    SetWindowPos( hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED );
                }
            }
            InvalidateRect( hwnd, NULL, 0 );
            Sleep( 1 );
        }
    }

    /* Cleanup memory */
    FreeAllData();
    CloseHandle( g_hHighBeepEvent );
    CloseHandle( g_hLowBeepEvent );
    CloseHandle( g_hMidBeepEvent );

    return ( int )( stMsg.wParam );
}

LRESULT CALLBACK WndProc( HWND hwnd, UINT uMsg, WPARAM wp, LPARAM lp )
{
    /* WM_KEYDOWN */
    int nInputLen = 0;              /* input buffer length */
    int nCustomerCount = 0;         /* customer count from input */
    int bWasGraphStarted = 0;       /* previous graph started state */

    /* WM_PAINT */
    PAINTSTRUCT stPs = { 0 };       /* paint struct */
    HDC hdcWindow = NULL;           /* window DC */
    HDC hdcMemory = NULL;           /* memory DC */
    RECT rcClient = { 0 };          /* client rect */
    int nWidth = 0;                 /* client width */
    int nHeight = 0;                /* client height */
    HBITMAP hBitmap = NULL;         /* back buffer bitmap */
    HBRUSH hBackgroundBrush = NULL; /* background brush */
    LARGE_INTEGER liNow = { 0 };    /* current time */
    double dElapsed = 0.0;          /* elapsed time */
    TCHAR szTransparencyLabel[ 16 ] = { 0 }; /* transparency label */
    TCHAR szTransparencyValue[ 16 ] = { 0 }; /* transparency value text */
    HFONT hFontLabel = NULL;        /* label font */
    HFONT hFontValue = NULL;        /* value font */
    SIZE szLabelExtent = { 0 };     /* label text extent */
    SIZE szValueExtent = { 0 };     /* value text extent */
    int nCenterX = 0;               /* overlay center X */
    int nBaseY = 0;                 /* overlay base Y */
    TCHAR szVolumeLabel[ 16 ] = { 0 }; /* volume label */
    TCHAR szVolumeValue[ 16 ] = { 0 }; /* volume value text */

    _tcsncpy( szTransparencyLabel, TEXT( "TRANSPARENT" ), 15 );
    szTransparencyLabel[ 15 ] = TEXT( '\0' );
    _tcsncpy( szVolumeLabel, TEXT( "VOLUME" ), 15 );
    szVolumeLabel[ 15 ] = TEXT( '\0' );

    switch( uMsg )
    {
        case WM_ERASEBKGND:
        {
            return 1;
        }
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        {
            QueryPerformanceCounter( &g_liLastInputTime );
            return DefWindowProc( hwnd, uMsg, wp, lp );
        }
        case WM_KEYDOWN:
        {
            QueryPerformanceCounter( &g_liLastInputTime );

            if( g_nGUIState == 0 )
            {
                if( _tcscmp( g_szMode, TEXT( "sequence" ) ) != 0 && wp >= '0' && wp <= '9' )
                {
                    nInputLen = ( int )_tcslen( g_szInputBuf );
                    if( nInputLen < 5 )
                    {
                        g_szInputBuf[ nInputLen ]     = ( TCHAR )( wp );
                        g_szInputBuf[ nInputLen + 1 ] = TEXT( '\0' );
                    }
                }
                else if( _tcscmp( g_szMode, TEXT( "sequence" ) ) != 0 && wp >= VK_NUMPAD0 && wp <= VK_NUMPAD9 )
                {
                    nInputLen = ( int )_tcslen( g_szInputBuf );
                    if( nInputLen < 5 )
                    {
                        g_szInputBuf[ nInputLen ]     = ( TCHAR )( wp - VK_NUMPAD0 + '0' );
                        g_szInputBuf[ nInputLen + 1 ] = TEXT( '\0' );
                    }
                }
                else if( wp == VK_BACK )
                {
                    nInputLen = ( int )_tcslen( g_szInputBuf );
                    if( nInputLen > 0 )
                    {
                        g_szInputBuf[ nInputLen - 1 ] = TEXT( '\0' );
                    }
                }
                else if( wp == VK_RETURN )
                {
                    if( _tcscmp( g_szMode, TEXT( "sequence" ) ) == 0 )
                    {
                        /* Sequence mode: sequences already built by LoadAllDataFromJson */
                        g_nGUIState        = 1;
                        g_bGraphStarted    = 0;
                        g_bWaitingForEnter = 1;
                        g_bFirstBeepPlayed = 0;
                        QueryPerformanceCounter( &g_liLastTime );
                    }
                    else
                    {
                        nCustomerCount = _ttoi( g_szInputBuf );
                        if( nCustomerCount > 0 )
                        {
                            GenerateSequences( nCustomerCount );
                            g_nGUIState        = 1;
                            g_bGraphStarted    = 0;
                            g_bWaitingForEnter = 1;
                            g_bFirstBeepPlayed = 0;
                            QueryPerformanceCounter( &g_liLastTime );
                        }
                    }
                }
            }
            else if( wp == VK_RETURN && g_bWaitingForEnter )
            {
                bWasGraphStarted   = g_bGraphStarted;
                g_bGraphStarted    = 1;
                g_bPaused          = 0;
                if( bWasGraphStarted )
                {
                    SetupNextCustomer();
                }
                g_bWaitingForEnter = 0;
                QueryPerformanceCounter( &g_liLastTime );
            }
            else if( wp == VK_RETURN && g_bCompleted )
            {
                ResetAllState();
            }
            else if( wp == VK_SPACE && g_bGraphStarted && !g_bWaitingForEnter )
            {
                g_bPaused = !g_bPaused;
                QueryPerformanceCounter( &g_liLastTime );
            }

            /* Volume control */
            if( wp == VK_HOME )
            {
                g_nBeepVolume += 5;
                if( g_nBeepVolume > 100 )
                {
                    g_nBeepVolume = 100;
                }
                QueryPerformanceCounter( &g_liVolumeChangeTime );
            }
            else if( wp == VK_END )
            {
                g_nBeepVolume -= 5;
                if( g_nBeepVolume < 0 )
                {
                    g_nBeepVolume = 0;
                }
                QueryPerformanceCounter( &g_liVolumeChangeTime );
            }

            /* Transparency control */
            if( wp == VK_PRIOR )
            {
                g_nTransparencyPct += 5;
                if( g_nTransparencyPct > 100 )
                {
                    g_nTransparencyPct = 100;
                }
                g_nTransparency = ( int )( ( 100.0 - g_nTransparencyPct ) / 100.0 * 255.0 + 0.5 );
                SetLayeredWindowAttributes( hwnd, 0, ( BYTE )g_nTransparency, LWA_ALPHA );
                QueryPerformanceCounter( &g_liTransparencyChangeTime );
            }
            else if( wp == VK_NEXT )
            {
                g_nTransparencyPct -= 5;
                if( g_nTransparencyPct < 0 )
                {
                    g_nTransparencyPct = 0;
                }
                g_nTransparency = ( int )( ( 100.0 - g_nTransparencyPct ) / 100.0 * 255.0 + 0.5 );
                SetLayeredWindowAttributes( hwnd, 0, ( BYTE )g_nTransparency, LWA_ALPHA );
                QueryPerformanceCounter( &g_liTransparencyChangeTime );
            }

            /* Vertical flip */
            if( wp == VK_DOWN )
            {
                _tcsncpy( g_szVertical, TEXT( "bottom" ), 19 );
                g_szVertical[ 19 ] = TEXT( '\0' );
            }
            else if( wp == VK_UP )
            {
                _tcsncpy( g_szVertical, TEXT( "top" ), 19 );
                g_szVertical[ 19 ] = TEXT( '\0' );
            }

            /* Dick position control */
            if( wp == VK_LEFT )
            {
                if( _tcscmp( g_szHorizontal, TEXT( "right" ) ) == 0 )
                {
                    _tcsncpy( g_szHorizontal, TEXT( "center" ), 19 );
                    g_szHorizontal[ 19 ] = TEXT( '\0' );
                }
                else if( _tcscmp( g_szHorizontal, TEXT( "center" ) ) == 0 )
                {
                    _tcsncpy( g_szHorizontal, TEXT( "left" ), 19 );
                    g_szHorizontal[ 19 ] = TEXT( '\0' );
                }
            }
            else if( wp == VK_RIGHT )
            {
                if( _tcscmp( g_szHorizontal, TEXT( "left" ) ) == 0 )
                {
                    _tcsncpy( g_szHorizontal, TEXT( "center" ), 19 );
                    g_szHorizontal[ 19 ] = TEXT( '\0' );
                }
                else if( _tcscmp( g_szHorizontal, TEXT( "center" ) ) == 0 )
                {
                    _tcsncpy( g_szHorizontal, TEXT( "right" ), 19 );
                    g_szHorizontal[ 19 ] = TEXT( '\0' );
                }
            }

            return 0;
        }
        case WM_PAINT:
        {
            hdcWindow = BeginPaint( hwnd, &stPs );
            hdcMemory = CreateCompatibleDC( hdcWindow );

            GetClientRect( hwnd, &rcClient );
            nWidth  = rcClient.right  - rcClient.left;
            nHeight = rcClient.bottom - rcClient.top;

            hBitmap = CreateCompatibleBitmap( hdcWindow, nWidth, nHeight );
            SelectObject( hdcMemory, hBitmap );

            hBackgroundBrush = CreateSolidBrush( RGB( 20, 25, 30 ) );
            FillRect( hdcMemory, &rcClient, hBackgroundBrush );
            DeleteObject( hBackgroundBrush );

            if( g_nGUIState == 0 )
            {
                RenderInputScreen( hdcMemory, nWidth, nHeight );
            }
            else if( g_bCompleted )
            {
                RenderCompletionScreen( hdcMemory, nWidth, nHeight );
            }
            else if( g_bWaitingForEnter && g_nActCount > 0 )
            {
                RenderWaitingScreen( hdcMemory, nWidth, nHeight );
            }
            else if( g_nActCount > 0 && g_bGraphStarted )
            {
                RenderGraph( hdcMemory, nWidth, nHeight );
            }

            /* Transparency display overlay */
            QueryPerformanceCounter( &liNow );
            dElapsed = ( double )( liNow.QuadPart - g_liTransparencyChangeTime.QuadPart ) / ( double )g_liFrequency.QuadPart;
            if( g_liTransparencyChangeTime.QuadPart != 0 && dElapsed < 1.0 )
            {
                _sntprintf( szTransparencyValue, 16, TEXT( "%d%%" ), g_nTransparencyPct );

                hFontLabel = CreateFont( 28, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, TEXT( "Consolas" ) );
                hFontValue = CreateFont( 48, 0, 0, 0, FW_BOLD,   0, 0, 0, 0, 0, 0, 0, 0, TEXT( "Consolas" ) );
                SetBkMode( hdcMemory, TRANSPARENT );

                SelectObject( hdcMemory, hFontLabel );
                GetTextExtentPoint32( hdcMemory, szTransparencyLabel, ( int )_tcslen( szTransparencyLabel ), &szLabelExtent );
                SelectObject( hdcMemory, hFontValue );
                GetTextExtentPoint32( hdcMemory, szTransparencyValue, ( int )_tcslen( szTransparencyValue ), &szValueExtent );

                nCenterX = nWidth / 2;
                nBaseY   = nHeight / 2 - ( szLabelExtent.cy + 6 + szValueExtent.cy ) / 2 - 60;

                SelectObject( hdcMemory, hFontLabel );
                SetTextColor( hdcMemory, RGB( 100, 140, 180 ) );
                TextOut( hdcMemory, nCenterX - szLabelExtent.cx / 2, nBaseY, szTransparencyLabel, ( int )_tcslen( szTransparencyLabel ) );

                SelectObject( hdcMemory, hFontValue );
                SetTextColor( hdcMemory, RGB( 180, 220, 255 ) );
                TextOut( hdcMemory, nCenterX - szValueExtent.cx / 2, nBaseY + szLabelExtent.cy + 6, szTransparencyValue, ( int )_tcslen( szTransparencyValue ) );

                DeleteObject( hFontLabel );
                DeleteObject( hFontValue );
            }

            /* Volume display overlay */
            QueryPerformanceCounter( &liNow );
            dElapsed = ( double )( liNow.QuadPart - g_liVolumeChangeTime.QuadPart ) / ( double )g_liFrequency.QuadPart;
            if( g_liVolumeChangeTime.QuadPart != 0 && dElapsed < 1.0 )
            {
                _sntprintf( szVolumeValue, 16, TEXT( "%d%%" ), g_nBeepVolume );

                hFontLabel = CreateFont( 28, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, TEXT( "Consolas" ) );
                hFontValue = CreateFont( 48, 0, 0, 0, FW_BOLD,   0, 0, 0, 0, 0, 0, 0, 0, TEXT( "Consolas" ) );
                SetBkMode( hdcMemory, TRANSPARENT );

                SelectObject( hdcMemory, hFontLabel );
                GetTextExtentPoint32( hdcMemory, szVolumeLabel, ( int )_tcslen( szVolumeLabel ), &szLabelExtent );
                SelectObject( hdcMemory, hFontValue );
                GetTextExtentPoint32( hdcMemory, szVolumeValue, ( int )_tcslen( szVolumeValue ), &szValueExtent );

                nCenterX = nWidth / 2;
                nBaseY   = nHeight / 2 - ( szLabelExtent.cy + 6 + szValueExtent.cy ) / 2 + 20;

                SelectObject( hdcMemory, hFontLabel );
                SetTextColor( hdcMemory, RGB( 100, 140, 180 ) );
                TextOut( hdcMemory, nCenterX - szLabelExtent.cx / 2, nBaseY, szVolumeLabel, ( int )_tcslen( szVolumeLabel ) );

                SelectObject( hdcMemory, hFontValue );
                SetTextColor( hdcMemory, RGB( 180, 220, 255 ) );
                TextOut( hdcMemory, nCenterX - szValueExtent.cx / 2, nBaseY + szLabelExtent.cy + 6, szVolumeValue, ( int )_tcslen( szVolumeValue ) );

                DeleteObject( hFontLabel );
                DeleteObject( hFontValue );
            }

            BitBlt( hdcWindow, 0, 0, nWidth, nHeight, hdcMemory, 0, 0, SRCCOPY );
            DeleteObject( hBitmap );
            DeleteDC( hdcMemory );
            EndPaint( hwnd, &stPs );

            return 0;
        }
        case WM_DESTROY:
        {
            PostQuitMessage( 0 );
            return 0;
        }
    }
    return DefWindowProc( hwnd, uMsg, wp, lp );
}

