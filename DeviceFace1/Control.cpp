#include "stdafx.h"
#include "Control.h"
#include "Graph.h"
#include "Processing.h"

struct Rect
{
	int x, y;
	int dx, dy;
};

typedef bool (CONTROL_PROCESS_FUNC)(_In_ const Control*, int, int);
typedef void (CONTROL_DRAW_FUNC)(_In_ const Control*, _In_ const PaintContext*);

struct Control
{
	int id;
	Rect pos;
	CONTROL_PROCESS_FUNC* check;
	CONTROL_DRAW_FUNC* draw;
};

struct ScreenControls
{
	std::vector< Control > c;
};

static ScreenControls controls;

static bool checkUpDown(_In_ const Control* c, int x, int y)
{
	if (y < c->pos.dy / 2)
	{
		gPostMsg(WM_USER_MSG_ZOOM, 1, c->id);
	}
	else if (y > c->pos.dy / 2 + 1)
	{
		gPostMsg(WM_USER_MSG_ZOOM, -1, c->id);
	}
	else
	{
		return false;
	}
	return true;
}

static void drawUpDown(_In_ const Control* c, _In_ const PaintContext* ctx)
{
	gMoveTo(ctx, c->pos.x, c->pos.y);
	gLineTo(ctx, c->pos.x + c->pos.dx, c->pos.y);
	gLineTo(ctx, c->pos.x + c->pos.dx, c->pos.y + c->pos.dy);
	gLineTo(ctx, c->pos.x, c->pos.y + c->pos.dy);
	gLineTo(ctx, c->pos.x, c->pos.y);
	gMoveTo(ctx, c->pos.x, c->pos.y + c->pos.dy / 2);
	gLineTo(ctx, c->pos.x + c->pos.dx, c->pos.y + c->pos.dy / 2);
	gSetLabel(ctx, c->pos.x, c->pos.y, "+");
	gSetLabel(ctx, c->pos.x, c->pos.y + c->pos.dy / 2, " -");

	gSetLabel(ctx, c->pos.x - c->pos.dx / 2, c->pos.y + c->pos.dy, "Zoom");
}

static bool checkCross(_In_ const Control* c, int x, int y)
{
	if (y < c->pos.dy / 3)
		gPostMsg(WM_USER_MSG_MOVE, 1, c->id);
	else if (y > c->pos.dy * 2 / 3)
		gPostMsg(WM_USER_MSG_MOVE, 3, c->id);
	else if (x < c->pos.dx / 3)
		gPostMsg(WM_USER_MSG_MOVE, 4, c->id);
	else if (x > c->pos.dx * 2 / 3)
		gPostMsg(WM_USER_MSG_MOVE, 2, c->id);
	return true;
}

static void drawCross(_In_ const Control* c, _In_ const PaintContext* ctx)
{
	gMoveTo(ctx, c->pos.x + c->pos.dx / 3, c->pos.y);
	gLineTo(ctx, c->pos.x + c->pos.dx * 2 / 3, c->pos.y);
	gLineTo(ctx, c->pos.x + c->pos.dx * 2 / 3, c->pos.y + c->pos.dy);
	gLineTo(ctx, c->pos.x + c->pos.dx / 3, c->pos.y + c->pos.dy);
	gLineTo(ctx, c->pos.x + c->pos.dx / 3, c->pos.y);

	gMoveTo(ctx, c->pos.x, c->pos.y + c->pos.dy / 3);
	gLineTo(ctx, c->pos.x + c->pos.dx, c->pos.y + c->pos.dy / 3);
	gLineTo(ctx, c->pos.x + c->pos.dx, c->pos.y + c->pos.dy * 2 / 3);
	gLineTo(ctx, c->pos.x, c->pos.y + c->pos.dy * 2 / 3);
	gLineTo(ctx, c->pos.x, c->pos.y + c->pos.dy / 3);

	gSetLabel(ctx, c->pos.x + c->pos.dx / 3, c->pos.y, " ^");
	gSetLabel(ctx, c->pos.x + c->pos.dx / 3, c->pos.y + c->pos.dy * 2 / 3, " v");
	gSetLabel(ctx, c->pos.x, c->pos.y + c->pos.dy / 3, " <");
	gSetLabel(ctx, c->pos.x + c->pos.dx * 2 / 3, c->pos.y + c->pos.dy / 3, " >");

	gSetLabel(ctx, c->pos.x, c->pos.y + c->pos.dy, "Move");
}

void createControls(int x, int y)
{
	Control c;
	int n = 1;
	c.id = n++;
	c.pos.x = x + 50;
	c.pos.y =  y + 10;
	c.pos.dx = 20;
	c.pos.dy = 40;
	c.check = checkUpDown;
	c.draw = drawUpDown;

	controls.c.push_back(c);

	c.id = n++;
	c.pos.x = x + 30;
	c.pos.y = y + 100;
	c.pos.dx = 60;
	c.pos.dy = 60;
	c.check = checkCross;
	c.draw = drawCross;

	controls.c.push_back(c);

}

void checkControls(int x, int y)
{
	for (const auto& it : controls.c)
	{
		if (it.pos.x < x && it.pos.x + it.pos.dx > x &&
			it.pos.y < y && it.pos.y + it.pos.dy > y)
			if (it.check(&it, x - it.pos.x, y - it.pos.y))
				break;
	}
}

void redrawControls(_In_ const PaintContext* ctx)
{
	for (const auto& it : controls.c)
	{
		it.draw(&it, ctx);
	}
}
