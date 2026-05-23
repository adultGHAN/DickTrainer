#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <tchar.h>
#include "types.h"
#include "cJSON/cJSON.h"
#include "utils.h"
#include "logic.h"

extern int g_bLog;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define LEVEL_EPSILON 1e-9

/* --- Pass 1: Calculate data sizes --- */
DataSizes CalculateDataSizes( const TCHAR *szFilename )
{
    /* pointers */
    FILE *hFp = NULL;
    char *pszData = NULL;           /* file data string */
    cJSON *pJson = NULL;            /* root JSON object */
    cJSON *pItem = NULL;            /* general iteration item */
    cJSON *pCData = NULL;           /* customer data array */
    cJSON *pSData = NULL;           /* size data array */
    cJSON *pMotionArr = NULL;       /* motion array */
    cJSON *pMotionItem = NULL;      /* motion iteration item */
    cJSON *pLevelArr = NULL;        /* level data array */
    cJSON *pLevelItem = NULL;       /* level iteration item */
    cJSON *pActs = NULL;            /* action array */
    cJSON *pCourseObj = NULL;       /* course JSON object */
    cJSON *pRepsObj = NULL;         /* reps JSON object */

    /* result */
    DataSizes stSizes = { 0 };

    /* counters */
    int nCourse = 0;
    int nReps = 0;                  /* reps count */
    int nTempActCount = 0;          /* temporary action count */
    int nLevelSequenceCount = 0;    /* level sequence count */
    int nMaxCustomerCourse = 0;     /* max course value among customers */
    int nCurrentMotionSequenceCount = 0; /* current motion sequence count */
    int nMaxMotionSequenceCount = 0; /* largest sequence count among motions */

    /* file size */
    long nLen = 0;

    /* Open file */
    hFp = _tfopen( szFilename, TEXT( "rb" ) );
    if( hFp == NULL )
    {
        return stSizes;
    }

    /* Check file size */
    fseek( hFp, 0, SEEK_END );
    nLen = ftell( hFp );
    fseek( hFp, 0, SEEK_SET );

    /* Allocate memory and read file */
    pszData = ( char * )malloc( ( size_t )( nLen + 1 ) );
    if( pszData == NULL )
    {
        fclose( hFp );
        return stSizes;
    }
    fread( pszData, 1, nLen, hFp );
    pszData[ nLen ] = '\0';
    fclose( hFp );

    /* Parse JSON */
    pJson = cJSON_Parse( pszData );
    free( pszData );
    if( !pJson )
    {
        return stSizes;
    }

    /* Customer count and max course value */
    pCData = cJSON_GetObjectItem( cJSON_GetObjectItem( pJson, "customer" ), "data" );
    if( pCData )
    {
        cJSON_ArrayForEach( pItem, pCData )
        {
            stSizes.nCustomerCount++;

            /* Track max course value */
            pCourseObj = cJSON_GetObjectItem( pItem, "course" );
            if( pCourseObj )
            {
                nCourse = pCourseObj->valueint;
                if( nCourse > nMaxCustomerCourse )
                {
                    nMaxCustomerCourse = nCourse;
                }
            }
        }
    }

    /* Size data count */
    pSData = cJSON_GetObjectItem( cJSON_GetObjectItem( pJson, "size" ), "data" );
    if( pSData )
    {
        cJSON_ArrayForEach( pItem, pSData )
        {
            stSizes.nSizeCount++;
        }
    }

    /* Motion count and max sequence size */
    pMotionArr = cJSON_GetObjectItem( pJson, "motion" );
    if( pMotionArr )
    {
        cJSON_ArrayForEach( pMotionItem, pMotionArr )
        {
            stSizes.nMotionCount++;

            /* Calculate largest sequence count per motion */
            nCurrentMotionSequenceCount = 0;
            pLevelArr = cJSON_GetObjectItem( pMotionItem, "data" );
            if( pLevelArr )
            {
                cJSON_ArrayForEach( pLevelItem, pLevelArr )
                {
                    /* Per level: preInterval(1) + actions(reps) + postInterval(1) */
                    nTempActCount = 0;
                    pActs = cJSON_GetObjectItem( pLevelItem, "act" );
                    if( pActs )
                    {
                        cJSON_ArrayForEach( pItem, pActs )
                        {
                            nTempActCount++;
                        }
                    }

                    pRepsObj = cJSON_GetObjectItem( pLevelItem, "reps" );
                    if( pRepsObj )
                    {
                        nReps = pRepsObj->valueint;
                        nLevelSequenceCount = 2 + ( nTempActCount * nReps ); /* pre + main(reps) + post */

                        /* Track max within this motion */
                        if( nLevelSequenceCount > nCurrentMotionSequenceCount )
                        {
                            nCurrentMotionSequenceCount = nLevelSequenceCount;
                        }
                    }
                }
            }

            /* Track max across all motions */
            if( nCurrentMotionSequenceCount > nMaxMotionSequenceCount )
            {
                nMaxMotionSequenceCount = nCurrentMotionSequenceCount;
            }
        }
    }

    /* Final calculation: customers * (max course + warmup + cum) * max motion sequence */
    if( stSizes.nCustomerCount > 0 && stSizes.nMotionCount > 0 && nMaxMotionSequenceCount > 0 )
    {
        stSizes.nMaxSequenceCount = stSizes.nCustomerCount * ( nMaxCustomerCourse + 2 ) * nMaxMotionSequenceCount;
    }

    cJSON_Delete( pJson );
    return stSizes;
}

