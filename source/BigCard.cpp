// Copyright 2007-2019 Flenniken
// All Rights Reserved.

#include "stdafx.h"
#include "people.h"
#include "Klondike.h"
#include "BigCard.h"
#include <vector>
#include <shlwapi.h>
#include <Gdiplus.h>
#include "Utils.h"
#include "Game.h"
#include "DrawUtils.h"

#define kDefaultCardWidth (90*4)
#define kDefaultCardHeight (131*4)
#define kMaximumCardWidth (90*8)
#define kMinimumCardWidth (90/2)
#define kResizeIncrement .05

class BigCard
{
public:
	#define kFontName _T("Arial")
	#define kFontSize 14

	BigCard();
	~BigCard();

	int OneTimeSettup();
	void DrawBigCard(HDC hdc);
	void GetStartingWindowPos(int32 &x, int32 &y, uint32 &width, uint32 &height);
	void MouseDown(int x, int y, bool forward);
	int ResizeCard(int32 direction);
	void ChangeCard(bool forward);
	void ChangeSuit(bool forward);

private:
	void DrawColor(HDC hdc, const RECT &clientRect, const RECT &cardRect, const std::vector<RECT> &suitRects, const std::vector<Card> &suitCards) const;
	void GetAreas(RECT &clientRect, RECT &cardRect, std::vector<RECT> &suitRects, std::vector<Card> &suitCards) const;
	void DrawCard(HDC hdc, int32 x, int32 y, const Card &card) const;

	uint32 fCardWidth;
	uint32 fCardHeight;
	HBITMAP fCardFace;
	CardSizer fCardSizer;
	Card::Suit fCardSuit;
	Card::Number fCardNumber;
	int32 fSuitIndex;
};


//extern Stats gBigCard;
extern HINSTANCE gInstance;
extern HWND gMainWindow;
extern Game gGame;
extern HWND gStatsWindow;

static ATOM RegisterStatsClass(HINSTANCE hInstance);
static LRESULT CALLBACK BigCardProc(HWND, UINT, WPARAM, LPARAM);
static TString GetNumberString(double number);
static uint32 GetFrameWidth();
static void UpdateTitle(uint32 gamesPlayed);
static void EraseBackground(Gdiplus::Graphics &graphics, const RECT &mainRect, const RECT &excludeRect, const Gdiplus::Color &color);
static void SetRectF(const RECT &rect, Gdiplus::RectF &rectF);
static bool IsAnythingVisible(HDC hdc, RECT &rect);
static bool IsVisible(const RECT &visibleArea, const RECT &rect);
static bool DrawOuch(int32 x, int32 y, const RECT &cardRect);
static void DrawRectangle(HDC hdc, const RECT &rect);
static int CreateBigCardWindow();
static void NextCard(WPARAM wParam);

static HWND gBigCardWindow;
static BigCard gBigCard;
static bool gOuchUp;
static RECT gOuchRect;
static Card::Suit gSuits[] = {Card::kSpades, Card::kDiamonds, Card::kClubs, Card::kHearts};

int CreateBigCardWindow()
{
	if (gBigCardWindow)
		return 1;
	ShowWindow(gStatsWindow, SW_HIDE);

	static TCHAR szWindowClass[] = _T("BigCardWindow");

	WNDCLASSEX wcex;
	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= 0;  // CS_DBLCLKS; // CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= BigCardProc;
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
	gBigCard.GetStartingWindowPos(x, y, width, height);

	gBigCardWindow = CreateWindow(szWindowClass, _T("Big Card"), WS_VISIBLE | WS_CHILD /*| WS_THICKFRAME | WS_CAPTION*/,
		x, y, width, height, gMainWindow, NULL, gInstance, NULL);

	if (!gBigCardWindow)
		return 1; // Unable to create the window.

	if (gBigCard.OneTimeSettup())
		return 1;

	UpdateWindow(gBigCardWindow);

	return 0; // success
}

int BigCard::OneTimeSettup()
{
	fCardSuit = Card::kSpades;
	fSuitIndex = 0;
	fCardNumber = Card::kAce;

	return ResizeCard(0);
}

