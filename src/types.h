#pragma once

#include <windows.h>

/* --- Data Structures --- */
typedef struct
{
    int nPos;                       /* position */
    double dMovetime;               /* move time */
    double dWaittime;               /* wait time */
    int nBeepType;                  /* beep type (0=none, 1=high(1200Hz), 2=low(400Hz), 3=mid(800Hz)) */
} Action;

typedef struct
{
    int nPos;                       /* position */
    double dMovetime;               /* move time */
    double dWaittime;               /* wait time */
    double dStartTime;              /* start time */

    TCHAR *szMotionName;             /* motion name */
    double dMotionLevel;            /* motion level */
    double dCustomerLevel;          /* customer level */

    int nCustomerIdx;               /* customer index */
    int nTotalCustomers;            /* total customers */
    int nCourseIdx;                 /* course index */
    int nTotalCourses;              /* total courses */

    int nCurrentRep;                /* current rep */
    int nTotalReps;                 /* total reps */
    TCHAR *szShape;                 /* size shape */

    int nBeepType;                  /* beep type (0=none, 1=high(1200Hz), 2=low(400Hz), 3=mid(800Hz)) */
} SequenceAct;

typedef struct
{
    double dLevel;                  /* level */
    int nReps;                      /* rep count */
    int nActCount;                  /* action count */
    Action astActs[ 50 ];           /* action array */
    Action stPreInterval;           /* pre-motion wait info */
    Action stPostInterval;          /* post-motion wait info */
} MotionLevel;

typedef struct
{
    TCHAR *szName;                   /* motion name */
    TCHAR *szDist;                   /* distribution type */
    double dMean;                   /* mean value */
    double dStd;                    /* standard deviation */

    int nLevelCount;                /* level count */
    MotionLevel astLevels[ 20 ];    /* level array */
} Motion;

typedef struct
{
    double dLevel;                  /* customer level */
    int nCourse;                    /* course count */
    int nSizeAdmend;                /* size adjustment */
    int nMotionAdmend;              /* motion adjustment */
} CustomerInfo;

typedef struct
{
    double dLevel;                  /* size level */
    TCHAR *szShape;                 /* shape */
} SizeInfo;

typedef struct
{
    TCHAR *szDist;                  /* distribution name */
    double dMean;                   /* mean value */
    double dStd;                    /* standard deviation */
} DistInfo;

/* --- Size calculation struct --- */
typedef struct
{
    int nCustomerCount;             /* customer count */
    int nMotionCount;               /* motion count */
    int nSizeCount;                 /* size count */
    int nMaxSequenceCount;          /* max sequence count */
} DataSizes;

/* --- Global Variables --- */
/* Customer info */
extern CustomerInfo *g_pstCustomers;
extern int g_nCustomerCount;
extern int g_nCustomerCapacity;

/* Motion info */
extern Motion *g_pstMotions;
extern int g_nMotionCount;
extern int g_nMotionCapacity;

/* Size info */
extern SizeInfo *g_pstSizes;
extern int g_nSizeCount;
extern int g_nSizeCapacity;

/* Distribution info */
extern DistInfo g_stCustomerDist;
extern DistInfo g_stSizeDist;

/* Sequence info */
extern SequenceAct *g_pstSequence;
extern int g_nActCount;
extern int g_nSequenceCapacity;

/* Current execution state */
extern int g_nCurrentAct;
extern double g_dCurrentTimeInAct;
extern int g_bFirstBeepPlayed;

/* Current customer range */
extern int g_nCurrentCustomerStartAct;
extern int g_nCurrentCustomerEndAct;
extern int g_nCurrentCustomerIdx;
extern int g_bGraphStarted;

/* GUI state */
extern int g_nGUIState;
extern TCHAR g_szInputBuf[ 16 ];
extern int g_bWaitingForEnter;
extern int g_bPaused;
extern int g_bCompleted;

/* Transparency */
extern int g_nTransparency;

/* Dick position */
extern TCHAR g_szDickPos[ 20 ];

/* Performance timing */
extern LARGE_INTEGER g_liFrequency;
extern LARGE_INTEGER g_liStartTime;
extern LARGE_INTEGER g_liLastTime;

/* Event handles */
extern HANDLE g_hHighBeepEvent;
extern HANDLE g_hLowBeepEvent;
extern HANDLE g_hMidBeepEvent;

/* Volume */
extern int g_nBeepVolume;
extern LARGE_INTEGER g_liVolumeChangeTime;

/* Transparency change timestamp */
extern LARGE_INTEGER g_liTransparencyChangeTime;

/* Transparency percentage (0~100) */
extern int g_nTransparencyPct;

/* Last input timestamp */
extern LARGE_INTEGER g_liLastInputTime;
