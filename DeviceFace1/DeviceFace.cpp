/*
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "stdafx.h"
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include "resource.h"

#include "Processing.h"
#include "Control.h"
#include "Graph.h"
#include "Recv4222.h"

bool terminateComm = false;

struct PaintContext
{
public:
	PAINTSTRUCT ps;
	HDC hdc;
	HWND hWnd;

	PaintContext(HWND hWnd_)
	{
		hWnd = hWnd_;
		hdc = BeginPaint(hWnd, &ps);
	}
	~PaintContext()
	{
		EndPaint(hWnd, &ps);
	}
	PaintContext(const PaintContext&) = delete;
	const PaintContext& operator=(const PaintContext&) = delete;
};

struct AppGlobals
{
	HINSTANCE hInst;
	//	HACCEL hAccelTable;
	const WCHAR* mainWndClassName;
	HWND mainWnd;
};

static AppGlobals appGlobals;

struct ScreenParam
{
	int dx;
	int dy;
	int bpc;
};

static ScreenParam screenParam = { 300, 400, 1 };

static LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK    ScreenOptionsDlg(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK    DialogStatus(HWND, UINT, WPARAM, LPARAM);

static ATOM MyRegisterClass(const AppGlobals* g)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = 0; //CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = g->hInst;
	wcex.hIcon = LoadIcon(g->hInst, MAKEINTRESOURCE(IDI_DEVICEFACE1));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_DEVICEFACE1);
	wcex.lpszClassName = g->mainWndClassName;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

void __cdecl _translateSEH(unsigned int, EXCEPTION_POINTERS*)
{
	throw "SEH";
}

static void InitInstance(AppGlobals* g, HINSTANCE hInstance)
{
	g->hInst = hInstance;
	g->mainWndClassName = L"DeviceFaceMainWndClass";
	//	g->hAccelTable = LoadAccelerators(g->hInst, MAKEINTRESOURCE(IDC_DEVICEFACE1));
	_set_se_translator(_translateSEH);
}

static HWND InitMainWnd(const AppGlobals* g, int nCmdShow)
{
	if (!MyRegisterClass(g))
		return FALSE;

	const WCHAR szTitle[] = L"DeviceFace";
	HWND hWnd = CreateWindowW(g->mainWndClassName, szTitle, WS_OVERLAPPEDWINDOW,
							  CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, g->hInst, nullptr);
	if (!hWnd)
		return NULL;

	createControls(screenParam.dx, 0);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return hWnd;
}

static WPARAM MainLoop(const AppGlobals* g)
{
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		//		if (!TranslateAccelerator(msg.hwnd, g->hAccelTable, &msg))
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
					  _In_opt_ HINSTANCE hPrevInstance,
					  _In_ LPWSTR    lpCmdLine,
					  _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	InitInstance(&appGlobals, hInstance);
	if (!(appGlobals.mainWnd = InitMainWnd(&appGlobals, nCmdShow)))
		return FALSE;

	if (IDOK != DialogBox(appGlobals.hInst, MAKEINTRESOURCE(IDD_DIALOG1), appGlobals.mainWnd, DialogStatus))
		return 1;

	HANDLE hComm = revcFT4222();

	MainLoop(&appGlobals);

	terminateComm = true;
	WaitForSingleObject(hComm, 5000);
	CloseHandle(hComm);

	return 0;
}

static BOOL ProcessMenu(HWND hWnd, WORD wmId)
{
	switch (wmId)
	{
	case IDM_ABOUT:
		DialogBox(appGlobals.hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, ScreenOptionsDlg);
		break;
	case IDM_EXIT:
		DestroyWindow(hWnd);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void gInvalidateDisplay()
{
	InvalidateRect(appGlobals.mainWnd, nullptr, TRUE);
}

void gPostMsg(int userMsg, int param1, int param2)
{
	PostMessageW(appGlobals.mainWnd, WM_USER + userMsg, param1, param2);
}

void gPostMsg(int userMsg, int param1, void* param2)
{
	PostMessageW(appGlobals.mainWnd, WM_USER + userMsg, param1, (LPARAM)param2);
}

static void drawFrame(HDC hdc)
{
	MoveToEx(hdc, 0, screenParam.dy, nullptr);
	LineTo(hdc, screenParam.dx, screenParam.dy);
	LineTo(hdc, screenParam.dx, 0);
}

static void drawUpDown(HDC hdc, int x, int y)
{
	const int dx = 20, dy = 20;
	MoveToEx(hdc, x, y, nullptr);
	LineTo(hdc, x + dx, y);
	LineTo(hdc, x + dx, y + 2 * dy);
	LineTo(hdc, x, y + 2 * dy);
	LineTo(hdc, x, y);
	MoveToEx(hdc, x, y + dy, nullptr);
	LineTo(hdc, x + dx, y + dy);
	SetBkMode(hdc, TRANSPARENT);
	TextOutA(hdc, x, y + 1, " +", 2);
	TextOutA(hdc, x, y + dy + 1, " -", 2);
}

static void paintScreen(const PaintContext* ctx)
{
	redraw(ctx);
	drawCompass(ctx);

	redrawControls(ctx);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	try
	{
		switch (message)
		{
		case WM_COMMAND:
		{
			if (!ProcessMenu(hWnd, LOWORD(wParam)))
				return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

		case (WM_USER + WM_USER_MSG_POINT_DATA):
		{
			const PositionRawData* pt = (PositionRawData *)lParam;
			processPoint(pt->id, pt->lat, pt->lon, pt->time);
			delete pt;
		}
		break;

		case (WM_USER + WM_USER_MSG_COMPASS_DATA):
		{
			const CompassRawData* pt = (CompassRawData *)lParam;
			processCompass(pt->dir);
			delete pt;
		}
		break;

		case (WM_USER + WM_USER_MSG_ZOOM):
		{
			processZoom(wParam);
		}
		break;

		case (WM_USER + WM_USER_MSG_MOVE):
		{
			processMove(wParam);
		}
		break;

		case (WM_USER + WM_USER_MSG_SCREEN_PARAM):
		{
			ScreenParam* p = reinterpret_cast<ScreenParam*>(lParam);
			if (p->dx >= 16 && p->dy >= 16 && p->dx < 1000 && p->dy < 1000)
			{
				screenParam = *p;
				gInvalidateDisplay();
			}
			delete p;
		}
		break;

		case WM_LBUTTONDOWN:
		{
			const int x = LOWORD(lParam);
			const int y = HIWORD(lParam);
			checkControls(x, y);
		}
		break;

		case WM_PAINT:
		{
			PaintContext ctx(hWnd);
			paintScreen(&ctx);
			drawFrame(ctx.hdc);
		}
		break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	catch (...)
	{
		Beep(880, 200);
	}
	return 0;
}

INT_PTR CALLBACK ScreenOptionsDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		SetDlgItemInt(hDlg, IDC_EDIT_DX, screenParam.dx, TRUE);
		SetDlgItemInt(hDlg, IDC_EDIT_DY, screenParam.dy, TRUE);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			if (LOWORD(wParam) == IDOK)
			{
				ScreenParam* p = new ScreenParam;
				BOOL translated;
				p->dx = GetDlgItemInt(hDlg, IDC_EDIT_DX, &translated, TRUE);
				if (translated)
				{
					p->dy = GetDlgItemInt(hDlg, IDC_EDIT_DY, &translated, TRUE);
					if (translated)
						PostMessageW(GetParent(hDlg), WM_USER + WM_USER_MSG_SCREEN_PARAM, 0, LPARAM(p));
				}
			}
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK DialogStatus(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		char buff[1024 * 4];
		int numDev = listFtUsbDevices(buff);
		if (numDev > 0)
		{
			SetDlgItemTextA(hDlg, IDC_STATUS, buff);
			PostMessageA(GetDlgItem(hDlg, IDC_STATUS), EM_SETSEL, -1, -1);
			if (numDev > 0)
				SetDlgItemInt(hDlg, IDC_SLAVE, slaveDevIdx, FALSE);
			if (numDev > 2)
				SetDlgItemInt(hDlg, IDC_MASTER, masterDevIdx, FALSE);
		}
		return (INT_PTR)TRUE;
	}

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			if (LOWORD(wParam) == IDOK)
			{
				BOOL t;
				int v1 = GetDlgItemInt(hDlg, IDC_SLAVE, &t, FALSE);
				slaveDevIdx = t ? v1 : -1;
				int v2 = GetDlgItemInt(hDlg, IDC_MASTER, &t, FALSE);
					masterDevIdx = t ? v2 : -1;
			}
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


// Graph ////////////////////

void gSetPoint(_In_ const PaintContext* ctx, int x, int y)
{
	const COLORREF c = RGB(255, 0, 0);
	SetPixel(ctx->hdc, x, y, c);
	SetPixel(ctx->hdc, x + 1, y, c);
	SetPixel(ctx->hdc, x, y + 1, c);
	SetPixel(ctx->hdc, x + 1, y + 1, c);
}

void gMoveTo(_In_ const PaintContext* ctx, int x, int y)
{
	MoveToEx(ctx->hdc, x, y, nullptr);
}

void gLineTo(_In_ const PaintContext* ctx, int x, int y)
{
	LineTo(ctx->hdc, x, y);
}

void gCircle(_In_ const PaintContext* ctx, int x, int y, int sz)
{
	ASSERT_DBG(sz >= 0);
	Ellipse(ctx->hdc, x - sz, y - sz, x + sz, y + sz);
}

void gSetLabel(_In_ const PaintContext* ctx, int x, int y, _In_z_ const char* text)
{
	SetBkMode(ctx->hdc, TRANSPARENT);
	TextOutA(ctx->hdc, x + 4, y, text, strlen(text));
}

void gGetLabelSize(_In_ int* dx, _In_ int* dy, _In_z_ const char* text)
{
	HDC dc = GetDC(appGlobals.mainWnd);
	SIZE sz;
	if (GetTextExtentPoint32A(dc, text, strlen(text), &sz))
	{
		*dx = sz.cx;
		*dy = sz.cy - 4;
	}
	else
	{
		*dx = *dy = 0;
	}
	ReleaseDC(appGlobals.mainWnd, dc);
}