/* --- Data Loading --- */
int LoadAllDataFromJson( const TCHAR *szFilename )
{
    /* file I/O */
    FILE *hFp = NULL;               /* file pointer */
    cJSON *pJson = NULL;
    char *pszData = NULL;           /* file data string */
    long nLen = 0;                  /* file length */

    /* customer data */
    cJSON *pCustomerObj = NULL;     /* customer object */
    cJSON *pCData = NULL;           /* customer data array */
    cJSON *pCDist = NULL;           /* customer distribution type */
    cJSON *pCMean = NULL;           /* customer distribution mean */
    cJSON *pCStd = NULL;            /* customer distribution std */

    /* size data */
    cJSON *pSizeObj = NULL;         /* size object */
    cJSON *pSData = NULL;           /* size data array */
    cJSON *pSDist = NULL;           /* size distribution type */
    cJSON *pSMean = NULL;           /* size distribution mean */
    cJSON *pSStd = NULL;            /* size distribution std */
    cJSON *pShape = NULL;           /* shape JSON object */

    /* volume, transparency, horizontal and vertical position */
    cJSON *pVolumeObj = NULL;       /* volume JSON object */
    cJSON *pTransObj = NULL;        /* transparency JSON object */
    cJSON *pDickposObj = NULL;      /* horizontal position JSON object */
    cJSON *pVerticalObj = NULL;     /* vertical position JSON object */
    int nVolumeVal = 0;             /* volume value (0-100) */
    int nTransVal = 0;              /* transparency value (0-100) */

    /* motion basic info */
    cJSON *pMotionArr = NULL;       /* motion array */
    cJSON *pMotionItem = NULL;      /* motion iteration item */
    cJSON *pMotionName = NULL;      /* motion name */
    cJSON *pMDist = NULL;           /* motion distribution type */
    cJSON *pMMean = NULL;           /* motion distribution mean */
    cJSON *pMStd = NULL;            /* motion distribution std */

    /* level and interval data */
    cJSON *pLevelArr = NULL;        /* level data array */
    cJSON *pLevelItem = NULL;       /* level iteration item */
    cJSON *pPreInterval = NULL;     /* preInterval object */
    cJSON *pPostInterval = NULL;    /* postInterval object */
    cJSON *pPrePos = NULL;          /* preInterval pos */
    cJSON *pPreMovetime = NULL;     /* preInterval movetime */
    cJSON *pPreWaittime = NULL;     /* preInterval waittime */
    cJSON *pPreBeep = NULL;         /* preInterval beep */
    cJSON *pPostPos = NULL;         /* postInterval pos */
    cJSON *pPostMovetime = NULL;    /* postInterval movetime */
    cJSON *pPostWaittime = NULL;    /* postInterval waittime */
    cJSON *pPostBeep = NULL;        /* postInterval beep */
    int nLevelIdx = 0;              /* level index */

    /* action data */
    cJSON *pActs = NULL;            /* action array */
    cJSON *pAction = NULL;          /* action iteration item */
    cJSON *pAPos = NULL;            /* action pos */
    cJSON *pAMovetime = NULL;       /* action movetime */
    cJSON *pAWaittime = NULL;       /* action waittime */
    cJSON *pABeep = NULL;           /* action beep */
    int nActionIdx = 0;             /* action index */

    /* Open file */
    hFp = _tfopen( szFilename, TEXT( "rb" ) );
    if( hFp == NULL )
    {
        return 0;
    }
    /* Check file size */
    fseek( hFp, 0, SEEK_END );
    nLen = ftell( hFp );
    fseek( hFp, 0, SEEK_SET );

    /* Allocate memory and read file */
    pszData = ( char * )malloc( ( size_t )( nLen + 1 ) );
    if( pszData == NULL )
    {
        fclose( hFp );
        return 0;
    }
    fread( pszData, 1, nLen, hFp );
    pszData[ nLen ] = '\0';
    fclose( hFp );

    /* Parse JSON */
    pJson = cJSON_Parse( pszData );
    free( pszData );
    if( !pJson )
    {
        return 0;
    }

    /* ===== Load volume setting ===== */
    {
        pVolumeObj = cJSON_GetObjectItem( pJson, "volume" );
        if( pVolumeObj )
        {
            nVolumeVal = ( int )pVolumeObj->valuedouble;
            if( nVolumeVal < 0 )
            {
                nVolumeVal = 0;
            }
            if( nVolumeVal > 100 )
            {
                nVolumeVal = 100;
            }
            g_nBeepVolume = nVolumeVal;
        }
    }

    /* ===== Load transparency setting ===== */
    {
        pTransObj = cJSON_GetObjectItem( pJson, "transparent" );
        if( pTransObj )
        {
            /* JSON value: 100=fully transparent, 0=opaque; convert to 0-255 alpha */
            nTransVal = ( int )pTransObj->valuedouble;
            g_nTransparency = ( int )( ( 100.0 - nTransVal ) / 100.0 * 255.0 );
            if( g_nTransparency < 0 )
            {
                g_nTransparency = 0;
            }
            if( g_nTransparency > 255 )
            {
                g_nTransparency = 255;
            }
        }
    }

    /* ===== Load horizontal position setting ===== */
    {
        pDickposObj = cJSON_GetObjectItem( pJson, "horizontal" );
        if( pDickposObj && pDickposObj->valuestring )
        {
            TCHAR *pszTmp = AllocTCHARFromUTF8( pDickposObj->valuestring );
            if( pszTmp )
            {
                _tcsncpy( g_szHorizontal, pszTmp, 19 );
                g_szHorizontal[ 19 ] = TEXT( '\0' );
                free( pszTmp );
            }
        }
    }
    /* ===== Load vertical position setting ===== */
    {
        pVerticalObj = cJSON_GetObjectItem( pJson, "vertical" );
        if( pVerticalObj && pVerticalObj->valuestring )
        {
            TCHAR *pszTmp = AllocTCHARFromUTF8( pVerticalObj->valuestring );
            if( pszTmp )
            {
                _tcsncpy( g_szVertical, pszTmp, 19 );
                g_szVertical[ 19 ] = TEXT( '\0' );
                free( pszTmp );
            }
        }
    }

    /* ===== Load customer data ===== */
    pCustomerObj = cJSON_GetObjectItem( pJson, "customer" );
    if( pCustomerObj )
    {
        /* Distribution info */
        pCDist = cJSON_GetObjectItem( pCustomerObj, "dist" );
        pCMean = cJSON_GetObjectItem( pCustomerObj, "mean" );
        pCStd  = cJSON_GetObjectItem( pCustomerObj, "std" );

        if( g_stCustomerDist.szDist )
        {
            free( g_stCustomerDist.szDist );
        }
        g_stCustomerDist.szDist = AllocTCHARFromUTF8( pCDist->valuestring );
        g_stCustomerDist.dMean = pCMean->valuedouble;
        g_stCustomerDist.dStd = pCStd->valuedouble;

        pCData = cJSON_GetObjectItem( pCustomerObj, "data" );
        g_nCustomerCount = 0;

        cJSON_ArrayForEach( pMotionItem, pCData )
        {
            g_pstCustomers[ g_nCustomerCount ].dLevel = cJSON_GetObjectItem( pMotionItem, "level" )->valuedouble;
            g_pstCustomers[ g_nCustomerCount ].nCourse = cJSON_GetObjectItem( pMotionItem, "course" )->valueint;
            g_pstCustomers[ g_nCustomerCount ].nSizeAdmend = cJSON_GetObjectItem( pMotionItem, "size_admend" )->valueint;
            g_pstCustomers[ g_nCustomerCount ].nMotionAdmend = cJSON_GetObjectItem( pMotionItem, "motion_admend" )->valueint;
            g_nCustomerCount++;
        }
    }

    /* ===== Load size data ===== */
    pSizeObj = cJSON_GetObjectItem( pJson, "size" );
    if( pSizeObj )
    {
        /* Distribution info */
        pSDist = cJSON_GetObjectItem( pSizeObj, "dist" );
        pSMean = cJSON_GetObjectItem( pSizeObj, "mean" );
        pSStd  = cJSON_GetObjectItem( pSizeObj, "std" );

        if( g_stSizeDist.szDist )
        {
            free( g_stSizeDist.szDist );
        }
        g_stSizeDist.szDist = AllocTCHARFromUTF8( pSDist->valuestring );
        g_stSizeDist.dMean = pSMean->valuedouble;
        g_stSizeDist.dStd = pSStd->valuedouble;

        /* Size data */
        pSData = cJSON_GetObjectItem( pSizeObj, "data" );
        g_nSizeCount = 0;

        cJSON_ArrayForEach( pMotionItem, pSData )
        {
            pShape = cJSON_GetObjectItem( pMotionItem, "shape" );
            g_pstSizes[ g_nSizeCount ].dLevel = cJSON_GetObjectItem( pMotionItem, "level" )->valuedouble;
            g_pstSizes[ g_nSizeCount ].szShape = AllocTCHARFromUTF8( ( pShape && pShape->valuestring ) ? pShape->valuestring : "" );
            g_nSizeCount++;
        }
    }

    /* ===== Load motion data ===== */
    pMotionArr = cJSON_GetObjectItem( pJson, "motion" );
    g_nMotionCount = 0;

    cJSON_ArrayForEach( pMotionItem, pMotionArr )
    {
        /* Motion basic info */
        pMotionName = cJSON_GetObjectItem( pMotionItem, "name" );
        {
            pMDist = cJSON_GetObjectItem( pMotionItem, "dist" );
            pMMean = cJSON_GetObjectItem( pMotionItem, "mean" );
            pMStd  = cJSON_GetObjectItem( pMotionItem, "std" );

            g_pstMotions[ g_nMotionCount ].szName = AllocTCHARFromUTF8( pMotionName->valuestring );
            g_pstMotions[ g_nMotionCount ].szDist = AllocTCHARFromUTF8( ( pMDist && pMDist->valuestring ) ? pMDist->valuestring : "normal" );
            g_pstMotions[ g_nMotionCount ].dMean = pMMean->valuedouble;
            g_pstMotions[ g_nMotionCount ].dStd = pMStd->valuedouble;
        }

        /* Level data */
        pLevelArr = cJSON_GetObjectItem( pMotionItem, "data" );
        nLevelIdx = 0;
        cJSON_ArrayForEach( pLevelItem, pLevelArr )
        {
            g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].dLevel = cJSON_GetObjectItem( pLevelItem, "level" )->valuedouble;
            g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].nReps = cJSON_GetObjectItem( pLevelItem, "reps" )->valueint;

            /* Parse preInterval (per level) */
            pPreInterval = cJSON_GetObjectItem( pLevelItem, "preInterval" );
            if( pPreInterval )
            {
                pPrePos      = cJSON_GetObjectItem( pPreInterval, "pos" );
                pPreMovetime = cJSON_GetObjectItem( pPreInterval, "movetime" );
                pPreWaittime = cJSON_GetObjectItem( pPreInterval, "waittime" );
                pPreBeep     = cJSON_GetObjectItem( pPreInterval, "beep" );

                g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].stPreInterval.nPos = pPrePos ? pPrePos->valueint : 1;
                g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].stPreInterval.dMovetime = pPreMovetime ? pPreMovetime->valuedouble : 0.0;
                g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].stPreInterval.dWaittime = pPreWaittime ? pPreWaittime->valuedouble : 0.0;

                /* Parse beep type */
                if( pPreBeep && pPreBeep->valuestring )
                {
                    if( !strcmp( pPreBeep->valuestring, "high" ) )
                    {
                        g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].stPreInterval.nBeepType = 1;
                    }
                    else if( !strcmp( pPreBeep->valuestring, "low" ) )
                    {
                        g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].stPreInterval.nBeepType = 2;
                    }
                    else if( !strcmp( pPreBeep->valuestring, "mid" ) )
                    {
                        g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].stPreInterval.nBeepType = 3;
                    }
                    else
                    {
                        g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].stPreInterval.nBeepType = 0;
                    }
                }
                else
                {
                    g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].stPreInterval.nBeepType = 0;
                }
            }
            else
            {
                g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].stPreInterval.nPos = 1;
                g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].stPreInterval.dMovetime = 0.0;
                g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].stPreInterval.dWaittime = 0.0;
                g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].stPreInterval.nBeepType = 0;
            }

            /* Parse postInterval (per level) */
            pPostInterval = cJSON_GetObjectItem( pLevelItem, "postInterval" );
            if( pPostInterval )
            {
                pPostPos      = cJSON_GetObjectItem( pPostInterval, "pos" );
                pPostMovetime = cJSON_GetObjectItem( pPostInterval, "movetime" );
                pPostWaittime = cJSON_GetObjectItem( pPostInterval, "waittime" );
                pPostBeep     = cJSON_GetObjectItem( pPostInterval, "beep" );

                g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].stPostInterval.nPos = pPostPos ? pPostPos->valueint : 1;
                g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].stPostInterval.dMovetime = pPostMovetime ? pPostMovetime->valuedouble : 0.0;
                g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].stPostInterval.dWaittime = pPostWaittime ? pPostWaittime->valuedouble : 0.0;

                /* Parse beep type */
                if( pPostBeep && pPostBeep->valuestring )
                {
                    if( !strcmp( pPostBeep->valuestring, "high" ) )
                    {
                        g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].stPostInterval.nBeepType = 1;
                    }
                    else if( !strcmp( pPostBeep->valuestring, "low" ) )
                    {
                        g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].stPostInterval.nBeepType = 2;
                    }
                    else if( !strcmp( pPostBeep->valuestring, "mid" ) )
                    {
                        g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].stPostInterval.nBeepType = 3;
                    }
                    else
                    {
                        g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].stPostInterval.nBeepType = 0;
                    }
                }
                else
                {
                    g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].stPostInterval.nBeepType = 0;
                }
            }
            else
            {
                g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].stPostInterval.nPos = 1;
                g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].stPostInterval.dMovetime = 0.0;
                g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].stPostInterval.dWaittime = 0.0;
                g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].stPostInterval.nBeepType = 0;
            }

            /* Action data */
            pActs = cJSON_GetObjectItem( pLevelItem, "act" );
            nActionIdx = 0;

            cJSON_ArrayForEach( pAction, pActs )
            {
                pABeep     = cJSON_GetObjectItem( pAction, "beep" );
                pAPos      = cJSON_GetObjectItem( pAction, "pos" );
                pAMovetime = cJSON_GetObjectItem( pAction, "movetime" );
                pAWaittime = cJSON_GetObjectItem( pAction, "waittime" );

                g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].astActs[ nActionIdx ].nPos = pAPos->valueint;
                g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].astActs[ nActionIdx ].dMovetime = pAMovetime->valuedouble;
                g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].astActs[ nActionIdx ].dWaittime = pAWaittime->valuedouble;

                /* Parse beep type */
                if( pABeep && pABeep->valuestring )
                {
                    if( !strcmp( pABeep->valuestring, "high" ) )
                    {
                        g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].astActs[ nActionIdx ].nBeepType = 1;
                    }
                    else if( !strcmp( pABeep->valuestring, "low" ) )
                    {
                        g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].astActs[ nActionIdx ].nBeepType = 2;
                    }
                    else if( !strcmp( pABeep->valuestring, "mid" ) )
                    {
                        g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].astActs[ nActionIdx ].nBeepType = 3;
                    }
                    else
                    {
                        g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].astActs[ nActionIdx ].nBeepType = 0;
                    }
                }
                else
                {
                    g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].astActs[ nActionIdx ].nBeepType = 0;
                }

                nActionIdx++;
            }

            g_pstMotions[ g_nMotionCount ].astLevels[ nLevelIdx ].nActCount = nActionIdx;
            nLevelIdx++;
        }

        g_pstMotions[ g_nMotionCount ].nLevelCount = nLevelIdx;
        g_nMotionCount++;
    }

    cJSON_Delete( pJson );
    return 1;
}

