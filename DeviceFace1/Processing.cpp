/*
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "stdafx.h"
#include "Processing.h"
#include "Graph.h"

struct Point2d
{
	int x, y;
};

struct Point2f
{
	coord_t lon, lat;
};

struct Screen
{
	Point2f center;
	double angle;
	int zoom;
};

static const coord_t INIT_LON = (coord_t)42.5; // TODO: remove
static const coord_t INIT_LAT = (coord_t)101.1;

Screen screen = { { INIT_LON, INIT_LAT }, 0, 1024 };

struct Rect
{
	int x, y;
	int dx, dy;
};

struct GeoPoint
{
	unsigned int id;
	unsigned int time;
	coord_t lon, lat;
	Rect label;
};

struct GeoData
{
	std::vector< GeoPoint > pos;
};

static GeoData data;

static void labelText(_Out_writes_z_(sz) char* dst, size_t sz, int id)
{
	*dst = 'L';
	_itoa_s(id, dst + 1, sz - 1, 10);
}

static int intersect(_In_ const Rect* a, _In_ const Rect* b)
{
	if ((a->y < b->y + b->dy) && (a->y + a->dy > b->y) &&
		(a->x < b->x + b->dx) && (a->x + a->dx > b->x))
		return (min(a->x + a->dx, b->x + b->dx) - max(a->x, b->x)) *
		(min(a->y + a->dy, b->y + b->dy) - max(a->y, b->y));
	return 0;
}

static int worldToScreenX(coord_t lat)
{
	return int(((lat - screen.center.lat) * screen.zoom) / 1024);
}

static int worldToScreenY(coord_t lon)
{
	return int(((lon - screen.center.lon) * screen.zoom) / 1024);
}

static void moveLabel(_In_ GeoPoint* p, int n)
{
	int x = worldToScreenX(p->lat);
	int y = worldToScreenY(p->lon);
	if (n == 0)
	{
		p->label.x = x;
		p->label.y = y - p->label.dy / 2;
	}
	else if (n == 1)
	{
		p->label.x = x;
		p->label.y = y - p->label.dy;
	}
	else
	{
		ASSERT_DBG(n == 2);
		p->label.x = x;
		p->label.y = y;
	}
}

static void positionLabel(_In_ GeoPoint* p)
{
	int bestSq = 1000000000; //  _INT_MAX
	int bestN = 0;
	for (int n = 0; n < 3; n++)
	{
		moveLabel(p, n);
		int sq = 0;
		for (const auto& it : data.pos)
		{
			if (it.id != p->id)
				sq += intersect(&it.label, &p->label);
		}
		if (sq < bestSq)
		{
			bestSq = sq;
			bestN = n;
		}
		if (sq == 0)
			break;
	}

	moveLabel(p, bestN);
}

static void positionAllLabels(_In_ GeoData* d)
{
	for (auto& it : d->pos)
	{
		it.label.y = it.label.x = 1000000000; // _INT_MAX
	}
	for (auto& it : d->pos)
	{
		positionLabel(&it);
	}
}

void processPoint(unsigned int id, coord_t lat, coord_t lon, unsigned int time)
{
	GeoPoint p;
	p.id = id;
	p.lon = lon;
	p.lat = lat;
	p.time = time;

	char name[32];
	labelText(name, sizeof(name), id);
	gGetLabelSize(&p.label.dx, &p.label.dy, name);
	positionLabel(&p);
	for (auto it = data.pos.begin(); ; ++it)
	{
		if (it == data.pos.end())
		{
			data.pos.push_back(p);
			break;
		}
		if (it->id == p.id)
		{
			*it = p;
			break;
		}
	}
	gInvalidateDisplay();
}

void redraw(_In_ const PaintContext* ctx)
{
	for (const auto& it : data.pos)
	{
		int x = worldToScreenX(it.lat);
		int y = worldToScreenY(it.lon);
		gSetPoint(ctx, x, y);
		char s[32];
		labelText(s, sizeof(s), it.id);
		gSetLabel(ctx, it.label.x, it.label.y, s);
	}
}

void drawCompass(_In_ const PaintContext* ctx)
{
	const int COMPASS_POS_X = 20;
	const int COMPASS_POS_Y = 20;
	const int COMPASS_SZ = 10;

	gCircle(ctx, COMPASS_POS_X, COMPASS_POS_Y, COMPASS_SZ);

	const int si = int(sin(screen.angle) * COMPASS_SZ);
	const int co = int(cos(screen.angle) * COMPASS_SZ);
	gMoveTo(ctx, COMPASS_POS_X - si, COMPASS_POS_Y + co);
	gLineTo(ctx, COMPASS_POS_X + si, COMPASS_POS_Y - co);

	gMoveTo(ctx, COMPASS_POS_X + si, COMPASS_POS_Y - co);
	gLineTo(ctx, COMPASS_POS_X + int(sin(screen.angle + 0.3) * (COMPASS_SZ - 2)), COMPASS_POS_Y - int(cos(screen.angle + 0.3) * (COMPASS_SZ - 2)));
	gMoveTo(ctx, COMPASS_POS_X + si, COMPASS_POS_Y - co);
	gLineTo(ctx, COMPASS_POS_X + int(sin(screen.angle - 0.3) * (COMPASS_SZ - 2)), COMPASS_POS_Y - int(cos(screen.angle - 0.3) * (COMPASS_SZ - 2)));
}

void processZoom(int z)
{
	ASSERT_DBG(z);
	if (z > 0)
	{
		if (screen.zoom < 1000000)
			screen.zoom *= 2;
	}
	else if (z < 0)
	{
		if (screen.zoom > 1)
			screen.zoom /= 2;
	}
	ASSERT_DBG(screen.zoom > 0);
	positionAllLabels(&data);
	gInvalidateDisplay();
}

void processMove(int dir)
{
	ASSERT_DBG(dir > 0);
	ASSERT_DBG(dir <= 4);
	if (dir == 1)
		screen.center.lon += 1;
	else if (dir == 3)
		screen.center.lon -= 1;
	else if (dir == 2)
		screen.center.lat += 1;
	else if (dir == 4)
		screen.center.lat -= 1;
	positionAllLabels(&data);
	gInvalidateDisplay();
}

void processCompass(unsigned int dir)
{
	screen.angle = (dir * 2 * M_PI) / 360;
	ASSERT_DBG(screen.angle < 2 * M_PI);
	ASSERT_DBG(screen.angle > -2 * M_PI);

	gInvalidateDisplay();
}