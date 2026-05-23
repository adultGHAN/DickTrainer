#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "types.h"
#include "utils.h"
#include "logic.h"
#include "render.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


void RenderInputScreen( HDC hdcMemory, int nWidth, int nHeight )
{
    HFONT hFont = NULL;             /* display font */
    TCHAR szDisplayBuf[ 32 ] = { 0 }; /* formatted display string */

    SetTextColor( hdcMemory, RGB( 180, 220, 255 ) );
    SetBkMode( hdcMemory, TRANSPARENT );

    hFont = CreateFont( 32, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, TEXT( "Consolas" ) );
    SelectObject( hdcMemory, hFont );
    TextOut( hdcMemory, nWidth / 2 - 150, nHeight / 2 - 40, TEXT( "ENTER CUSTOMER COUNT" ), 20 );

    _sntprintf( szDisplayBuf, 32, TEXT( "[ %s_ ]" ), g_szInputBuf );
    TextOut( hdcMemory, nWidth / 2 - 50, nHeight / 2 + 10, szDisplayBuf, ( int )_tcslen( szDisplayBuf ) );

    DeleteObject( hFont );
}

void RenderWaitingScreen( HDC hdcMemory, int nWidth, int nHeight )
{
    SequenceAct *pCurrentAct = NULL; /* current sequence action */
    HFONT hFont = NULL;             /* display font */
    SIZE stTextExtent1 = { 0 };     /* text extent for line 1 */
    SIZE stTextExtent2 = { 0 };     /* text extent for line 2 */
    SIZE stTextExtent3 = { 0 };     /* text extent for line 3 */
    int nTotalHeight = 0;           /* total block height */
    int nStartY = 0;                /* block start Y */
    TCHAR szTextBuffer[ 128 ] = { 0 }; /* formatted text buffer */

    pCurrentAct = &g_pstSequence[ g_nCurrentAct ];

    SetTextColor( hdcMemory, RGB( 255, 255, 100 ) );
    SetBkMode( hdcMemory, TRANSPARENT );

    hFont = CreateFont( 40, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, TEXT( "Consolas" ) );
    SelectObject( hdcMemory, hFont );

    /* Measure text sizes - verify customerIdx */
    _sntprintf( szTextBuffer, 128, TEXT( "Customer %d" ), g_pstSequence[ g_nCurrentAct ].nCustomerIdx );
    GetTextExtentPoint32( hdcMemory, szTextBuffer, ( int )_tcslen( szTextBuffer ), &stTextExtent1 );

    _sntprintf( szTextBuffer, 128, TEXT( "Shape: [ %s ]" ), pCurrentAct->szShape );
    GetTextExtentPoint32( hdcMemory, szTextBuffer, ( int )_tcslen( szTextBuffer ), &stTextExtent2 );

    GetTextExtentPoint32( hdcMemory, TEXT( "Press [ENTER] to Start" ), 22, &stTextExtent3 );

    /* Calculate total height (line spacing 40px) */
    nTotalHeight = stTextExtent1.cy + stTextExtent2.cy + stTextExtent3.cy + 80;
    nStartY = nHeight / 2 - nTotalHeight / 2;

    /* First line */
    _sntprintf( szTextBuffer, 128, TEXT( "Customer %d" ), pCurrentAct->nCustomerIdx );
    TextOut( hdcMemory, nWidth / 2 - stTextExtent1.cx / 2, nStartY, szTextBuffer, ( int )_tcslen( szTextBuffer ) );

    /* Second line */
    _sntprintf( szTextBuffer, 128, TEXT( "Shape: [ %s ]" ), g_pstSequence[ g_nCurrentAct ].szShape );
    TextOut( hdcMemory, nWidth / 2 - stTextExtent2.cx / 2, nStartY + stTextExtent1.cy + 40, szTextBuffer, ( int )_tcslen( szTextBuffer ) );

    /* Third line */
    TextOut( hdcMemory, nWidth / 2 - stTextExtent3.cx / 2, nStartY + stTextExtent1.cy + stTextExtent2.cy + 80, TEXT( "Press [ENTER] to Start" ), 22 );

    DeleteObject( hFont );
}

