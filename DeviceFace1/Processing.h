#pragma once

#include "Protocol.h"

struct Control;
struct PaintContext;

extern void redraw(_In_ const PaintContext* context);
extern void drawCompass(_In_ const PaintContext* context);

extern void processPoint(unsigned int id, coord_t lat, coord_t lon, unsigned int time);
extern void processZoom(int z);
extern void processMove(int dir);
extern void processCompass(unsigned int dir);
