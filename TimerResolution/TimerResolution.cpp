#include "TimerResolution.h"
#include <windows.h>
#include <mmsystem.h>

static UINT TargetResolution = 0;
static bool AlreadySet = false;

bool TimerResolution::Set()
{
        if (AlreadySet)
                return true;

        TIMECAPS tc;
        if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != MMSYSERR_NOERROR)
                return false;

        TargetResolution = min(max(tc.wPeriodMin, 1), tc.wPeriodMax);
        MMRESULT status = timeBeginPeriod(TargetResolution);
        if (status != TIMERR_NOERROR)
                return false;

        AlreadySet = true;

        return true;
}

bool TimerResolution::Reset()
{
        if (!AlreadySet)
                return true;

        MMRESULT status = timeEndPeriod(TargetResolution);
        if (status != TIMERR_NOERROR)
                return false;

        AlreadySet = false;

        return true;
}