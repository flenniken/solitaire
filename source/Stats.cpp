// Copyright 2007-2019 Flenniken
// All Rights Reserved.

#include "stdafx.h"
#include "people.h"
#include "Klondike.h"
#include "Stats.h"
#include <vector>
#include <shlwapi.h>
#include <Gdiplus.h>
#include "Utils.h"
#include "Game.h"
#include "DrawUtils.h"
#include "SolitaireIniFile.h"

class StatDraw
{
public:
	#define kFontName _T("Arial")
	#define kFontSize 14
	#define kIndent 40

	StatDraw();
	~StatDraw();

	void DrawStatistics(HDC hdc);
	void DrawStatsText(Gdiplus::Graphics &graphics);
	void GetStartingStatWindowPos(int32 &x, int32 &y, uint32 &width, uint32 &height);
	uint32 GetMaxStringLength();
	void DrawLine(Gdiplus::Graphics &graphics, const Gdiplus::PointF &origin, const TString &string1, const TString &string2, const Gdiplus::Font &font);
	void InvalidateNumbers();

	static const int32 fXExtent = 100;
	int32 fYExtent;
	static const uint32 fPadding = 10;
	float fLineHeight;
};

extern Stats gStats;
extern HINSTANCE gInstance;
extern HWND gMainWindow;
extern Game gGame;

static ATOM RegisterStatsClass(HINSTANCE hInstance);
static LRESULT CALLBACK StatisticsProc(HWND, UINT, WPARAM, LPARAM);
static TString GetNumberString(double number);
static uint32 GetFrameWidth();
static void UpdateTitle(uint32 gamesPlayed);
static void SetRectF(const RECT &rect, Gdiplus::RectF &rectF);
static bool DrawOuch(int32 x, int32 y, const RECT &cardRect);
static void DrawRectangle(HDC hdc, const RECT &rect);
static void PositionWindow();
static void WriteWindowPos();


HWND gStatsWindow;
static StatDraw gStatDraw;
static Card::Number gCardNumber = Card::kAce;
static Card::Suit gCardSuit = Card::kSpades;
static bool gOuchUp;
static RECT gOuchRect;
static TCHAR szWindowClass[] = _T("StatisticsWindow");
// Ini file keys.
static TCHAR gTotalCardsUpStr[] = _T("Total Cards Up");
static TCHAR gGamesPlayedStr[] = _T("Games Played");
static TCHAR gGamesWonStr[] = _T("Games Won");
static TCHAR gWindowXStr[] = _T("Stats X");
static TCHAR gWindowYStr[] = _T("Stats Y");
static TCHAR gWindowWidthStr[] = _T("Stats Width");
static TCHAR gWindowHeightStr[] = _T("Stats Height");

int StatsUtils::CreateStatisticsWindow()
{
	WNDCLASSEX wcex;
	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= 0;  // CS_DBLCLKS; // CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= StatisticsProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= gInstance;
	wcex.hIcon			= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_KLONDIKE_ICON));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= NULL;
	ATOM atom = RegisterClassEx(&wcex);
	if (atom == 0)
		return 1; // Unable to create the class.

	int32 x;
	int32 y;
	uint32 width;
	uint32 height;
	gStatDraw.GetStartingStatWindowPos(x, y, width, height);

	gStatsWindow = CreateWindow(szWindowClass, _T(""), WS_VISIBLE | WS_CHILD | WS_THICKFRAME /*| WS_CAPTION*/,
		x, y, width, height, gMainWindow, NULL, gInstance, NULL);

	if (!gStatsWindow)
		return 1; // Unable to create the window.

	// Read the stats from the ini file.
	gStats.ReadStats();

	PositionWindow();

	UpdateWindow(gStatsWindow);

	return 0; // success
}

LRESULT CALLBACK StatisticsProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		
		gStatDraw.DrawStatistics(hdc);

		EndPaint(hWnd, &ps);
		break;
	}

	// No backgroud erasing, we just redraw the whole client area.
	case WM_ERASEBKGND:
		return 1;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void StatDraw::DrawStatistics(HDC hdc)
{
	Gdiplus::Graphics graphics(hdc);

	// Get the whole client area.
	RECT clientRect;
	GetClientRect(gStatsWindow, &clientRect);

	Gdiplus::SolidBrush solidBrush(Gdiplus::Color(150, 150, 150));
	Gdiplus::RectF rectF;
	SetRectF(clientRect, rectF);
	graphics.FillRectangle(&solidBrush, rectF);

	// Draw all the text.
	DrawStatsText(graphics);
}