/* --- Free Memory --- */
void FreeAllData( void )
{
    int nMotionIdx = 0;             /* motion index for string free */

    if( g_pstCustomers )
    {
        free( g_pstCustomers );
        g_pstCustomers = NULL;
    }
    if( g_pstMotions )
    {
        for( nMotionIdx = 0; nMotionIdx < g_nMotionCount; nMotionIdx++ )
        {
            if( g_pstMotions[ nMotionIdx ].szName )
            {
                free( g_pstMotions[ nMotionIdx ].szName );
            }
            if( g_pstMotions[ nMotionIdx ].szDist )
            {
                free( g_pstMotions[ nMotionIdx ].szDist );
            }
        }
        free( g_pstMotions );
        g_pstMotions = NULL;
    }
    if( g_pstSizes )
    {
        for( nMotionIdx = 0; nMotionIdx < g_nSizeCount; nMotionIdx++ )
        {
            if( g_pstSizes[ nMotionIdx ].szShape )
            {
                free( g_pstSizes[ nMotionIdx ].szShape );
            }
        }
        free( g_pstSizes );
        g_pstSizes = NULL;
    }
    if( g_pstSequence )
    {
        for( nMotionIdx = 0; nMotionIdx < g_nActCount; nMotionIdx++ )
        {
            if( g_pstSequence[ nMotionIdx ].szMotionName )
            {
                free( g_pstSequence[ nMotionIdx ].szMotionName );
            }
            if( g_pstSequence[ nMotionIdx ].szShape )
            {
                free( g_pstSequence[ nMotionIdx ].szShape );
            }
        }
        free( g_pstSequence );
        g_pstSequence = NULL;
    }
    g_nCustomerCount = 0;
    g_nMotionCount = 0;
    g_nSizeCount = 0;
    g_nActCount = 0;
}