void RenderCompletionScreen( HDC hdcMemory, int nWidth, int nHeight )
{
    HFONT hFontTitle = NULL;        /* title font */
    HFONT hFontSubtitle = NULL;     /* subtitle font */
    HFONT hFontRestartMsg = NULL;   /* restart message font */
    SIZE stTitleExtent = { 0 };     /* title text extent */
    SIZE stSubtitleExtent = { 0 };  /* subtitle text extent */
    SIZE stRestartExtent = { 0 };   /* restart text extent */
    int nStartY = 0;                /* block start Y */
    TCHAR szTitleText[ 32 ] = { 0 }; /* title string */
    TCHAR szSubtitleText[ 128 ] = { 0 }; /* subtitle string */
    TCHAR szRestartText[ 64 ] = { 0 }; /* restart string */

    _tcsncpy( szTitleText, TEXT( "Service Completed!" ), 31 );
    szTitleText[ 31 ] = TEXT( '\0' );
    _tcsncpy( szSubtitleText, TEXT( "You have served all customers. Thank you for your hard work." ), 127 );
    szSubtitleText[ 127 ] = TEXT( '\0' );
    _tcsncpy( szRestartText, TEXT( "Press [ENTER] to play again" ), 63 );
    szRestartText[ 63 ] = TEXT( '\0' );

    SetTextColor( hdcMemory, RGB( 100, 255, 100 ) );
    SetBkMode( hdcMemory, TRANSPARENT );

    /* Title font */
    hFontTitle = CreateFont( 60, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, TEXT( "Consolas" ) );
    SelectObject( hdcMemory, hFontTitle );
    GetTextExtentPoint32( hdcMemory, szTitleText, ( int )_tcslen( szTitleText ), &stTitleExtent );

    /* Subtitle font */
    hFontSubtitle = CreateFont( 28, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, TEXT( "Consolas" ) );

    /* Restart message font */
    hFontRestartMsg = CreateFont( 24, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, TEXT( "Consolas" ) );

    /* Calculate Start Y */
    nStartY = nHeight / 2 - ( stTitleExtent.cy + 40 ) / 2;

    /* Display title */
    SelectObject( hdcMemory, hFontTitle );
    TextOut( hdcMemory, nWidth / 2 - stTitleExtent.cx / 2, nStartY, szTitleText, ( int )_tcslen( szTitleText ) );

    /* Display subtitle */
    SelectObject( hdcMemory, hFontSubtitle );
    SetTextColor( hdcMemory, RGB( 180, 220, 255 ) );
    GetTextExtentPoint32( hdcMemory, szSubtitleText, ( int )_tcslen( szSubtitleText ), &stSubtitleExtent );
    TextOut( hdcMemory, nWidth / 2 - stSubtitleExtent.cx / 2, nStartY + stTitleExtent.cy + 40, szSubtitleText, ( int )_tcslen( szSubtitleText ) );

    /* Display restart message */
    SelectObject( hdcMemory, hFontRestartMsg );
    SetTextColor( hdcMemory, RGB( 255, 200, 100 ) );
    GetTextExtentPoint32( hdcMemory, szRestartText, ( int )_tcslen( szRestartText ), &stRestartExtent );
    TextOut( hdcMemory, nWidth / 2 - stRestartExtent.cx / 2, nStartY + stTitleExtent.cy + stSubtitleExtent.cy + 80, szRestartText, ( int )_tcslen( szRestartText ) );

    DeleteObject( hFontTitle );
    DeleteObject( hFontSubtitle );
    DeleteObject( hFontRestartMsg );
}