void StatDraw::DrawStatsText(Gdiplus::Graphics &graphics)
{
	Gdiplus::Font font(kFontName, kFontSize);

	float x = (float)fPadding;
	float y = (float)fPadding*3;

	Gdiplus::PointF origin(x, y);
	Gdiplus::StringFormat format;

	TString message;

	// x Games Played
	TString gamesPlayed = Utils::GetString(gInstance, kStrGamesPlayed);
	message = Utils::GetNumberString((int32)gStats.GetTotalGamesFinished());
	DrawLine(graphics, origin, message, gamesPlayed, font);

	// x Games Won
	TString gamesWon = Utils::GetString(gInstance, kStrGamesWon);
	origin.Y += fLineHeight;
	message = Utils::GetNumberString((int32)gStats.GetGamesWon());
	DrawLine(graphics, origin, message, gamesWon, font);

	// xx.x% Percent Won
	TString percentWon = Utils::GetString(gInstance, kStrPercentWon);
	origin.Y += fLineHeight;
	origin.Y += fLineHeight;
	message = GetNumberString(gStats.GetPercentWon());
	DrawLine(graphics, origin, message, percentWon, font);

	// x.x Average Cards Up
	TString averageCardsUp = Utils::GetString(gInstance, kStrAverageCardsUp);
	origin.Y += fLineHeight;
	message = GetNumberString(gStats.GetAverageCardsUp());
	DrawLine(graphics, origin, message, averageCardsUp, font);

	// xx Cards Up
	TString strCardsUp = Utils::GetString(gInstance, kStrCardsUp);
	origin.Y += fLineHeight;
	origin.Y += fLineHeight;
	int32 cardsUp = (int32)gStats.GetCardsUp();
	message = Utils::GetNumberString(cardsUp);
	DrawLine(graphics, origin, message, strCardsUp, font);

	origin.Y += fLineHeight;
	origin.Y += 1.0f;
	fYExtent = (int32)origin.Y;

	TString strategy = Utils::GetString(gInstance, kStrStrategy);
	origin.Y += fLineHeight;
	origin.Y += fLineHeight;
	origin.X = (float)fPadding;
	DrawLine(graphics, origin, strategy, _T(""), font);

	origin.Y += fLineHeight;
//	origin.X = kIndent;
	gStats.GetStrategyLetters(message);
	DrawLine(graphics, origin, message, _T(""), font);

	// One at a time
	// OR
	// Three at a time.

	TString currentOptions = Utils::GetString(gInstance, kStrCurrentOptions);
	origin.Y += fLineHeight;
	origin.Y += fLineHeight;
	DrawLine(graphics, origin, currentOptions, _T(""), font);

	TString oneAtATime = Utils::GetString(gInstance, kStrOneAtATime);
	TString threeAtATime = Utils::GetString(gInstance, kStrThreeAtATime);
	origin.Y += fLineHeight;
	origin.X = kIndent;
	if (gStats.GetDealOneAtATime())
		message = oneAtATime;
	else
		message = threeAtATime;
	DrawLine(graphics, origin, message, _T(""), font);

	origin.Y += fLineHeight;
	if (gStats.GetOneTimeThrough())
	{
		TString oneTime = Utils::GetString(gInstance, kStrOneTime);
		TString threeTimes = Utils::GetString(gInstance, kStrThreeTimes);
		if (gStats.GetDealOneAtATime())
			message = oneTime;
		else
			message = threeTimes;
	}
	else
		message = Utils::GetString(gInstance, kStrMultipleTime);
	DrawLine(graphics, origin, message, _T(""), font);

	TString partialAllowed = Utils::GetString(gInstance, kStrOPartialAllowed);
	TString partialNotAllowed = Utils::GetString(gInstance, kStrOPartialNotAllowed);
	origin.Y += fLineHeight;
	if (gStats.GetAllowPartialPileMoves())
		message = partialAllowed;
	else
		message = partialNotAllowed;
	DrawLine(graphics, origin, message, _T(""), font);
}