/* --- Core Sequence Generation Logic --- */
void AppendMotionWithIntervals( int nMotionIdx, double dTargetLevel, int nCustomerIdx, int nTotalCustomers, int nCourseIdx, int nTotalCourses, const TCHAR *szShape, double dCustomerLevel, double *pdAccumulatedTime )
{
    /* locals */
    Motion *pMotion = NULL;
    MotionLevel *pMotionLevel = NULL; /* motion level pointer */
    SequenceAct *pSequenceAction = NULL; /* sequence action pointer */
    int nClosestLevelIdx = 0;       /* best matching motion level index */
    int nLoopIdx = 0;               /* loop variable */
    int nRepIdx = 0;                /* rep loop variable */
    int nActionIdx = 0;             /* action loop variable */
    double dClosestDiff = 0.0;      /* closest level difference */
    double dCurrentDiff = 0.0;      /* current level difference */

    /* Validate input */
    if( nMotionIdx < 0 || nMotionIdx >= g_nMotionCount )
    {
        return;
    }

    pMotion = &g_pstMotions[ nMotionIdx ];

    /* Find best matching level: closest level regardless of direction */
    nClosestLevelIdx = 0;
    {
        dClosestDiff = 999999.0;
        dCurrentDiff = 0.0;

        for( nLoopIdx = 0; nLoopIdx < pMotion->nLevelCount; nLoopIdx++ )
        {
            /* Find closest level by absolute distance */
            dCurrentDiff = fabs( pMotion->astLevels[ nLoopIdx ].dLevel - dTargetLevel );
            if( dCurrentDiff < dClosestDiff )
            {
                dClosestDiff = dCurrentDiff;
                nClosestLevelIdx = nLoopIdx;
            }
        }
    }

    pMotionLevel = &pMotion->astLevels[ nClosestLevelIdx ];

    /* Pre-motion wait (preInterval) */
    if( pMotionLevel->stPreInterval.dMovetime > 0 || pMotionLevel->stPreInterval.dWaittime > 0 )
    {
        if( g_nActCount >= 10000 )
        {
            return;
        }

        pSequenceAction = &g_pstSequence[ g_nActCount ];
        memset( pSequenceAction, 0, sizeof( SequenceAct ) );
        g_nActCount++;

        /* Fill preInterval fields */
        pSequenceAction->nPos = pMotionLevel->stPreInterval.nPos;
        pSequenceAction->dMovetime = pMotionLevel->stPreInterval.dMovetime;
        pSequenceAction->dWaittime = pMotionLevel->stPreInterval.dWaittime; /* stored together */
        pSequenceAction->dStartTime = *pdAccumulatedTime;
        pSequenceAction->nBeepType = pMotionLevel->stPreInterval.nBeepType;

        /* Save metadata */
        pSequenceAction->dMotionLevel = pMotionLevel->dLevel;
        pSequenceAction->dCustomerLevel = dCustomerLevel;
        pSequenceAction->nCustomerIdx = nCustomerIdx;
        pSequenceAction->nTotalCustomers = nTotalCustomers;
        pSequenceAction->nCourseIdx = nCourseIdx;
        pSequenceAction->nTotalCourses = nTotalCourses;
        pSequenceAction->nCurrentRep = 1;
        pSequenceAction->nTotalReps = 1;

        pSequenceAction->szMotionName = _tcsdup( TEXT( "ready" ) );
        pSequenceAction->szShape = _tcsdup( szShape );

        /* Accumulate time */
        *pdAccumulatedTime += ( pMotionLevel->stPreInterval.dMovetime + pMotionLevel->stPreInterval.dWaittime );
    }

    /* Generate actions for each rep */
    for( nRepIdx = 0; nRepIdx < pMotionLevel->nReps; nRepIdx++ )
    {
        for( nActionIdx = 0; nActionIdx < pMotionLevel->nActCount; nActionIdx++ )
        {
            /* Check array overflow */
            if( g_nActCount >= 10000 )
            {
                return;
            }

            pSequenceAction = &g_pstSequence[ g_nActCount ];
            g_nActCount++;

            /* Timing and position info */
            pSequenceAction->nPos = pMotionLevel->astActs[ nActionIdx ].nPos;
            pSequenceAction->dMovetime = pMotionLevel->astActs[ nActionIdx ].dMovetime;
            pSequenceAction->dWaittime = pMotionLevel->astActs[ nActionIdx ].dWaittime;
            pSequenceAction->dStartTime = *pdAccumulatedTime;

            /* Set beep type */
            pSequenceAction->nBeepType = pMotionLevel->astActs[ nActionIdx ].nBeepType;

            /* Save metadata */
            pSequenceAction->dMotionLevel = pMotionLevel->dLevel;
            pSequenceAction->dCustomerLevel = dCustomerLevel;
            pSequenceAction->nCustomerIdx = nCustomerIdx;
            pSequenceAction->nTotalCustomers = nTotalCustomers;
            pSequenceAction->nCourseIdx = nCourseIdx;
            pSequenceAction->nTotalCourses = nTotalCourses;
            pSequenceAction->nCurrentRep = nRepIdx + 1;
            pSequenceAction->nTotalReps = pMotionLevel->nReps;

            pSequenceAction->szMotionName = _tcsdup( pMotion->szName );
            pSequenceAction->szShape = _tcsdup( szShape );

            /* Accumulate time */
            *pdAccumulatedTime += ( pSequenceAction->dMovetime + pSequenceAction->dWaittime );
        }
    }

    /* Post-motion wait (postInterval) */
    if( pMotionLevel->stPostInterval.dMovetime > 0 || pMotionLevel->stPostInterval.dWaittime > 0 )
    {
        if( g_nActCount >= 10000 )
        {
            return;
        }

        pSequenceAction = &g_pstSequence[ g_nActCount ];
        memset( pSequenceAction, 0, sizeof( SequenceAct ) );
        g_nActCount++;

        /* Fill postInterval fields */
        pSequenceAction->nPos = pMotionLevel->stPostInterval.nPos;
        pSequenceAction->dMovetime = pMotionLevel->stPostInterval.dMovetime;
        pSequenceAction->dWaittime = pMotionLevel->stPostInterval.dWaittime; /* stored together */
        pSequenceAction->dStartTime = *pdAccumulatedTime;
        pSequenceAction->nBeepType = pMotionLevel->stPostInterval.nBeepType;

        /* Save metadata */
        pSequenceAction->dMotionLevel = pMotionLevel->dLevel;
        pSequenceAction->dCustomerLevel = dCustomerLevel;
        pSequenceAction->nCustomerIdx = nCustomerIdx;
        pSequenceAction->nTotalCustomers = nTotalCustomers;
        pSequenceAction->nCourseIdx = nCourseIdx;
        pSequenceAction->nTotalCourses = nTotalCourses;
        pSequenceAction->nCurrentRep = pMotionLevel->nReps;
        pSequenceAction->nTotalReps = pMotionLevel->nReps;

        pSequenceAction->szMotionName = _tcsdup( TEXT( "rest" ) );
        pSequenceAction->szShape = _tcsdup( szShape );

        /* Accumulate time */
        *pdAccumulatedTime += ( pMotionLevel->stPostInterval.dMovetime + pMotionLevel->stPostInterval.dWaittime );
    }
}