void RenderGraph( HDC hdcMemory, int nWidth, int nHeight )
{
    /* ===== Layout settings ===== */
    int nInfoHeight = 0;
    int nInfoHeightActual = 0;
    int nTopMargin = 0;
    int nGraphTopMargin = 0;
    int nBottomMargin = 0;
    int nTopY = 0;
    int nBottomY = 0;
    int nPillarX = 0;
    double dPixelsPerSecond = 0.0;
    HBRUSH hInfoPanelBrush = NULL;
    RECT rcTopInfo = { 0 };

    /* ===== Grid lines ===== */
    int nGridIndex = 0;
    int nGridY = 0;
    HPEN hGridPen = NULL;

    /* ===== Time calculation ===== */
    LARGE_INTEGER liCurrentTime = { 0 };
    double dDeltaTime = 0.0;
    double dActionDuration = 0.0;
    double dAbsoluteTime = 0.0;
    int nBeepType = 0;

    /* ===== Vertical grid lines ===== */
    HPEN hVerticalGridPen = NULL;
    double dTimeStep = 0.0;
    double dNextSecond = 0.0;
    int nVerticalX = 0;

    /* ===== Colors and zones ===== */
    COLORREF acrColors[ 5 ] = { 0 }; /* zone colors */
    int anYPos[ 6 ] = { 0 };        /* Y positions per grid line */
    int nBoundary01 = 0;
    int nBoundary12 = 0;
    int nBoundary23 = 0;
    int nBoundary34 = 0;
    HBRUSH hZoneBrush = NULL;
    int nZoneBottom = 0;
    int nZoneTop = 0;
    HRGN hClipRegion = NULL;
    int nHeadHalfHeight = 0;
    int nGraphHeight = 0;
    int nTopThickness = 0;
    int nDickHalfOuter = 0;         /* dick outer half-width (proportional to nWidth) */
    int nDickHalfShaft = 0;         /* dick shaft half-width (proportional to nWidth) */
    int nDickHalfHead = 0;          /* dick head half-width (proportional to nWidth) */

    /* ===== Graph path drawing ===== */
    int nPrevX = 0;
    int nPrevY = 0;
    int nPixelX = 0;
    double dCurrentTime = 0.0;
    double dYValue = 0.0;
    int nZone = 0;
    HPEN hPathPen = NULL;

    /* ===== Marker display ===== */
    HPEN hMarkerPen = NULL;
    double dMarkerTime = 0.0;
    double dTimeOffset = 0.0;
    int nMarkerX = 0;
    int nMarkerY = 0;
    HBRUSH hCurrentPosBrush = NULL;

    /* ===== Info text display ===== */
    HFONT hFontInfo = NULL;         /* info panel font */
    SequenceAct *pCurrentAct = NULL; /* current sequence action */
    int nFontSize = 0;              /* font size */
    int nStartY = 0;                /* info text start Y */
    int nLineGap = 0;               /* line gap between rows */
    int nColumnSpacing = 0;         /* column spacing */
    int nLine1StartX = 0;           /* first column X */
    TCHAR szTextBuffer[ 128 ] = { 0 }; /* formatted text buffer */

    /* ===== Flip / current-pos / pause ===== */
    XFORM stXf = { 0 };             /* world transform */
    int nDist = 0;                  /* pixel distance from pillar */
    HPEN hWhitePen = NULL;          /* white pen for current pos */
    HFONT hFontPause = NULL;        /* pause message font */
    SIZE stPauseExtent = { 0 };     /* pause text extent */

    /* Layout proportional to window size */
    nInfoHeight = nHeight / 3;
    nInfoHeightActual = max( 120, nInfoHeight );
    nTopMargin = ( int )( nHeight * 0.05 );
    nGraphTopMargin = ( int )( nHeight * 0.05 );
    nBottomMargin = ( int )( nHeight * 0.05 );

    /* Calculate graph area */
    nTopY = nInfoHeightActual + nTopMargin + nGraphTopMargin;
    nBottomY = nHeight - nBottomMargin;

    /* Set dick position based on dick_pos */
    if( _tcscmp( g_szDickPos, TEXT( "left" ) ) == 0 )
    {
        nPillarX = nWidth / 6;
    }
    else if( _tcscmp( g_szDickPos, TEXT( "right" ) ) == 0 )
    {
        nPillarX = nWidth * 5 / 6;
    }
    else
    {
        nPillarX = nWidth / 2;
    }
    dPixelsPerSecond = 150.0;

    /* Top info panel */
    rcTopInfo.left   = 0;
    rcTopInfo.top    = 0;
    rcTopInfo.right  = nWidth;
    rcTopInfo.bottom = nInfoHeightActual;

    hInfoPanelBrush = CreateSolidBrush( RGB( 25, 30, 35 ) );
    FillRect( hdcMemory, &rcTopInfo, hInfoPanelBrush );
    DeleteObject( hInfoPanelBrush );

    /* Draw horizontal grid lines */
    if( _tcscmp( g_szVertical, TEXT( "top" ) ) == 0 )
    {
        SetGraphicsMode( hdcMemory, GM_ADVANCED );
        stXf.eM11 = 1.0f; stXf.eM12 = 0.0f;
        stXf.eM21 = 0.0f; stXf.eM22 = -1.0f;
        stXf.eDx  = 0.0f; stXf.eDy  = ( float )( nTopY + nBottomY );
        SetWorldTransform( hdcMemory, &stXf );
    }
    for( nGridIndex = 1; nGridIndex <= 5; nGridIndex++ )
    {
        nGridY = GetYFromPos( nGridIndex, nTopY, nBottomY );
        hGridPen = CreatePen( PS_SOLID, 1, RGB( 40, 50, 60 ) );
        SelectObject( hdcMemory, hGridPen );
        MoveToEx( hdcMemory, 0, nGridY, 0 );
        LineTo( hdcMemory, nWidth, nGridY );
        DeleteObject( hGridPen );
    }

    /* Calculate current time */
    QueryPerformanceCounter( &liCurrentTime );
    dDeltaTime = ( double )( liCurrentTime.QuadPart - g_liLastTime.QuadPart ) / g_liFrequency.QuadPart;
    g_liLastTime = liCurrentTime;

    if( g_bWaitingForEnter || g_bPaused )
    {
        dDeltaTime = 0;
    }

    /* Play first beep */
    if( !g_bFirstBeepPlayed && !g_bWaitingForEnter )
    {
        SetEvent( g_hLowBeepEvent );
        g_bFirstBeepPlayed = 1;
    }

    /* Update actions */
    dActionDuration = g_pstSequence[ g_nCurrentAct ].dMovetime + g_pstSequence[ g_nCurrentAct ].dWaittime;
    g_dCurrentTimeInAct += dDeltaTime;

    while( g_dCurrentTimeInAct >= dActionDuration && g_nCurrentAct < g_nCurrentCustomerEndAct - 1 )
    {
        g_dCurrentTimeInAct -= dActionDuration;
        g_nCurrentAct++;
        dActionDuration = g_pstSequence[ g_nCurrentAct ].dMovetime + g_pstSequence[ g_nCurrentAct ].dWaittime;

        nBeepType = g_pstSequence[ g_nCurrentAct ].nBeepType;
        if( nBeepType == 1 )
        {
            SetEvent( g_hHighBeepEvent );
        }
        else if( nBeepType == 2 )
        {
            SetEvent( g_hLowBeepEvent );
        }
        else if( nBeepType == 3 )
        {
            SetEvent( g_hMidBeepEvent );
        }
    }

    /* Check if last action for current customer has been reached */
    if( g_nCurrentAct >= g_nCurrentCustomerEndAct - 1 )
    {
        dActionDuration = g_pstSequence[ g_nCurrentAct ].dMovetime + g_pstSequence[ g_nCurrentAct ].dWaittime;
        if( g_dCurrentTimeInAct >= dActionDuration )
        {
            /* Check if this is the last customer */
            if( g_nCurrentCustomerEndAct >= g_nActCount )
            {
                g_bCompleted = 1;
            }
            else
            {
                g_bWaitingForEnter = 1;
                g_nCurrentAct++;
            }
        }
    }

    /* Calculate absolute time */
    dAbsoluteTime = g_pstSequence[ g_nCurrentAct ].dStartTime + g_dCurrentTimeInAct;

    /* Draw time grid */
    hVerticalGridPen = CreatePen( PS_SOLID, 1, RGB( 40, 50, 60 ) );
    SelectObject( hdcMemory, hVerticalGridPen );

    dNextSecond = ceil( dAbsoluteTime );

    if( _tcscmp( g_szDickPos, TEXT( "left" ) ) == 0 )
    {
        for( dTimeStep = dNextSecond; dTimeStep <= dAbsoluteTime + nWidth / dPixelsPerSecond; dTimeStep += 1.0 )
        {
            nVerticalX = 0 + ( int )( ( dTimeStep - dAbsoluteTime ) * dPixelsPerSecond );
            if( nVerticalX >= nWidth )
            {
                break;
            }
            MoveToEx( hdcMemory, nVerticalX, nTopY, 0 );
            LineTo( hdcMemory, nVerticalX, nBottomY );
        }
    }
    else if( _tcscmp( g_szDickPos, TEXT( "right" ) ) == 0 )
    {
        for( dTimeStep = dNextSecond; dTimeStep <= dAbsoluteTime + nWidth / dPixelsPerSecond; dTimeStep += 1.0 )
        {
            nVerticalX = nWidth - ( int )( ( dTimeStep - dAbsoluteTime ) * dPixelsPerSecond );
            if( nVerticalX < 0 )
            {
                break;
            }
            MoveToEx( hdcMemory, nVerticalX, nTopY, 0 );
            LineTo( hdcMemory, nVerticalX, nBottomY );
        }
    }
    else
    {
        for( dTimeStep = dNextSecond; dTimeStep <= dAbsoluteTime + nPillarX / dPixelsPerSecond; dTimeStep += 1.0 )
        {
            nVerticalX = nPillarX + ( int )( ( dTimeStep - dAbsoluteTime ) * dPixelsPerSecond );
            if( nVerticalX >= nWidth )
            {
                break;
            }
            MoveToEx( hdcMemory, nVerticalX, nTopY, 0 );
            LineTo( hdcMemory, nVerticalX, nBottomY );
        }

        for( dTimeStep = dNextSecond; dTimeStep <= dAbsoluteTime + nPillarX / dPixelsPerSecond; dTimeStep += 1.0 )
        {
            nVerticalX = nPillarX - ( int )( ( dTimeStep - dAbsoluteTime ) * dPixelsPerSecond );
            if( nVerticalX <= 0 )
            {
                break;
            }
            MoveToEx( hdcMemory, nVerticalX, nTopY, 0 );
            LineTo( hdcMemory, nVerticalX, nBottomY );
        }
    }
    DeleteObject( hVerticalGridPen );

    /* Set colors per zone */
    acrColors[ 0 ] = RGB( 230, 40, 40 );
    acrColors[ 1 ] = RGB( 255, 140, 0 );
    acrColors[ 2 ] = RGB( 255, 215, 0 );
    acrColors[ 3 ] = RGB( 50, 205, 50 );
    acrColors[ 4 ] = RGB( 30, 144, 255 );

    for( nGridIndex = 1; nGridIndex <= 5; nGridIndex++ )
    {
        anYPos[ nGridIndex ] = GetYFromPos( nGridIndex, nTopY, nBottomY );
    }

    nBoundary01 = ( anYPos[ 1 ] + anYPos[ 2 ] ) / 2;
    nBoundary12 = ( anYPos[ 2 ] + anYPos[ 3 ] ) / 2;
    nBoundary23 = ( anYPos[ 3 ] + anYPos[ 4 ] ) / 2;
    nBoundary34 = ( anYPos[ 4 ] + anYPos[ 5 ] ) / 2;

    SelectObject( hdcMemory, GetStockObject( NULL_PEN ) );

    nGraphHeight = nBottomY - nTopY;
    nTopThickness = nGraphHeight / 10;
    nDickHalfOuter = nWidth * 48 / 900;
    nDickHalfShaft = nWidth * 28 / 900;
    nDickHalfHead  = nWidth * 35 / 900;

    for( nGridIndex = 0; nGridIndex < 5; nGridIndex++ )
    {
        hZoneBrush = CreateSolidBrush( acrColors[ nGridIndex ] );
        SelectObject( hdcMemory, hZoneBrush );

        nZoneBottom = ( nGridIndex == 0 ) ? nHeight : ( anYPos[ nGridIndex ] + anYPos[ nGridIndex + 1 ] ) / 2;
        nZoneTop    = ( nGridIndex == 4 ) ? 0 : ( anYPos[ nGridIndex + 1 ] + anYPos[ nGridIndex + 2 ] ) / 2;

        if( nGridIndex == 0 )
        {
            nZoneBottom = nBottomY + 100;
        }
        if( nGridIndex == 4 )
        {
            nZoneTop = nTopY - 100;
        }

        /* Right color zone */
        hClipRegion = CreateRectRgn( nPillarX, max( 0, nZoneTop ), nWidth, min( nHeight, nZoneBottom ) );
        SelectClipRgn( hdcMemory, hClipRegion );

        RoundRect( hdcMemory, nPillarX - nDickHalfOuter, nTopY - nTopThickness, nPillarX + nDickHalfOuter, nTopY + nTopThickness, 25, 25 );
        RoundRect( hdcMemory, nPillarX - nDickHalfShaft, nTopY, nPillarX + nDickHalfShaft, anYPos[ 2 ] + 15, 15, 15 );

        nHeadHalfHeight = ( anYPos[ 1 ] - anYPos[ 2 ] ) / 2;
        Chord( hdcMemory, nPillarX - nDickHalfHead, anYPos[ 2 ] - nHeadHalfHeight, nPillarX + nDickHalfHead, anYPos[ 1 ], nPillarX - nDickHalfHead, anYPos[ 2 ], nPillarX + nDickHalfHead, anYPos[ 2 ] );

        DeleteObject( hClipRegion );

        /* Left color zone (mirrored) */
        hClipRegion = CreateRectRgn( 0, max( 0, nZoneTop ), nPillarX, min( nHeight, nZoneBottom ) );
        SelectClipRgn( hdcMemory, hClipRegion );

        RoundRect( hdcMemory, nPillarX - nDickHalfOuter, nTopY - nTopThickness, nPillarX + nDickHalfOuter, nTopY + nTopThickness, 25, 25 );
        RoundRect( hdcMemory, nPillarX - nDickHalfShaft, nTopY, nPillarX + nDickHalfShaft, anYPos[ 2 ] + 15, 15, 15 );

        nHeadHalfHeight = ( anYPos[ 1 ] - anYPos[ 2 ] ) / 2;
        Chord( hdcMemory, nPillarX - nDickHalfHead, anYPos[ 2 ] - nHeadHalfHeight, nPillarX + nDickHalfHead, anYPos[ 1 ], nPillarX - nDickHalfHead, anYPos[ 2 ], nPillarX + nDickHalfHead, anYPos[ 2 ] );

        DeleteObject( hZoneBrush );
        SelectClipRgn( hdcMemory, NULL );
        DeleteObject( hClipRegion );
    }

    /* Draw graph path over time */
    if( _tcscmp( g_szDickPos, TEXT( "left" ) ) == 0 )
    {
        for( nPixelX = 0; nPixelX < nWidth; nPixelX++ )
        {
            nDist = nPixelX - nPillarX;
            dCurrentTime = dAbsoluteTime + nDist / dPixelsPerSecond;
            dYValue = GetYAtTime( dCurrentTime, nTopY, nBottomY );
            if( dYValue >= nBoundary01 )
            {
                nZone = 0;
            }
            else if( dYValue >= nBoundary12 )
            {
                nZone = 1;
            }
            else if( dYValue >= nBoundary23 )
            {
                nZone = 2;
            }
            else if( dYValue >= nBoundary34 )
            {
                nZone = 3;
            }
            else
            {
                nZone = 4;
            }

            if( nPixelX == 0 )
            {
                nPrevX = nPixelX;
                nPrevY = ( int )( dYValue );
            }
            else
            {
                hPathPen = CreatePen( PS_SOLID, 4, acrColors[ nZone ] );
                SelectObject( hdcMemory, hPathPen );
                MoveToEx( hdcMemory, nPrevX, nPrevY, 0 );
                LineTo( hdcMemory, nPixelX, ( int )( dYValue ) );
                DeleteObject( hPathPen );

                nPrevX = nPixelX;
                nPrevY = ( int )( dYValue );
            }
        }
    }
    else if( _tcscmp( g_szDickPos, TEXT( "right" ) ) == 0 )
    {
        for( nPixelX = nWidth - 1; nPixelX >= 0; nPixelX-- )
        {
            nDist = nPillarX - nPixelX;
            dCurrentTime = dAbsoluteTime + nDist / dPixelsPerSecond;
            dYValue = GetYAtTime( dCurrentTime, nTopY, nBottomY );
            if( dYValue >= nBoundary01 )
            {
                nZone = 0;
            }
            else if( dYValue >= nBoundary12 )
            {
                nZone = 1;
            }
            else if( dYValue >= nBoundary23 )
            {
                nZone = 2;
            }
            else if( dYValue >= nBoundary34 )
            {
                nZone = 3;
            }
            else
            {
                nZone = 4;
            }

            if( nPixelX == nWidth - 1 )
            {
                nPrevX = nPixelX;
                nPrevY = ( int )( dYValue );
            }
            else
            {
                hPathPen = CreatePen( PS_SOLID, 4, acrColors[ nZone ] );
                SelectObject( hdcMemory, hPathPen );
                MoveToEx( hdcMemory, nPrevX, nPrevY, 0 );
                LineTo( hdcMemory, nPixelX, ( int )( dYValue ) );
                DeleteObject( hPathPen );

                nPrevX = nPixelX;
                nPrevY = ( int )( dYValue );
            }
        }
    }
    else
    {
        /* Draw left side */
        nPrevX = 0;
        nPrevY = ( int )( GetYAtTime( dAbsoluteTime + nPillarX / dPixelsPerSecond, nTopY, nBottomY ) );
        for( nPixelX = 1; nPixelX <= nPillarX; nPixelX++ )
        {
            dCurrentTime = dAbsoluteTime + ( nPillarX - nPixelX ) / dPixelsPerSecond;
            dYValue = GetYAtTime( dCurrentTime, nTopY, nBottomY );
            if( dYValue >= nBoundary01 )
            {
                nZone = 0;
            }
            else if( dYValue >= nBoundary12 )
            {
                nZone = 1;
            }
            else if( dYValue >= nBoundary23 )
            {
                nZone = 2;
            }
            else if( dYValue >= nBoundary34 )
            {
                nZone = 3;
            }
            else
            {
                nZone = 4;
            }

            hPathPen = CreatePen( PS_SOLID, 4, acrColors[ nZone ] );
            SelectObject( hdcMemory, hPathPen );
            MoveToEx( hdcMemory, nPrevX, nPrevY, 0 );
            LineTo( hdcMemory, nPixelX, ( int )( dYValue ) );
            DeleteObject( hPathPen );

            nPrevX = nPixelX;
            nPrevY = ( int )( dYValue );
        }

        /* Draw right side */
        nPrevX = nPillarX;
        nPrevY = ( int )( GetYAtTime( dAbsoluteTime, nTopY, nBottomY ) );
        for( nPixelX = nPillarX + 1; nPixelX < nWidth; nPixelX++ )
        {
            dCurrentTime = dAbsoluteTime + ( nPixelX - nPillarX ) / dPixelsPerSecond;
            dYValue = GetYAtTime( dCurrentTime, nTopY, nBottomY );
            if( dYValue >= nBoundary01 )
            {
                nZone = 0;
            }
            else if( dYValue >= nBoundary12 )
            {
                nZone = 1;
            }
            else if( dYValue >= nBoundary23 )
            {
                nZone = 2;
            }
            else if( dYValue >= nBoundary34 )
            {
                nZone = 3;
            }
            else
            {
                nZone = 4;
            }

            hPathPen = CreatePen( PS_SOLID, 4, acrColors[ nZone ] );
            SelectObject( hdcMemory, hPathPen );
            MoveToEx( hdcMemory, nPrevX, nPrevY, 0 );
            LineTo( hdcMemory, nPixelX, ( int )( dYValue ) );
            DeleteObject( hPathPen );

            nPrevX = nPixelX;
            nPrevY = ( int )( dYValue );
        }
    }

    /* Draw markers */
    hMarkerPen = CreatePen( PS_SOLID, 5, RGB( 255, 255, 255 ) );
    SelectObject( hdcMemory, hMarkerPen );
    SelectObject( hdcMemory, GetStockObject( NULL_BRUSH ) );

    if( _tcscmp( g_szDickPos, TEXT( "left" ) ) == 0 )
    {
        for( nGridIndex = g_nCurrentCustomerStartAct; nGridIndex < g_nCurrentCustomerEndAct; nGridIndex++ )
        {
            dMarkerTime = g_pstSequence[ nGridIndex ].dStartTime;
            dTimeOffset = dMarkerTime - dAbsoluteTime;

            if( dTimeOffset >= 0 && dTimeOffset < nWidth / dPixelsPerSecond )
            {
                nMarkerX = nPillarX + ( int )( dTimeOffset * dPixelsPerSecond );
                if( nMarkerX >= 0 && nMarkerX < nWidth )
                {
                    nMarkerY = ( int )GetYAtTime( dMarkerTime, nTopY, nBottomY );
                    Ellipse( hdcMemory, nMarkerX - 8, nMarkerY - 8, nMarkerX + 8, nMarkerY + 8 );
                }
            }
        }
    }
    else if( _tcscmp( g_szDickPos, TEXT( "right" ) ) == 0 )
    {
        for( nGridIndex = g_nCurrentCustomerStartAct; nGridIndex < g_nCurrentCustomerEndAct; nGridIndex++ )
        {
            dMarkerTime = g_pstSequence[ nGridIndex ].dStartTime;
            dTimeOffset = dMarkerTime - dAbsoluteTime;

            if( dTimeOffset >= 0 && dTimeOffset < nWidth / dPixelsPerSecond )
            {
                nMarkerX = nPillarX - ( int )( dTimeOffset * dPixelsPerSecond );
                if( nMarkerX >= 0 && nMarkerX < nWidth )
                {
                    nMarkerY = ( int )GetYAtTime( dMarkerTime, nTopY, nBottomY );
                    Ellipse( hdcMemory, nMarkerX - 8, nMarkerY - 8, nMarkerX + 8, nMarkerY + 8 );
                }
            }
        }
    }
    else
    {
        for( nGridIndex = g_nCurrentCustomerStartAct; nGridIndex < g_nCurrentCustomerEndAct; nGridIndex++ )
        {
            dMarkerTime = g_pstSequence[ nGridIndex ].dStartTime;
            dTimeOffset = dMarkerTime - dAbsoluteTime;

            if( dTimeOffset >= 0 )
            {
                if( dTimeOffset < ( nWidth - nPillarX ) / dPixelsPerSecond )
                {
                    nMarkerX = nPillarX + ( int )( dTimeOffset * dPixelsPerSecond );
                    nMarkerY = ( int )GetYAtTime( dMarkerTime, nTopY, nBottomY );
                    Ellipse( hdcMemory, nMarkerX - 8, nMarkerY - 8, nMarkerX + 8, nMarkerY + 8 );
                }

                if( dTimeOffset < nPillarX / dPixelsPerSecond )
                {
                    nMarkerX = nPillarX - ( int )( dTimeOffset * dPixelsPerSecond );
                    nMarkerY = ( int )GetYAtTime( dMarkerTime, nTopY, nBottomY );
                    Ellipse( hdcMemory, nMarkerX - 8, nMarkerY - 8, nMarkerX + 8, nMarkerY + 8 );
                }
            }
        }
    }
    DeleteObject( hMarkerPen );

    /* Display current position */
    hCurrentPosBrush = CreateSolidBrush( RGB( 180, 50, 255 ) );
    {
        hWhitePen = CreatePen( PS_SOLID, 5, RGB( 255, 255, 255 ) );
        SelectObject( hdcMemory, hCurrentPosBrush );
        SelectObject( hdcMemory, hWhitePen );
        Ellipse( hdcMemory, nPillarX - 10, ( int )( GetYAtTime( dAbsoluteTime, nTopY, nBottomY ) ) - 10, nPillarX + 10, ( int )( GetYAtTime( dAbsoluteTime, nTopY, nBottomY ) ) + 10 );
        DeleteObject( hWhitePen );
    }
    DeleteObject( hCurrentPosBrush );

    /* Display info text */
    if( _tcscmp( g_szVertical, TEXT( "top" ) ) == 0 )
    {
        stXf.eM11 = 1.0f; stXf.eM12 = 0.0f;
        stXf.eM21 = 0.0f; stXf.eM22 = 1.0f;
        stXf.eDx  = 0.0f; stXf.eDy  = 0.0f;
        SetWorldTransform( hdcMemory, &stXf );
        SetGraphicsMode( hdcMemory, GM_COMPATIBLE );
    }
    SetTextColor( hdcMemory, RGB( 180, 220, 255 ) );
    SetBkMode( hdcMemory, TRANSPARENT );

    nFontSize = max( 12, ( nInfoHeightActual / 6 ) );
    hFontInfo = CreateFont( nFontSize, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, TEXT( "Consolas" ) );

    pCurrentAct = &g_pstSequence[ g_nCurrentAct ];
    nStartY       = nInfoHeightActual / 8;
    nLineGap      = nInfoHeightActual / 2;
    nColumnSpacing = nWidth / 7;
    nLine1StartX  = ( int )( nColumnSpacing * 0.5 );

    SelectObject( hdcMemory, hFontInfo );
    TextOut( hdcMemory, nLine1StartX, nStartY, TEXT( "customer" ), 8 );
    _sntprintf( szTextBuffer, 128, TEXT( "%d / %d" ), pCurrentAct->nCustomerIdx, pCurrentAct->nTotalCustomers );
    TextOut( hdcMemory, nLine1StartX + nColumnSpacing, nStartY, szTextBuffer, ( int )_tcslen( szTextBuffer ) );

    TextOut( hdcMemory, nLine1StartX + nColumnSpacing * 2, nStartY, TEXT( "customer" ), 8 );
    TextOut( hdcMemory, nLine1StartX + nColumnSpacing * 2, ( int )( nStartY + nFontSize + 2 ), TEXT( "level" ), 5 );
    _sntprintf( szTextBuffer, 128, TEXT( "%.1f" ), pCurrentAct->dCustomerLevel );
    TextOut( hdcMemory, nLine1StartX + nColumnSpacing * 3, nStartY, szTextBuffer, ( int )_tcslen( szTextBuffer ) );

    TextOut( hdcMemory, nLine1StartX + nColumnSpacing * 4, nStartY, TEXT( "course" ), 6 );
    _sntprintf( szTextBuffer, 128, TEXT( "%d / %d" ), pCurrentAct->nCourseIdx, pCurrentAct->nTotalCourses );
    TextOut( hdcMemory, nLine1StartX + nColumnSpacing * 5, nStartY, szTextBuffer, ( int )_tcslen( szTextBuffer ) );

    /* Second line */
    TextOut( hdcMemory, nLine1StartX, ( int )( nStartY + nLineGap ), TEXT( "motion" ), 6 );
    TextOut( hdcMemory, nLine1StartX + nColumnSpacing, ( int )( nStartY + nLineGap ), pCurrentAct->szMotionName, ( int )_tcslen( pCurrentAct->szMotionName ) );

    TextOut( hdcMemory, nLine1StartX + nColumnSpacing * 2, ( int )( nStartY + nLineGap ), TEXT( "motion" ), 6 );
    TextOut( hdcMemory, nLine1StartX + nColumnSpacing * 2, ( int )( nStartY + nLineGap + nFontSize + 2 ), TEXT( "level" ), 5 );
    _sntprintf( szTextBuffer, 128, TEXT( "%.1f" ), pCurrentAct->dMotionLevel );
    TextOut( hdcMemory, nLine1StartX + nColumnSpacing * 3, ( int )( nStartY + nLineGap ), szTextBuffer, ( int )_tcslen( szTextBuffer ) );

    TextOut( hdcMemory, nLine1StartX + nColumnSpacing * 4, ( int )( nStartY + nLineGap ), TEXT( "reps" ), 4 );
    _sntprintf( szTextBuffer, 128, TEXT( "%d / %d" ), pCurrentAct->nCurrentRep, pCurrentAct->nTotalReps );
    TextOut( hdcMemory, nLine1StartX + nColumnSpacing * 5, ( int )( nStartY + nLineGap ), szTextBuffer, ( int )_tcslen( szTextBuffer ) );

    DeleteObject( hFontInfo );

    /* Display pause message */
    if( g_bPaused )
    {
        hFontPause = CreateFont( 80, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, TEXT( "Consolas" ) );
        SelectObject( hdcMemory, hFontPause );
        SetTextColor( hdcMemory, RGB( 255, 100, 100 ) );
        SetBkMode( hdcMemory, TRANSPARENT );

        GetTextExtentPoint32( hdcMemory, TEXT( "PAUSE" ), 5, &stPauseExtent );
        TextOut( hdcMemory, nWidth / 2 - stPauseExtent.cx / 2, nHeight / 2 - stPauseExtent.cy / 2, TEXT( "PAUSE" ), 5 );

        DeleteObject( hFontPause );
    }
}