void SetRectF(const RECT &rect, Gdiplus::RectF &rectF)
{
	rectF.X = (float)rect.left;
	rectF.Y = (float)rect.top;
	rectF.Width = (float)rect.right - rect.left;
	rectF.Height = (float)rect.bottom - rect.top;
}

void StatDraw::DrawLine(Gdiplus::Graphics &graphics, const Gdiplus::PointF &origin, const TString &string1, const TString &string2, const Gdiplus::Font &font)
{
	Gdiplus::Color color(0, 0, 0);
	Gdiplus::SolidBrush solidBrush(color);

	Gdiplus::PointF point = origin;
	Gdiplus::StringFormat format;
	graphics.DrawString(string1.c_str(), (int)string1.size(), &font, point, &format, &solidBrush);
	if (string2.size())
	{
		point.X = (float)fXExtent;
		graphics.DrawString(string2.c_str(), (int)string2.size(), &font, point, &format, &solidBrush);
	}
}

void StatDraw::GetStartingStatWindowPos(int32 &x, int32 &y, uint32 &width, uint32 &height)
{
	// Determine the left and right edges of the game.
	std::vector<VisibleRectangle> visibleRectangles;
	gGame.GetVisibleRectangles(0, 0, visibleRectangles);
	int32 xMost = 0;
	int32 xLeast = 1000;
	for (uint32 i = 0; i < visibleRectangles.size(); i++)
	{
		RECT rect = gGame.GetVisibleRect(visibleRectangles[i]);
		if (rect.right > xMost)
			xMost = rect.right;
		if (rect.left < xLeast)
			xLeast = rect.left;
	}

	// Get the width and height of the parent window.
	RECT parent;
	SetRect(&parent, 0, 0, 100, 100);
	GetClientRect(gMainWindow, &parent);

	uint32 frameWidth = GetFrameWidth();

	uint32 maxStringLength = GetMaxStringLength();
	int32 fixX = parent.right - (maxStringLength + fXExtent + fPadding);

	x = xMost + xLeast;
	if (fixX > x)
		x = fixX;

	y = 0 - frameWidth;

	if (parent.right - x > 0)
		width = parent.right - x;
	else
		width = 0;

	height = parent.bottom + 2 * frameWidth;
}

uint32 StatDraw::GetMaxStringLength()
{
	AutoReleaseDC autoReleaseDC;
	HDC hdc = autoReleaseDC.GetDC(gStatsWindow);
	Gdiplus::Graphics graphics(hdc);

	float width;
	float height;

	TString longString;
	gStats.GetStrategyLetters(longString);

	Gdiplus::Font font(kFontName, kFontSize);
	DrawUtils::MeasureString(graphics, longString, font, width, height);

	fLineHeight = height;

	return (uint32)width;
}

void StatDraw::InvalidateNumbers()
{
	RECT rect;
	SetRect(&rect, 0, 0, fXExtent, fYExtent);
	InvalidateRect(gStatsWindow, &rect, false);
}

void Stats::GameFinished(uint32 cardsUp) 
{
	fTotalCardsUp += cardsUp;
	fTotalGamesFinished++;
	fCardsUp = cardsUp;

	if (cardsUp == 52)
		fTotalGamesWon++;

	UpdateWindow(gStatsWindow);
	UpdateTitle(GetTotalGamesFinished());
//	InvalidateRect(gStatsWindow, NULL, false);
}

void UpdateTitle(uint32 gamesPlayed)
{
	TString message;
	message = Utils::GetNumberString((int32)gamesPlayed);
	message += _T(" ");
	message += Utils::GetString(gInstance, kStrGamesPlayed);

	SetWindowText(gMainWindow, message.c_str());
}

void Stats::RunningTotal(uint32 cardsUp)
{
	fCardsUp = cardsUp;
	gStatDraw.InvalidateNumbers();
}

void Stats::GameOptions(bool dealOneAtATime, bool allowPartialPileMoves, bool oneTimeThrough)
{
	fDealOneAtATime = dealOneAtATime;
	fAllowPartialPileMoves = allowPartialPileMoves;
	fOneTimeThrough = oneTimeThrough;			
	InvalidateRect(gStatsWindow, NULL, false);
}

void Stats::SetStrategyLetters(const TString &string)
{
	fStrategyLetters = string;
	InvalidateRect(gStatsWindow, NULL, false);
}