void GenerateSequences( int nCount )
{
    /* locals */
    int nWarmupMotionIdx = 0;
    int nCumMotionIdx = 0;          /* cum motion index */
    int nCustomerIdx = 0;           /* customer loop variable */
    int nLoopIdx = 0;               /* general loop variable */
    double dAccumulatedTime = 0.0;  /* accumulated time */
    double dClosestLevelDiff = 0.0; /* closest level difference */
    double dCurrentLevelDiff = 0.0; /* current level difference */

    /* customer selection */
    CustomerInfo stSelectedCustomer = { 0 };
    int nSelectedCustomerIdx = 0;   /* selected customer index */
    int nMatchingCustomerCount = 0; /* matching customer count */
    int anMatchingCustomerIndices[ 20 ] = { 0, }; /* matching customer indices */
    double dGeneratedCustomerLevel = 0.0; /* generated customer level */

    /* size selection */
    SizeInfo stSelectedSize = { 0 }; /* selected size info */
    int nSelectedSizeIdx = 0;       /* selected size index */
    int nMatchingSizeCount = 0;     /* matching size count */
    int anMatchingSizeIndices[ 20 ] = { 0 }; /* matching size indices */
    double dGeneratedSizeLevel = 0.0; /* generated size level */

    /* motion and course selection */
    int nCourseIdx = 0;             /* course loop variable */
    int nSelectedMotionIdx = 0;     /* selected motion index */
    int nSelectedMotionLevelIdx = 0; /* selected motion level index */
    int nMatchingMotionCount = 0;   /* matching motion count */
    int anMatchingMotionIndices[ 20 ] = { 0 }; /* matching motion indices */
    double dGeneratedMotionLevel = 0.0; /* generated motion level */

    /* Initialize */
    g_nActCount = 0;
    nWarmupMotionIdx = -1;
    nCumMotionIdx = -1;

    /* Find special motion indices */
    for( nCustomerIdx = 0; nCustomerIdx < g_nMotionCount; nCustomerIdx++ )
    {
        if( !_tcscmp( g_pstMotions[ nCustomerIdx ].szName, TEXT( "warmup" ) ) )
        {
            nWarmupMotionIdx = nCustomerIdx;
        }
        if( !_tcscmp( g_pstMotions[ nCustomerIdx ].szName, TEXT( "cum" ) ) )
        {
            nCumMotionIdx = nCustomerIdx;
        }
    }

    /* Generate sequences per customer */
    for( nCustomerIdx = 0; nCustomerIdx < nCount; nCustomerIdx++ )
    {
        /* ===== Determine customer level ===== */
        dGeneratedCustomerLevel = GenerateSample( g_stCustomerDist.szDist, g_stCustomerDist.dMean, g_stCustomerDist.dStd );

        /* Find closest customer to target level (random on tie) */
        nSelectedCustomerIdx = 0;
        dClosestLevelDiff = 999999.0;
        dCurrentLevelDiff = 0.0;
        {
            nMatchingCustomerCount = 0;

            for( nLoopIdx = 0; nLoopIdx < g_nCustomerCount; nLoopIdx++ )
            {
                dCurrentLevelDiff = fabs( g_pstCustomers[ nLoopIdx ].dLevel - dGeneratedCustomerLevel );
                if( dCurrentLevelDiff < dClosestLevelDiff - LEVEL_EPSILON )
                {
                    dClosestLevelDiff = dCurrentLevelDiff;
                    nSelectedCustomerIdx = nLoopIdx;
                    nMatchingCustomerCount = 0;
                    anMatchingCustomerIndices[ nMatchingCustomerCount++ ] = nLoopIdx;
                }
                else if( dCurrentLevelDiff < dClosestLevelDiff + LEVEL_EPSILON )
                {
                    anMatchingCustomerIndices[ nMatchingCustomerCount++ ] = nLoopIdx;
                }
            }

            if( nMatchingCustomerCount > 1 )
            {
                nSelectedCustomerIdx = anMatchingCustomerIndices[ rand() % nMatchingCustomerCount ];
            }
        }
        stSelectedCustomer = g_pstCustomers[ nSelectedCustomerIdx ];

        if( g_bLog )
        {
            _tprintf( TEXT( "[Customer %d] generated_level=%.4f  selected_customer_idx=%d  level=%.4f  course=%d\n" ),
                nCustomerIdx + 1,
                dGeneratedCustomerLevel,
                nSelectedCustomerIdx,
                stSelectedCustomer.dLevel,
                stSelectedCustomer.nCourse );
        }

        /* ===== Determine size level ===== */
        dGeneratedSizeLevel = GenerateSample( g_stSizeDist.szDist, g_stSizeDist.dMean + stSelectedCustomer.nSizeAdmend, g_stSizeDist.dStd );

        /* Find closest size to target level (random on tie) */
        nSelectedSizeIdx = 0;
        dClosestLevelDiff = 999999.0;
        {
            nMatchingSizeCount = 0;

            for( nLoopIdx = 0; nLoopIdx < g_nSizeCount; nLoopIdx++ )
            {
                dCurrentLevelDiff = fabs( g_pstSizes[ nLoopIdx ].dLevel - dGeneratedSizeLevel );
                if( dCurrentLevelDiff < dClosestLevelDiff - LEVEL_EPSILON )
                {
                    dClosestLevelDiff = dCurrentLevelDiff;
                    nSelectedSizeIdx = nLoopIdx;
                    nMatchingSizeCount = 0;
                    anMatchingSizeIndices[ nMatchingSizeCount++ ] = nLoopIdx;
                }
                else if( dCurrentLevelDiff < dClosestLevelDiff + LEVEL_EPSILON )
                {
                    anMatchingSizeIndices[ nMatchingSizeCount++ ] = nLoopIdx;
                }
            }

            if( nMatchingSizeCount > 1 )
            {
                nSelectedSizeIdx = anMatchingSizeIndices[ rand() % nMatchingSizeCount ];
            }
        }
        stSelectedSize = g_pstSizes[ nSelectedSizeIdx ];

        if( g_bLog )
        {
            _tprintf( TEXT( "[Customer %d] generated_size_level=%.4f  selected_size=%s  level=%.4f\n" ),
                nCustomerIdx + 1,
                dGeneratedSizeLevel,
                stSelectedSize.szShape,
                stSelectedSize.dLevel );
        }

        /* ===== 1. Add Warmup ===== */
        if( nWarmupMotionIdx != -1 )
        {
            dGeneratedMotionLevel = GenerateSample( g_pstMotions[ nWarmupMotionIdx ].szDist, g_pstMotions[ nWarmupMotionIdx ].dMean + stSelectedCustomer.nMotionAdmend, g_pstMotions[ nWarmupMotionIdx ].dStd );

            /* Select motion level (random on tie) */
            {
                nSelectedMotionLevelIdx = 0;
                nMatchingMotionCount = 0;
                dClosestLevelDiff = 999999.0;
                for( nLoopIdx = 0; nLoopIdx < g_pstMotions[ nWarmupMotionIdx ].nLevelCount; nLoopIdx++ )
                {
                    dCurrentLevelDiff = fabs( g_pstMotions[ nWarmupMotionIdx ].astLevels[ nLoopIdx ].dLevel - dGeneratedMotionLevel );
                    if( dCurrentLevelDiff < dClosestLevelDiff - LEVEL_EPSILON )
                    {
                        dClosestLevelDiff = dCurrentLevelDiff;
                        nSelectedMotionLevelIdx = nLoopIdx;
                        nMatchingMotionCount = 0;
                        anMatchingMotionIndices[ nMatchingMotionCount++ ] = nLoopIdx;
                    }
                    else if( dCurrentLevelDiff < dClosestLevelDiff + LEVEL_EPSILON )
                    {
                        anMatchingMotionIndices[ nMatchingMotionCount++ ] = nLoopIdx;
                    }
                }
                if( nMatchingMotionCount > 1 )
                {
                    nSelectedMotionLevelIdx = anMatchingMotionIndices[ rand() % nMatchingMotionCount ];
                }

                if( g_bLog )
                {
                    _tprintf( TEXT( "[Customer %d] [warmup] motion_admend=%d  mean=%.4f  mean+admend=%.4f  generated=%.4f  selected_level=%.4f\n" ),
                        nCustomerIdx + 1,
                        stSelectedCustomer.nMotionAdmend,
                        g_pstMotions[ nWarmupMotionIdx ].dMean,
                        g_pstMotions[ nWarmupMotionIdx ].dMean + stSelectedCustomer.nMotionAdmend,
                        dGeneratedMotionLevel,
                        g_pstMotions[ nWarmupMotionIdx ].astLevels[ nSelectedMotionLevelIdx ].dLevel );
                }

                AppendMotionWithIntervals( nWarmupMotionIdx, g_pstMotions[ nWarmupMotionIdx ].astLevels[ nSelectedMotionLevelIdx ].dLevel, nCustomerIdx + 1, nCount, 0, stSelectedCustomer.nCourse, stSelectedSize.szShape, stSelectedCustomer.dLevel, &dAccumulatedTime );
            }
        }

        /* ===== 2. Add Main Courses ===== */
        for( nCourseIdx = 0; nCourseIdx < stSelectedCustomer.nCourse; nCourseIdx++ )
        {
            /* Select motion (excluding warmup and cum) */
            nSelectedMotionIdx = -1;
            do
            {
                nSelectedMotionIdx = rand() % g_nMotionCount;
            }
            while( nSelectedMotionIdx == nWarmupMotionIdx || nSelectedMotionIdx == nCumMotionIdx );

            /* Determine motion level */
            dGeneratedMotionLevel = GenerateSample( g_pstMotions[ nSelectedMotionIdx ].szDist, g_pstMotions[ nSelectedMotionIdx ].dMean + stSelectedCustomer.nMotionAdmend, g_pstMotions[ nSelectedMotionIdx ].dStd );

            /* Select motion level (random on tie) */
            {
                nSelectedMotionLevelIdx = 0;
                nMatchingMotionCount = 0;
                dClosestLevelDiff = 999999.0;
                for( nLoopIdx = 0; nLoopIdx < g_pstMotions[ nSelectedMotionIdx ].nLevelCount; nLoopIdx++ )
                {
                    dCurrentLevelDiff = fabs( g_pstMotions[ nSelectedMotionIdx ].astLevels[ nLoopIdx ].dLevel - dGeneratedMotionLevel );
                    if( dCurrentLevelDiff < dClosestLevelDiff - LEVEL_EPSILON )
                    {
                        dClosestLevelDiff = dCurrentLevelDiff;
                        nSelectedMotionLevelIdx = nLoopIdx;
                        nMatchingMotionCount = 0;
                        anMatchingMotionIndices[ nMatchingMotionCount++ ] = nLoopIdx;
                    }
                    else if( dCurrentLevelDiff < dClosestLevelDiff + LEVEL_EPSILON )
                    {
                        anMatchingMotionIndices[ nMatchingMotionCount++ ] = nLoopIdx;
                    }
                }
                if( nMatchingMotionCount > 1 )
                {
                    nSelectedMotionLevelIdx = anMatchingMotionIndices[ rand() % nMatchingMotionCount ];
                }

                if( g_bLog )
                {
                    _tprintf( TEXT( "[Customer %d] [course %d] motion=%s  motion_admend=%d  mean=%.4f  mean+admend=%.4f  generated=%.4f  selected_level=%.4f\n" ),
                        nCustomerIdx + 1,
                        nCourseIdx + 1,
                        g_pstMotions[ nSelectedMotionIdx ].szName,
                        stSelectedCustomer.nMotionAdmend,
                        g_pstMotions[ nSelectedMotionIdx ].dMean,
                        g_pstMotions[ nSelectedMotionIdx ].dMean + stSelectedCustomer.nMotionAdmend,
                        dGeneratedMotionLevel,
                        g_pstMotions[ nSelectedMotionIdx ].astLevels[ nSelectedMotionLevelIdx ].dLevel );
                }

                AppendMotionWithIntervals( nSelectedMotionIdx, g_pstMotions[ nSelectedMotionIdx ].astLevels[ nSelectedMotionLevelIdx ].dLevel, nCustomerIdx + 1, nCount, nCourseIdx + 1, stSelectedCustomer.nCourse, stSelectedSize.szShape, stSelectedCustomer.dLevel, &dAccumulatedTime );
            }
        }

        /* ===== 3. Add Cum ===== */
        if( nCumMotionIdx != -1 )
        {
            dGeneratedMotionLevel = GenerateSample( g_pstMotions[ nCumMotionIdx ].szDist, g_pstMotions[ nCumMotionIdx ].dMean + stSelectedCustomer.nMotionAdmend, g_pstMotions[ nCumMotionIdx ].dStd );

            /* Select motion level (random on tie) */
            {
                nSelectedMotionLevelIdx = 0;
                nMatchingMotionCount = 0;
                dClosestLevelDiff = 999999.0;
                for( nLoopIdx = 0; nLoopIdx < g_pstMotions[ nCumMotionIdx ].nLevelCount; nLoopIdx++ )
                {
                    dCurrentLevelDiff = fabs( g_pstMotions[ nCumMotionIdx ].astLevels[ nLoopIdx ].dLevel - dGeneratedMotionLevel );
                    if( dCurrentLevelDiff < dClosestLevelDiff - LEVEL_EPSILON )
                    {
                        dClosestLevelDiff = dCurrentLevelDiff;
                        nSelectedMotionLevelIdx = nLoopIdx;
                        nMatchingMotionCount = 0;
                        anMatchingMotionIndices[ nMatchingMotionCount++ ] = nLoopIdx;
                    }
                    else if( dCurrentLevelDiff < dClosestLevelDiff + LEVEL_EPSILON )
                    {
                        anMatchingMotionIndices[ nMatchingMotionCount++ ] = nLoopIdx;
                    }
                }
                if( nMatchingMotionCount > 1 )
                {
                    nSelectedMotionLevelIdx = anMatchingMotionIndices[ rand() % nMatchingMotionCount ];
                }

                if( g_bLog )
                {
                    _tprintf( TEXT( "[Customer %d] [cum] motion_admend=%d  mean=%.4f  mean+admend=%.4f  generated=%.4f  selected_level=%.4f\n" ),
                        nCustomerIdx + 1,
                        stSelectedCustomer.nMotionAdmend,
                        g_pstMotions[ nCumMotionIdx ].dMean,
                        g_pstMotions[ nCumMotionIdx ].dMean + stSelectedCustomer.nMotionAdmend,
                        dGeneratedMotionLevel,
                        g_pstMotions[ nCumMotionIdx ].astLevels[ nSelectedMotionLevelIdx ].dLevel );
                }

                AppendMotionWithIntervals( nCumMotionIdx, g_pstMotions[ nCumMotionIdx ].astLevels[ nSelectedMotionLevelIdx ].dLevel, nCustomerIdx + 1, nCount, stSelectedCustomer.nCourse, stSelectedCustomer.nCourse, stSelectedSize.szShape, stSelectedCustomer.dLevel, &dAccumulatedTime );
            }
        }
    }

    /* Set range for first customer */
    g_nCurrentCustomerIdx = 0;
    g_nCurrentCustomerStartAct = 0;
    g_nCurrentAct = 0;              /* start from first action of first customer */
    g_nCurrentCustomerEndAct = g_nActCount; /* default: full range */

    for( nCustomerIdx = 1; nCustomerIdx < g_nActCount; nCustomerIdx++ )
    {
        if( g_pstSequence[ nCustomerIdx ].nCustomerIdx != g_pstSequence[ 0 ].nCustomerIdx )
        {
            g_nCurrentCustomerEndAct = nCustomerIdx;
            break;
        }
    }
}