LRESULT CALLBACK BigCardProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		
		gBigCard.DrawBigCard(hdc);

		EndPaint(hWnd, &ps);
		break;
	}

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		gBigCard.MouseDown(LOWORD(lParam), HIWORD(lParam), message == WM_LBUTTONDOWN ? true : false);
		break;

	case WM_KEYDOWN:
		NextCard(wParam);
		break;

	// No backgroud erasing, we just redraw the whole client area.
	case WM_ERASEBKGND:
		return 1;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void BigCard::DrawBigCard(HDC hdc)
{
	RECT clientRect;
	RECT cardRect;
	std::vector<RECT> suitRects;
	std::vector<Card> suitCards;
	GetAreas(clientRect, cardRect, suitRects, suitCards);

	// Draw the suits and the card.
	DrawColor(hdc, clientRect, cardRect, suitRects, suitCards);
}

bool IsAnythingVisible(HDC hdc, RECT &rect)
{
	if (GetClipBox(hdc, &rect) == NULLREGION)
		return false;
	return true;
}

bool IsVisible(const RECT &visibleArea, const RECT &rect)
{
	RECT intersect;
	if (IntersectRect(&intersect, &rect, &visibleArea))
		return true;
	return false;
}

void BigCard::DrawColor(HDC hdc, const RECT &clientRect, const RECT &cardRect, const std::vector<RECT> &suitRects, const std::vector<Card> &suitCards) const
{
	RECT visibleArea;
	if (!IsAnythingVisible(hdc, visibleArea))
		return;

	if (!IsVisible(visibleArea, clientRect))
		return;

	// Erase everything but the card area.
	Gdiplus::Graphics graphics(hdc);
	EraseBackground(graphics, clientRect, cardRect, Gdiplus::Color(200, 200, 200));

	// Draw the suits.
	for (uint32 i = 0; i < 4; i++)
	{
		RECT rect = suitRects[i];
		uint32 width = rect.right - rect.left;
		int32 cX = rect.left + width/2;
		int32 cY = rect.top + width/2;

		if (IsVisible(visibleArea, rect))
			DrawSuitCentered(hdc, suitCards[i], cX, cY, width, false);
	}

	// Draw a card.
	if (IsVisible(visibleArea, cardRect))
	{
		Card card(fCardNumber, fCardSuit);
		DrawCard(hdc, cardRect.left, cardRect.top, card);
	}
}

void BigCard::DrawCard(HDC hdc, int32 x, int32 y, const Card &card) const
{
	class DrawCardFunction : public IDrawFunction
	{
	public:
		DrawCardFunction(const Card &card, int32 x, int32 y, uint32 width, uint32 height) : 
		  fCard(card), fx(x), fy(y), fWidth(width), fHeight(height) {}
		~DrawCardFunction() {}
		int Draw(HDC hdc) const
		{
			DrawCardDirect(hdc, fx, fy, fWidth, fHeight, fCard);
			return 0;
		}
	private:
		Card fCard;
		int32 fx;
		int32 fy;
		uint32 fWidth;
		uint32 fHeight;
	} drawCardFunction(card, 0, 0, fCardWidth, fCardHeight);

	DrawToBitmap(fCardFace, drawCardFunction);

	DrawBitmap(hdc, fCardFace, x, y);
}

void EraseBackground(Gdiplus::Graphics &graphics, const RECT &mainRect, const RECT &excludeRect, const Gdiplus::Color &color)
{
	// Exclude the card rectangle so it area doesn't draw.
	Gdiplus::RectF rectF;
	SetRectF(excludeRect, rectF);
	graphics.SetClip(rectF, Gdiplus::CombineModeExclude);

	// Erase the window.
	Gdiplus::SolidBrush solidBrush(color);
	SetRectF(mainRect, rectF);
	graphics.FillRectangle(&solidBrush, rectF);
}