// Resize the window relative to the parent window.
void Stats::Resize(uint32 parentWidth, uint32 parentHeight)
{
	// Get the stats window width and height.
	RECT wRect;
	GetWindowRect(gStatsWindow, &wRect);

	// Get the parent window information.
	WINDOWINFO parentInfo;
	parentInfo.cbSize = sizeof(WINDOWINFO);
	GetWindowInfo(gMainWindow, &parentInfo);

	uint32 frameWidth = GetFrameWidth();;

	int32 x = wRect.left - parentInfo.rcClient.left;
	int32 y = wRect.top - parentInfo.rcClient.top;
	uint32 width = parentInfo.rcClient.right - wRect.left + frameWidth;;
	uint32 height = parentInfo.rcClient.bottom - parentInfo.rcClient.top + 2 * frameWidth;
//	uint32 height = wRect.bottom - wRect.top;

	MoveWindow(gStatsWindow, x, y, width, height, true);
}

#pragma warning ( disable : 4996 ) // warning C4996: '_swprintf' was declared deprecated

TString GetNumberString(double number)
{
  TCHAR buffer[256];

  _stprintf(buffer, _T("%.4f"), number);

  return TString(buffer);
}

uint32 GetFrameWidth()
{
	return GetSystemMetrics(SM_CXSIZEFRAME);
}

void DrawRectangle(HDC hdc, const RECT &rect)
{
	Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
}

StatDraw::StatDraw()
{
};

StatDraw::~StatDraw()
{
};


Stats::Stats() : 
	fTotalCardsUp(0), 
	fTotalGamesFinished(0), 
	fTotalGamesWon(0),
	fCardsUp(0),

	fDealOneAtATime(true),
	fAllowPartialPileMoves(true),
	fOneTimeThrough(true)
{
}

// Reset the stats.
void Stats::Reset()
{
	fTotalCardsUp = 0; 
	fTotalGamesFinished = 0; 
	fTotalGamesWon = 0;

	gStatDraw.InvalidateNumbers();
}

Stats::~Stats()
{
}

void Stats::ReadStats()
{
	IniFileAccess *iniFileAccess = SolitaireIniFile::GetIniFileAccess();

	iniFileAccess->GetUint32(gTotalCardsUpStr, fTotalCardsUp);
	iniFileAccess->GetUint32(gGamesPlayedStr, fTotalGamesFinished);
	iniFileAccess->GetUint32(gGamesWonStr, fTotalGamesWon);
}

void Stats::WriteStats()
{
	IniFileAccess *iniFileAccess = SolitaireIniFile::GetIniFileAccess();

	iniFileAccess->SetUint32(gTotalCardsUpStr, fTotalCardsUp);
	iniFileAccess->SetUint32(gGamesPlayedStr, fTotalGamesFinished);
	iniFileAccess->SetUint32(gGamesWonStr, fTotalGamesWon);

	WriteWindowPos();
}

void PositionWindow()
{
	IniFileAccess *iniFileAccess = SolitaireIniFile::GetIniFileAccess();

	// x, y are relative to the parent.
	int32 x;
	int32 y;
	uint32 width;
	uint32 height;
	if (iniFileAccess->GetInt32(gWindowXStr, x))
		return;
	if (iniFileAccess->GetInt32(gWindowYStr, y))
		return;
	if (iniFileAccess->GetUint32(gWindowWidthStr, width))
		return;
	if (iniFileAccess->GetUint32(gWindowHeightStr, height))
		return;

	MoveWindow(gStatsWindow, x, y, width, height, true);
}

void WriteWindowPos()
{
	RECT wRect;
	GetWindowRect(gStatsWindow, &wRect);

	// Get the parent window information.
	WINDOWINFO parentInfo;
	parentInfo.cbSize = sizeof(WINDOWINFO);
	GetWindowInfo(gMainWindow, &parentInfo);

	int32 x = wRect.left - parentInfo.rcClient.left;
	int32 y = wRect.top - parentInfo.rcClient.top;

	IniFileAccess *iniFileAccess = SolitaireIniFile::GetIniFileAccess();

	iniFileAccess->SetInt32(gWindowXStr, x);
	iniFileAccess->SetInt32(gWindowYStr, y);
	iniFileAccess->SetUint32(gWindowWidthStr, wRect.right - wRect.left);
	iniFileAccess->SetUint32(gWindowHeightStr, wRect.bottom - wRect.top);
}
