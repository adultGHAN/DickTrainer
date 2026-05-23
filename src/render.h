#pragma once

#include <windows.h>

void RenderInputScreen( HDC hdcMemory, int nWidth, int nHeight );
void RenderWaitingScreen( HDC hdcMemory, int nWidth, int nHeight );
void RenderCompletionScreen( HDC hdcMemory, int nWidth, int nHeight );
void RenderGraph( HDC hdcMemory, int nWidth, int nHeight );