void BigCard::GetAreas(RECT &clientRect, RECT &cardRect, std::vector<RECT> &suitRects, std::vector<Card> &suitCards) const
{
	suitCards.push_back(Card(Card::kAce, Card::kSpades));
	suitCards.push_back(Card(Card::kAce, Card::kDiamonds));
	suitCards.push_back(Card(Card::kAce, Card::kClubs));
	suitCards.push_back(Card(Card::kAce, Card::kHearts));

	GetClientRect(gBigCardWindow, &clientRect);

	// Get the card position.
	int32 x = 100;
	int32 y = 25;
	SetRect(&cardRect, x, y, x+fCardWidth, y+fCardHeight);

	// Get the suit positions.
	int32 left = 20;
	int32 top = 35;
	uint32 diameter = 50;
	for (uint32 i = 0; i < 4; i++)
	{
		RECT rect;
		SetRect(&rect, left, top, left+diameter, top+diameter);
		suitRects.push_back(rect);
		top += 100;
	}
}

void SetRectF(const RECT &rect, Gdiplus::RectF &rectF)
{
	rectF.X = (float)rect.left;
	rectF.Y = (float)rect.top;
	rectF.Width = (float)rect.right - rect.left;
	rectF.Height = (float)rect.bottom - rect.top;
}

void BigCard::MouseDown(int x, int y, bool forward)
{
	if (gOuchUp)
	{
		ShowWindow(gStatsWindow, SW_SHOW);
		ShowWindow(gBigCardWindow, SW_HIDE);
//		InvalidateRect(gBigCardWindow, &gOuchRect, false);
		gOuchUp = false;
		return;
	}

	RECT clientRect;
	RECT cardRect;
	std::vector<RECT> suitRects;
	std::vector<Card> suitCards;
	GetAreas(clientRect, cardRect, suitRects, suitCards);

	POINT pt;
	pt.x = x;
	pt.y = y;
	if (PtInRect(&cardRect, pt))
	{
		if (fCardNumber == Card::kQueen && fCardSuit == Card::kHearts)
		{
			if (DrawOuch(x, y, cardRect))
				return;
		}
		ChangeCard(forward);
	}
	else
	{
		for (uint32 i = 0; i < 4; i++)
		{
			if (PtInRect(&suitRects[i], pt))
			{
				if (fCardSuit != suitCards[i].GetSuit())
				{
					fCardSuit = suitCards[i].GetSuit();
					InvalidateRect(gBigCardWindow, &cardRect, false);
					break;
				}
			}
		}
	}
}

void BigCard::ChangeSuit(bool forward)
{
	if (forward)
	{
		fSuitIndex++;

		if (fSuitIndex > 3)
			fSuitIndex = 0;
	}
	else
	{
		fSuitIndex--;

		if (fSuitIndex < 0)
			fSuitIndex = 3;
	}

	fCardSuit = gSuits[fSuitIndex];

	RECT clientRect;
	RECT cardRect;
	std::vector<RECT> suitRects;
	std::vector<Card> suitCards;
	GetAreas(clientRect, cardRect, suitRects, suitCards);

	InvalidateRect(gBigCardWindow, &cardRect, false);
}

void BigCard::ChangeCard(bool forward)
{
	if (forward)
	{
		if (fCardNumber == Card::kKing)
			fCardNumber = Card::kAce;
		else
			fCardNumber = (Card::Number)(fCardNumber + 1);
	}
	else
	{
		if (fCardNumber == Card::kAce)
			fCardNumber = Card::kKing;
		else
			fCardNumber = (Card::Number)(fCardNumber - 1);
	}

	RECT clientRect;
	RECT cardRect;
	std::vector<RECT> suitRects;
	std::vector<Card> suitCards;
	GetAreas(clientRect, cardRect, suitRects, suitCards);

	InvalidateRect(gBigCardWindow, &cardRect, false);
	UpdateWindow(gBigCardWindow);
}