void SetupNextCustomer( void )
{
    /* loop variable */
    int nLoopIdx = 0;

    if( g_nCurrentCustomerEndAct >= g_nActCount )
    {
        /* No more customers, keep current */
        return;
    }

    g_nCurrentCustomerIdx++;
    g_nCurrentCustomerStartAct = g_nCurrentCustomerEndAct;
    g_nCurrentCustomerEndAct = g_nActCount;

    for( nLoopIdx = g_nCurrentCustomerStartAct; nLoopIdx < g_nActCount; nLoopIdx++ )
    {
        if( g_pstSequence[ nLoopIdx ].nCustomerIdx != g_pstSequence[ g_nCurrentCustomerStartAct ].nCustomerIdx )
        {
            g_nCurrentCustomerEndAct = nLoopIdx;
            break;
        }
    }

    g_nCurrentAct = g_nCurrentCustomerStartAct;
    g_dCurrentTimeInAct = 0.0;
    g_bFirstBeepPlayed = 0;
}

double GetYAtTime( double dTime, int nTopY, int nBottomY )
{
    /* locals */
    int nIdx = 0;
    int nLoopIdx = 0;               /* loop variable */
    int nPrevPos = 0;               /* previous position value */
    int nCurrPos = 0;               /* current position value */
    int nStartY = 0;                /* start Y coordinate */
    int nEndY = 0;                  /* end Y coordinate */
    double dDuration = 0.0;         /* action duration */
    double dStartTime = 0.0;        /* action start time */
    double dEndTime = 0.0;          /* action end time */
    double dLocalTime = 0.0;        /* elapsed time in current action */
    double dMoveTime = 0.0;         /* movement time */
    double dProgress = 0.0;         /* movement progress (0-1) */
    double dEasing = 0.0;           /* ease-in-out value */

    if( g_nCurrentCustomerEndAct <= g_nCurrentCustomerStartAct )
    {
        return ( double )nBottomY;
    }

    if( dTime <= g_pstSequence[ g_nCurrentCustomerStartAct ].dStartTime )
    {
        return ( double )GetYFromPos( g_pstSequence[ g_nCurrentCustomerStartAct ].nPos, nTopY, nBottomY );
    }

    /* Find action at given time */
    nIdx = g_nCurrentCustomerEndAct - 1;
    for( nLoopIdx = g_nCurrentCustomerStartAct; nLoopIdx < g_nCurrentCustomerEndAct; nLoopIdx++ )
    {
        dDuration = g_pstSequence[ nLoopIdx ].dMovetime + g_pstSequence[ nLoopIdx ].dWaittime;
        dStartTime = g_pstSequence[ nLoopIdx ].dStartTime;
        dEndTime = dStartTime + dDuration;

        if( dTime >= dStartTime && dTime < dEndTime )
        {
            nIdx = nLoopIdx;
            break;
        }
    }

    /* Get previous and current position */
    nPrevPos = ( nIdx == g_nCurrentCustomerStartAct ) ? g_pstSequence[ g_nCurrentCustomerStartAct ].nPos : g_pstSequence[ nIdx - 1 ].nPos;
    nCurrPos = g_pstSequence[ nIdx ].nPos;

    nStartY = GetYFromPos( nPrevPos, nTopY, nBottomY );
    nEndY = GetYFromPos( nCurrPos, nTopY, nBottomY );

    /* Elapsed time within current action */
    dLocalTime = dTime - g_pstSequence[ nIdx ].dStartTime;
    dMoveTime = g_pstSequence[ nIdx ].dMovetime;

    /* During movement (ease-in-out curve) */
    if( dLocalTime < dMoveTime && dMoveTime > 0 )
    {
        dProgress = dLocalTime / dMoveTime;
        dEasing = ( 1.0 - cos( dProgress * M_PI ) ) / 2.0;
        return nStartY + ( nEndY - nStartY ) * dEasing;
    }

    return ( double )nEndY;
}
