#pragma once

static const unsigned int WM_USER_MSG_ZOOM = 2;
static const unsigned int WM_USER_MSG_MOVE = 3;
static const unsigned int WM_USER_MSG_SCREEN_PARAM = 4;
static const unsigned int WM_USER_MSG_POINT_DATA = 5;
static const unsigned int WM_USER_MSG_COMPASS_DATA = 6;

extern void gPostMsg(int userMsg, int param1, int param2);
extern void gPostMsg(int userMsg, int param1, void* param2);

struct PaintContext;
extern void gSetPoint(_In_ const PaintContext* ctx, int x, int y);
extern void gMoveTo(_In_ const PaintContext* ctx, int x, int y);
extern void gLineTo(_In_ const PaintContext* ctx, int x, int y);
extern void gCircle(_In_ const PaintContext* ctx, int x, int y, int sz);
extern void gSetLabel(_In_ const PaintContext* ctx, int x, int y, _In_z_ const char* text);
extern void gGetLabelSize(_In_ int* dx, _In_ int* dy, _In_z_ const char* text);
extern void gInvalidateDisplay();
