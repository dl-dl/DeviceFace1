#pragma once

struct PaintContext;
extern void createControls(int x, int y);
extern void checkControls(int x, int y);
extern void redrawControls(_In_ const PaintContext* context);