// Draw ouch! when clicking on the face. Return true when it is.
bool DrawOuch(int32 x, int32 y, const RECT &cardRect)
{
	AutoReleaseDC autoReleaseDC;
	HDC hdc = autoReleaseDC.GetDC(gBigCardWindow);
	Gdiplus::Graphics graphics(hdc);

	COLORREF color = GetPixel(hdc, x, y);
	// whitish and blue color
//	if (!(color == RGB(196, 194, 203) || color == RGB(28, 69, 130)))
//		return false;

	// Red
	if (color != RGB(255, 0, 0))
		return false;

	TString string(_T("ouch!"));

	Gdiplus::Font font(kFontName, kFontSize);
	Gdiplus::PointF point((float)cardRect.left, (float)cardRect.top-20);
	Gdiplus::SolidBrush solidBrush(Gdiplus::Color(0, 0, 0));
	Gdiplus::StringFormat format;
	graphics.DrawString(string.c_str(), (int)string.size(), &font, point, &format, &solidBrush);

	SetRect(&gOuchRect, cardRect.left, cardRect.top-20, cardRect.right, cardRect.top);
	gOuchUp = true;

	return true;
}

void BigCard::GetStartingWindowPos(int32 &x, int32 &y, uint32 &width, uint32 &height)
{
	// Get the width and height of the parent window.
	RECT parent;
	SetRect(&parent, 0, 0, 100, 100);
	GetClientRect(gMainWindow, &parent);

	uint32 frameWidth = GetFrameWidth();

	x = parent.left;
	y = parent.top;

	width = parent.right - parent.left;
	height = parent.bottom - parent.top;
}

// Resize the window relative to the parent window.
void BigCardWindow::Resize(uint32 parentWidth, uint32 parentHeight)
{
	if (!gBigCardWindow)
		return;

	MoveWindow(gBigCardWindow, 0, 0, parentWidth, parentHeight, true);
}

void BigCardWindow::ChangeCard(WPARAM wParam)
{
	if (!gBigCardWindow)
		return;

	NextCard(wParam);
}

// DrawUtils
uint32 GetFrameWidth()
{
	return GetSystemMetrics(SM_CXSIZEFRAME);
}

// DrawUtils
void DrawRectangle(HDC hdc, const RECT &rect)
{
	Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
}

BigCard::BigCard(): 
	fCardFace(nil), 
	fCardWidth(kDefaultCardWidth), 
	fCardHeight(kDefaultCardHeight),
	fCardSizer(kDefaultCardWidth, kDefaultCardHeight, kMaximumCardWidth, kMinimumCardWidth, kResizeIncrement)
{
};

BigCard::~BigCard()
{
	if (fCardFace)
		DeleteObject(fCardFace);
};

int BigCardWindow::ResizeCard(int32 direction)
{
	if (!IsWindowVisible())
		return 1;

	return gBigCard.ResizeCard(direction);
}

int BigCard::ResizeCard(int32 direction)
{
	// Get the new width and height.
	uint32 width;
	uint32 height;
	fCardSizer.ResizeTheCard(fCardWidth, fCardHeight, direction, width, height);

	// Create a new bitmap big enough.
	if (DrawUtils::CreateBitmap(gBigCardWindow, width, height, fCardFace))
		return 1; // out of memory

	// Resize the card.
	fCardWidth = width;
	fCardHeight = height;

	RECT clientRect;
	RECT cardRect;
	std::vector<RECT> suitRects;
	std::vector<Card> suitCards;
	GetAreas(clientRect, cardRect, suitRects, suitCards);

	// Redraw everything except the strip with the suits.
	RECT rect;
	SetRect(&rect, cardRect.left, cardRect.top, clientRect.right, clientRect.bottom);
	InvalidateRect(gBigCardWindow, &rect, false);

	return 0; // success
}

void BigCardWindow::ShowBigCardWindow()
{
	if (!gBigCardWindow)
		CreateBigCardWindow();
	else
		ShowWindow(gBigCardWindow, SW_SHOW);
}

bool BigCardWindow::IsWindowVisible()
{
	if (!gBigCardWindow)
		return false;

	return ::IsWindowVisible(gBigCardWindow) ? true : false;
}

void NextCard(WPARAM wParam)
{
	switch (wParam)
	{
	case VK_RIGHT:
		gBigCard.ChangeCard(true);
		break;
	case VK_LEFT:
		gBigCard.ChangeCard(false);
		break;
	case VK_UP:
		gBigCard.ChangeSuit(false);
		break;
	case VK_DOWN:
		gBigCard.ChangeSuit(true);
		break;
	}
}
