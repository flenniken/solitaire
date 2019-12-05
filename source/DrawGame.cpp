// Copyright 2007-2019 Flenniken
// All Rights Reserved.

#include "stdafx.h"
#include "people.h"
#include "Klondike.h"
#include <vector>
#include <algorithm>
#include "TraceToNotepad.h"
#include "DrawUtils.h"
#include "Game.h"
#include "MyBrush.h"
#include "Utils.h"
#include <shlwapi.h>
#include "Gdiplus.h"
#include "Stats.h"
#include "Strategy.h"
#include "BigCard.h"
#include "SolitaireIniFile.h"

#define kDefaultCardWidth (90)
#define kDefaultCardHeight (131)
#define kMaximumCardWidth (90*2)
#define kMinimumCardWidth (90/2)
#define kResizeIncrement .05

#define kFontName _T("Arial")
#define kFontSize 10

int32 gEdgePad = 40;
int32 gTextPad = 8;

class SuitLocation
{
public:
	SuitLocation(int32 x, int32 y, bool flip) : fx(x), fy(y), fFlip(flip) {}
	~SuitLocation() {}
	int32 fx;
	int32 fy;
	bool fFlip;
};

class MyDoublePoint
{
public:
	double x;
	double y;
};

class GameLabel
{
public:
	GameLabel(int x, int y, uint32 id) : fx(x), fy(y), fID(id) {}
	~GameLabel(){};

	void GetBoundingBox(HDC hdc, Gdiplus::Font &font, RECT &rect, TString &string) const;

	int fx;			// point to draw the label.
	int fy;
	uint32 fID;		// Label resource id.
};

extern HINSTANCE gInstance;
extern HWND gMainWindow;
extern int32 gClientX;
extern int32 gClientY;
extern Stats gStats;
extern Strategy gStrategy;
extern HWND gStatsWindow;

static void DrawCardBack(HDC hdc, const RECT &rect);
static void DrawEmptyCard(HDC hdc, const RECT &rect);
static void DrawHitRectangle(HDC hdc, const RECT &rect, const RECT &rectClip);
static int DrawSuit(HDC hdc, int32 x, int32 y, uint32 width, uint32 height, const Card &card, bool drawSmall, bool drawCentered, bool flip=false);
static void DrawNumber(HDC hdc, int32 x, int32 y, uint32 width, const Card &card);
static bool DrawCardInside(HDC hdc, int32 x, int32 y, uint32 width, uint32 height, const Card &card);
static void GetSuitLocations(int32 x, int32 y, uint32 width, uint32 height, const Card &card, std::vector<SuitLocation> &locations);
static void GetPointsBetween(int32 x, int32 y, int32 x2, int32 y2, std::vector<POINT> &pts);
static int DrawResourceImage(HDC hdc, const ResourceID &resourceID, int32 x, int32 y, uint32 width, bool flip);
static void SetPoints(MyDoublePoint *sourcePoints, std::vector<Gdiplus::PointF> &destinationPoints, uint32 count);
static void GetSuitPoints(const Card &card, std::vector<Gdiplus::PointF> &one, std::vector<Gdiplus::PointF> &two, double &suitWidth, double &suitHeight);
static bool IsAnythingVisible(HDC hdc, RECT &rect);
static uint32 ScaleWidth(uint32 width, uint32 number);
static int DrawResourceImage(HDC hdc, const ResourceID &resourceID, int32 x, int32 y, uint32 width, uint32 height, bool whiteTransparent);
static int DrawResourceMetafile(HDC hdc, ResourceID resourceID, int32 x, int32 y, uint32 width, uint32 height);
static void DrawLabels(HDC hdc, RECT &visibleArea, std::vector<GameLabel> &gameLabels);

static void DrawRoundRect(HDC hdc, const RECT &rect, int32 corner, bool drawCorners);
static HDC MyCreateCompatibleDC();
static void DrawRectangle(HDC hdc, const RECT &rect);

// Ini file key names.
static TCHAR gCardWidthStr[] = _T("Card Width");
static TCHAR gCardHeightStr[] = _T("Card Height");

void Game::DrawGameNow()
{
	AutoReleaseDC autoReleaseDC;
	HDC hdc = autoReleaseDC.GetDC(gMainWindow);

	DrawGame(hdc, 0, 0, false);
	gStats.RunningTotal(fState.GetCardsUp());
}

void Game::GetVisibleRectangles(int32 x0, int32 y0, std::vector<VisibleRectangle> &visibleRectangles) const
{
	std::vector<GameLabel> gameLabels;
	GetVisibleRectangles(x0, y0, visibleRectangles, gameLabels);
}

void Game::GetVisibleRectangles(int32 x0, int32 y0, std::vector<VisibleRectangle> &visibleRectangles, std::vector<GameLabel> &gameLabels) const
{
	visibleRectangles.clear();

	int32 x = x0 + gEdgePad;
	int32 y = y0 + gEdgePad;

	// Deck
	visibleRectangles.push_back(VisibleRectangle(x, y, kDeck));
	gameLabels.push_back(GameLabel(x, y-gTextPad, kStrDeck));

	uint32 paddingBetween = gCardWidth/3;
	uint32 cardAndPadding = gCardWidth+paddingBetween;
	x += cardAndPadding;

	// Waste pile
	visibleRectangles.push_back(VisibleRectangle(x, y, kWastePile));
	gameLabels.push_back(GameLabel(x, y-gTextPad, kStrWastePile));

	// Hearts foundation pile.
	x += cardAndPadding;
	x += cardAndPadding;
	visibleRectangles.push_back(VisibleRectangle(x, y, kFoundation0));
	gameLabels.push_back(GameLabel(x, y-gTextPad, kStrFoundation));

	x += cardAndPadding;
	visibleRectangles.push_back(VisibleRectangle(x, y, kFoundation1));

	x += cardAndPadding;
	visibleRectangles.push_back(VisibleRectangle(x, y, kFoundation2));

	x += cardAndPadding;
	visibleRectangles.push_back(VisibleRectangle(x, y, kFoundation3));

	// Pile 0 top to bottom, pile 1 top to bottom, etc.
	x = x0 + gEdgePad;
	y = y0 + gEdgePad+gCardHeight+gCardHeight/2;
	gameLabels.push_back(GameLabel(x, y-gTextPad, kStrTableau));
	for (int32 index = 0; index < 7; index++)
	{
		y = y0 + gEdgePad+gCardHeight+gCardHeight/2;

		PileNumber pileNumber;

		pileNumber = PILENUMBER(kFaceDown0+index);
		uint32 faceDownCount = fState.GetPileSize(pileNumber);
		for (uint32 i = 0; i < faceDownCount; i++)
		{
			visibleRectangles.push_back(VisibleRectangle(x, y, pileNumber, i));
			y += gOverlapFaceDown;
		}

		pileNumber = PILENUMBER(kFaceUp0+index);
		uint32 faceUpCount = fState.GetPileSize(pileNumber);
		for (uint32 i = 0; i < faceUpCount; i++)
		{
			visibleRectangles.push_back(VisibleRectangle(x, y, pileNumber, i));
			y += gOverlap;
		}

		// Mark the top of the pile, if any in the pile.
		if (faceDownCount || faceUpCount)
			visibleRectangles[visibleRectangles.size()-1].fTopOfPile = true;
		else if (faceDownCount == 0 && faceUpCount == 0)
		{
			// Save the empty spot.
			visibleRectangles.push_back(VisibleRectangle(x, y, pileNumber, 0));
			visibleRectangles[visibleRectangles.size()-1].fEmptySpot = true;
		}

		x += cardAndPadding;
	}
}

Game::Game() :
	fCardSizer(kDefaultCardWidth, kDefaultCardHeight, kMaximumCardWidth, kMinimumCardWidth, kResizeIncrement)
{
}

Game::~Game()
{
	if (fCardFace)
		DeleteObject(fCardFace);
}

int Game::ResizeCard(int32 direction)
{
	// Get the new width and height.
	uint32 width;
	uint32 height;
	fCardSizer.ResizeTheCard(gCardWidth, gCardHeight, direction, width, height);

	// Resize the card.
	return ResizeCard(width, height);
}

int Game::ResizeCard(uint32 width, uint32 height)
{
	if (!fCardSizer.IsValidSizeCard(width, height))
		return 1;

	// Create a new bitmap big enough.
	if (DrawUtils::CreateBitmap(gMainWindow, width, height, fCardFace))
		return 1; // out of memory

	gCardWidth = width;
	gCardHeight = height;
	gOverlapFaceDown = gCardHeight/8;
	gOverlap = gCardHeight/4;

	Invalidate();

	return 0;
}

void Game::ReadCardSize()
{
	IniFileAccess *iniFileAccess = SolitaireIniFile::GetIniFileAccess();

	uint32 width;
	uint32 height;
	if (iniFileAccess->GetUint32(gCardWidthStr, width))
		return;
	if (iniFileAccess->GetUint32(gCardHeightStr, height))
		return;

	ResizeCard(width, height);
}

void Game::WriteCardSize()
{
	IniFileAccess *iniFileAccess = SolitaireIniFile::GetIniFileAccess();

	iniFileAccess->SetUint32(gCardWidthStr, gCardWidth);
	iniFileAccess->SetUint32(gCardHeightStr, gCardHeight);
}

int Game::OneTimeSettup()
{
	if (ResizeCard(0))
		return 1;

	bool dealOneAtATime;
	bool allowPartialPileMoves;
	bool oneTimeThrough;
	fState.GetOptions(dealOneAtATime, allowPartialPileMoves, oneTimeThrough);
	gStats.GameOptions(dealOneAtATime, allowPartialPileMoves, oneTimeThrough);

	TString string;
	gStrategy.GetStrategyLetters(string);
	gStats.SetStrategyLetters(string);

	TString gameString;
	fState.GetGameString(gameString);

	return 0;
}

void Game::DrawGame(HDC hdc, int32 x, int32 y, bool drawlabels) const
{
	RECT visibleArea;
	if (!IsAnythingVisible(hdc, visibleArea))
		return;

	SetGraphicsMode(hdc, GM_ADVANCED);

#if 0
	RECT rect;
	SetRect(&rect, 0, 0, gClientX, gClientY);
	FillRect(hdc, &rect, (HBRUSH)GetStockObject(LTGRAY_BRUSH)); 

	Card card(Card::kAce, Card::kClubs);
	DrawSuitCentered(hdc, card, 200, 400, gCardWidth-6, false);
	return;
#endif

	std::vector<VisibleRectangle> visibleRectangles;
	std::vector<GameLabel> gameLabels;
	GetVisibleRectangles(x, y, visibleRectangles, gameLabels);

	// Erase the background without touching the card areas.
	EraseBackground(hdc, visibleRectangles);

	// Draw the visible rectangles.
	uint32 count = (uint32)visibleRectangles.size();
	for (uint32 i = 0; i < count; i++)
	{
		RECT intersect;
		RECT rect = GetVisibleRect(visibleRectangles[i]);
		if (IntersectRect(&intersect, &rect, &visibleArea))
			DrawVisibleRectangle(hdc, visibleRectangles[i]);
	}

	if (drawlabels)
		DrawLabels(hdc, visibleArea, gameLabels);
}

void DrawLabels(HDC hdc, RECT &visibleArea, std::vector<GameLabel> &gameLabels)
{
	Gdiplus::Font font(kFontName, (float)kFontSize);
	Gdiplus::Color black(0, 0, 0);
	Gdiplus::SolidBrush solidBrush(black);
	Gdiplus::StringFormat format;
	Gdiplus::Graphics graphics(hdc);

	uint32 count = (uint32)gameLabels.size();
	for (uint32 i = 0; i < count; i++)
	{
		RECT boundingBox;
		TString string;
		gameLabels[i].GetBoundingBox(hdc, font, boundingBox, string);

		RECT intersect;
		if (IntersectRect(&intersect, &boundingBox, &visibleArea))
		{
			DrawRectangle(hdc, boundingBox);

			Gdiplus::PointF point((float)boundingBox.left, (float)boundingBox.top);
			graphics.DrawString(string.c_str(), (int)string.size(), &font, point, &format, &solidBrush);
		}
	}
}
bool IsAnythingVisible(HDC hdc, RECT &rect)
{
	if (GetClipBox(hdc, &rect) == NULLREGION)
		return false;
	return true;
}

void Game::EraseBackground(HDC hdc, const std::vector<VisibleRectangle> &visibleRectangles) const
{
	// Clip out the visible rectangles.
	uint32 count = (uint32)visibleRectangles.size();
	for (uint32 i = 0; i < count; i++)
	{
		if (!visibleRectangles[i].fEmptySpot)
		{
			RECT rect = GetCardRect(visibleRectangles[i]);
			ExcludeClipRect(hdc, rect.left, rect.top, rect.right, rect.bottom);
		}
	}

	// Erase the background.
	RECT rect;
	SetRect(&rect, 0, 0, gClientX, gClientY);
	FillRect(hdc, &rect, (HBRUSH)GetStockObject(LTGRAY_BRUSH)); 

	SelectClipRgn(hdc, NULL);
}

void Game::DrawVisibleRectangle(HDC hdc, const VisibleRectangle &visibleRectangle) const
{
	switch (visibleRectangle.fPileNumber)
	{
	case kDeck:
		DrawCardBack(hdc, visibleRectangle);
		break;

	case kWastePile:
	case kFoundation0:
	case kFoundation1:
	case kFoundation2:
	case kFoundation3:
		DrawTopCard(hdc, visibleRectangle);
		break;

	case kFaceDown0:
	case kFaceDown1:
	case kFaceDown2:
	case kFaceDown3:
	case kFaceDown4:
	case kFaceDown5:
	case kFaceDown6:
		DrawPartCardBack(hdc, visibleRectangle);
		break;

	case kFaceUp0:
	case kFaceUp1:
	case kFaceUp2:
	case kFaceUp3:
	case kFaceUp4:
	case kFaceUp5:
	case kFaceUp6:
		DrawPartCard(hdc, visibleRectangle);
		break;
	}
}

uint32 Game::GetPickUpHeight()
{
	uint32 count = fState.GetPileSize(kPickedUp);
	if (!count)
		return 0;
	return (count-1)*gOverlap+gCardHeight;
}

void Game::DrawPickUpPile(HDC hdc, int32 x, int32 y) const
{
	uint32 count = fState.GetPileSize(kPickedUp);

	for (uint32 i = 0; i < count; i++)
	{
		VisibleRectangle visibleRectangle(x, y, kPickedUp, i);
		if (i+1 == count)
			visibleRectangle.fTopOfPile = true;
		else
			visibleRectangle.fTopOfPile = false;

		DrawPartCard(hdc, visibleRectangle);

		y += gOverlap;
	}
}

void Game::DrawCardBack(HDC hdc, const VisibleRectangle &visibleRectangle) const
{
	if (fState.IsPileEmpty(visibleRectangle.fPileNumber))
	{
		DrawEmptyCard(hdc, GetCardRect(visibleRectangle));
		return;
	}

	::DrawCardBack(hdc, GetCardRect(visibleRectangle));
}

void Game::DrawPartCardBack(HDC hdc, const VisibleRectangle &visibleRectangle) const
{
	if (fState.IsPileEmpty(visibleRectangle.fPileNumber))
	{
		DrawEmptyCard(hdc, GetCardRect(visibleRectangle));
		return;
	}

	// If it's the very top card, draw the whole thing, otherwise draw the visible overlap.

	if (!visibleRectangle.fTopOfPile)
	{
		RECT rect = GetCardRect(visibleRectangle);
		rect.top += GetVisibleOverlap(visibleRectangle);
		ExcludeClipRect(hdc, rect.left, rect.top, rect.right, rect.bottom);
	}

	::DrawCardBack(hdc, GetCardRect(visibleRectangle));

	if (!visibleRectangle.fTopOfPile)
	{
		SelectClipRgn(hdc, NULL);
	}
}

void Game::DrawTopCard(HDC hdc, const VisibleRectangle &visibleRectangle) const
{
	if (fState.IsPileEmpty(visibleRectangle.fPileNumber))
		DrawEmptyCard(hdc, GetCardRect(visibleRectangle));
	else
		DrawCard(hdc, visibleRectangle);
}

void Game::DrawPartCard(HDC hdc, const VisibleRectangle &visibleRectangle) const
{
	Card card;
	if (fState.GetCard(visibleRectangle.fPileNumber, visibleRectangle.fPosition, card))
		return;

	// If it's the very top card, draw the whole thing, otherwise draw the visible overlap.

	if (!visibleRectangle.fTopOfPile)
	{
		RECT rect = GetCardRect(visibleRectangle);
		rect.top += GetVisibleOverlap(visibleRectangle);
		int rc = ExcludeClipRect(hdc, rect.left, rect.top, rect.right, rect.bottom);
	}

	DrawCard(hdc, visibleRectangle);

	if (!visibleRectangle.fTopOfPile)
	{
		SelectClipRgn(hdc, NULL);
	}
}

void Game::DrawCard(HDC hdc, const VisibleRectangle &visibleRectangle) const
{
	Card card;
	if (fState.GetCard(visibleRectangle.fPileNumber, visibleRectangle.fPosition, card))
		return;

	RECT rect = GetCardRect(visibleRectangle);

	DrawCard(hdc, rect.left, rect.top, card);
}

void Game::DrawCard(HDC hdc, const VisibleRectangle &visibleRectangle, int32 x, int32 y) const
{
	Card card;
	if (fState.GetCard(visibleRectangle.fPileNumber, visibleRectangle.fPosition, card))
		return;

	DrawCard(hdc, x, y, card);
}

void Game::DrawCard(HDC hdc, int32 x, int32 y, const Card &card) const
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
	} drawCardFunction(card, 0, 0, gCardWidth, gCardHeight);

	DrawToBitmap(fCardFace, drawCardFunction);

	DrawBitmap(hdc, fCardFace, x, y);
}

void DrawCardDirect(HDC hdc, int32 x, int32 y, uint32 width, uint32 height, const Card &card)
{
	// Draw the corners gray, draw the edges black and draw interior white.
	RECT rect;
	SetRect(&rect, x, y, x+width, y+height);
	SelectObject(hdc, GetStockObject(WHITE_BRUSH));
	DrawRoundRect(hdc, rect, ScaleWidth(width, 9), false);

	if (DrawCardInside(hdc, x, y, width, height, card))
		return; // done drawing

	uint32 xOffset = ScaleWidth(width, 14);
	uint32 yOffset = ScaleWidth(width, 25);

	uint32 nYOffset = ScaleWidth(width, 5);

	DrawSuit(hdc, rect.left+xOffset, rect.top+yOffset, width, height, card, true, false);
	DrawNumber(hdc, rect.left+xOffset, rect.top+nYOffset, width, card);

	// Flip the card's number and suit to draw the bottom.
    XFORM xForm; 
	xForm.eM11 = (FLOAT) -1.0; 
	xForm.eM12 = (FLOAT) 0.0; 
	xForm.eM21 = (FLOAT) 0.0; 
	xForm.eM22 = (FLOAT) -1.0; 
	xForm.eDx  = (FLOAT) 0.0; 
	xForm.eDy  = (FLOAT) 0.0; 
	SetWorldTransform(hdc, &xForm); 

    DPtoLP(hdc, (LPPOINT) &rect, 2); 

	DrawNumber(hdc, rect.right+xOffset, rect.bottom+nYOffset, width, card);
	DrawSuit(hdc, rect.right+xOffset, rect.bottom+yOffset, width, height, card, true, false);

	ModifyWorldTransform(hdc, NULL, MWT_IDENTITY);
}

MyDoublePoint gSpade1[] = {
	{306.3, 1},
	{356, 130.4}, {616.9, 212.3}, {610.3, 405.8},
	{606.3, 556.5}, {435.1, 598.9}, {335.6, 489.4},
	{357.5, 586.9}, {357.5, 692.4}, {451, 738.2}
};
#define gSpade1Count (sizeof(gSpade1)/sizeof(gSpade1[0]))

MyDoublePoint gSpade2[] = {
	{161, 738},
	{254.5, 692.4}, {254.5, 586.9}, {276.4, 489.4},
	{177, 598.8}, {5.8, 527.2}, {1.8, 405.8},
	{-4.8, 212.3}, {256.1, 130.4},  {305.9, 1}

};
#define gSpade2Count (sizeof(gSpade2)/sizeof(gSpade2[0]))
#define gSpadeWidth 611
#define gSpadeHeight 739

MyDoublePoint gHeart1[] = {
	{92, 186.68},
	{88.487, 169.728}, {44.082, 142.106}, {16.459, 93.155},
	{-45.275, -16.246}, {92, -19.987}, {92, 36.501}
};
MyDoublePoint gHeart2[] = {
	{91.942, 186.68},
	{95.455, 169.727}, {139.741, 142.043}, {167.363, 93.092},
	{229.096, -16.308}, {91.942, -19.987}, {91.942, 36.501}
};
#define gHeart1Count (sizeof(gHeart1)/sizeof(gHeart1[0]))
#define gHeart2Count (sizeof(gHeart2)/sizeof(gHeart2[0]))
#define gHeartWidth 183.906
#define gHeartHeight 186.782

MyDoublePoint gDiamond[] = {
	{183.027, 94.685},
	{122.622, 122.606}, {94.306, 187.064}, {94.306, 187.064},
	{94.231, 187.019}, {65.479, 122.606}, {5.513, 94.641},
	{65.479, 65.774}, {94.263, 1.535}, {94.263, 1.535},
	{94.294, 1.46}, {122.622, 65.774}, {183.014, 93.838}
};
#define gDiamondCount (sizeof(gDiamond)/sizeof(gDiamond[0]))
#define gDiamondWidth 178.547
#define gDiamondHeight 187.236

MyDoublePoint gClub1[] = {
	{89.559, 85.174},
	{89.216, 111.97}, {83.078, 205.094}, {65.872, 208.16}
};
#define gClub1Count (sizeof(gClub1)/sizeof(gClub1[0]))

MyDoublePoint gClub2[] = {
	{123.961, 208.159},
	{106.736, 205.094}, {100.593, 111.94}, {100.249, 85.136}
};
#define gClub2Count (sizeof(gClub2)/sizeof(gClub2[0]))
#define gClubWidth 190.0
#define gClubHeight 208.0



#if 0
// diamond
<path fill="#FFFFFF" stroke="#000000" d="
M 183.027	94.685
C	122.622	122.606	94.306	187.064	94.306	187.064
C	94.231	187.019	65.479	122.606	5.513	94.641
C	65.479	65.774	94.263	1.535	94.263	1.535
C	94.294	1.46	122.622	65.774	183.014	93.838
"/>
<rect x="4.979" y="0.5" fill="none" stroke="#000000" width="178.547" height="187.236"/>


// club.svg
<ellipse cx="94.517" cy="45.592" rx="45" ry="45"/>
<ellipse cx="45.017" cy="119.81" rx="45" ry="45"/>
<ellipse cx="145.017" cy="119.81" rx="45" ry="45"/>
<path d="
M					89.559	85.174
C	89.216	111.97	83.078	205.094	65.872	208.16
L					123.961	208.159
C	106.736	205.094	100.593	111.94	100.249	85.136
"/>
<rect x="71.851" y="80.667" width="45.5" height="30.144"/>
<rect x="0.00" y="0.0" fill="none" stroke="#000000" width="190.0" height="208.0"/>




// heart.svg
<path fill="none" stroke="#000000" d="
M 92, 186.68						
C 	88.487	169.728	44.082	142.106	16.459	93.155
C -45.275 -16.246,   92     -19.987,       92,     36.501						
  "/>
<path fill="none" stroke="#000000" d="
M 91.942, 186.68						
C	95.455	169.727	139.741	142.043	167.363	93.092
C	229.096	-16.308	91.942	-19.987	91.942	36.501
	"/>
<rect x="0.01" y="0.013" fill="#FFFFFF" stroke="#000000" width="183.906" height="186.782"/>


// spade2.svg
<svg  version="1.0" id="Layer_1" xmlns="&ns_svg;" xmlns:xlink="&ns_xlink;" width="612" height="739.6" viewBox="0 0 612 739.6"
	 overflow="visible" enable-background="new 0 0 612 739.6" xml:space="preserve">
<path d="
M					306.3	1

C	356	130.4	616.9	212.3	610.3	405.8
C	606.3	556.5	435.1	598.9	335.6	489.4
C	357.5	586.9	357.5	692.4	451	738.2

H					161


C	254.5	692.4	254.5	586.9	276.4	489.4

C	177	598.8	5.8	527.2	1.8	405.8
C	-4.8	212.3	256.1	130.4	305.9	1
	"/>
<rect x="0.5" y="0.5" fill="none" stroke="#000000" width="611" height="738.6"/>
</svg>

#endif

void GetSuitPoints(const Card &card, std::vector<Gdiplus::PointF> &one, std::vector<Gdiplus::PointF> &two, double &suitWidth, double &suitHeight)
{
	if (card.GetSuit() == Card::kSpades)
	{
		SetPoints(gSpade1, one, gSpade1Count);
		SetPoints(gSpade2, two, gSpade2Count);
		suitWidth = gSpadeWidth;
		suitHeight = gSpadeHeight;
	}
	else if (card.GetSuit() == Card::kHearts)
	{
		SetPoints(gHeart1, one, gHeart1Count);
		SetPoints(gHeart2, two, gHeart2Count);
		suitWidth = gHeartWidth;
		suitHeight = gHeartHeight;
	}
	else if (card.GetSuit() == Card::kDiamonds)
	{
		SetPoints(gDiamond, one, gDiamondCount);
		suitWidth = gDiamondWidth;
		suitHeight = gDiamondHeight;
		two.clear();
	}
	else
	{
		SetPoints(gClub1, one, gClub1Count);
		SetPoints(gClub2, two, gClub2Count);
		suitWidth = gClubWidth;
		suitHeight = gClubHeight;
	}
}


void DrawSuitCentered(HDC hdc, const Card &card, int32 x, int32 y, uint32 width, bool flip)
{
	std::vector<Gdiplus::PointF> one;
	std::vector<Gdiplus::PointF> two;
	double suitWidth;
	double suitHeight;
	GetSuitPoints(card, one, two, suitWidth, suitHeight);

	Gdiplus::Graphics graphics(hdc);

	// Use antialiasing when drawing.
	graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

	// Set the transform matrix.
	float scaleX = (float)( (double)width / (double)suitWidth );
	if (flip)
		scaleX = -scaleX;

	double height = (double)suitHeight * scaleX;

	float xPos;
	if (flip)
		xPos = (float)((double)x + ((double)width / 2.0));
	else
		xPos = (float)((double)x - ((double)width / 2.0));
	float yPos = (float)((double)y - (height / 2.0));


#if 0
	// Draw the bounding box.
	uint32 intHeight = (uint32)height;
	RECT rect;
	rect.left = x-width/2;
	rect.top = y-intHeight/2;
	rect.right = rect.left + width - 1;
	rect.bottom = rect.top + intHeight - 1;
	SelectObject(hdc, GetStockObject(WHITE_BRUSH));
	DrawRectangle(hdc, rect);
#endif

	Gdiplus::Matrix matrix(scaleX, 0.0, 0.0 , scaleX, xPos, yPos);
	graphics.SetTransform(&matrix);

	Gdiplus::GraphicsPath path;

	uint32 count = (uint32)one.size();
	path.AddBeziers(&one[0], count);

	if (two.size())
		{
		Gdiplus::PointF pointa(one[count-1]);
		Gdiplus::PointF pointb(two[0]);
		path.AddLine(pointa, pointb);

		_ASSERT(count == (uint32)two.size());
		path.AddBeziers(&two[0], count);
	}

	Gdiplus::Color black(0, 0, 0);
	Gdiplus::Color red(255, 0, 0);
	Gdiplus::SolidBrush solidBrush(card.IsRed() ? red : black);
	graphics.FillPath(&solidBrush, &path);

	// Clubs has ellipses as well as beziers.
	if (card.GetSuit() == Card::kClubs)
	{
		Gdiplus::RectF rect;
		rect.X = (float)49.517; rect.Y = (float)0.592; rect.Width = (float)(2.*45.); rect.Height = (float)(2.*45.);
		graphics.FillEllipse(&solidBrush, rect);
		rect.X = (float)0.017; rect.Y = (float)74.81; rect.Width = (float)(2.*45.); rect.Height = (float)(2.*45.);
		graphics.FillEllipse(&solidBrush, rect);
		rect.X = (float)100.017; rect.Y = (float)74.81; rect.Width = (float)(2.*45.); rect.Height = (float)(2.*45.);
		graphics.FillEllipse(&solidBrush, rect);
		rect.X = (float)71.851; rect.Y = (float)80.667; rect.Width = (float)45.5; rect.Height = (float)30.144;
		graphics.FillRectangle(&solidBrush, rect);
	}
}

void SetPoints(MyDoublePoint *sourcePoints, std::vector<Gdiplus::PointF> &destinationPoints, uint32 count)
{
	for (uint32 i = 0; i < count; i++)
	{
		Gdiplus::PointF point((float)sourcePoints[i].x, (float)sourcePoints[i].y);
		destinationPoints.push_back(point);
	}
}

/*
When the game is done, show the under cards.
*/

uint32 gFaceCardResourceID[] = {
	kJackHearts, kJackDiamonds, kJackSpades, kJackClubs,
	kQueenHearts, kQueenDiamonds, kQueenSpades, kQueenClubs,
	kKingHearts, kKingDiamonds, kKingSpades, kKingClubs,
};

bool DrawCardInside(HDC hdc, int32 x, int32 y, uint32 width, uint32 height, const Card &card)
{
	if (card.GetNumber() >= Card::kJack)
	{
		uint32 index = (card.GetNumber() - Card::kJack) * 4 + (card.GetSuit());
		uint32 id = gFaceCardResourceID[index];
		
		// 390 x 552 king.gif
				
		// Draw the image to fit with white transparent.
//		DrawResourceImage(hdc, ResourceID(gInstance, id, _T("GIFFILE")), x, y, width, height, true);
		return false;
	}

	// Draw a big ace of spades.
	if (card.GetSuit() == Card::kSpades && card.GetNumber() == Card::kAce)
	{
		DrawSuitCentered(hdc, card, x+width/2, y+height/2, width-ScaleWidth(width, 15), false);
		return false;
	}

	std::vector<SuitLocation> locations;
	GetSuitLocations(x, y, width, height, card, locations);

	uint32 count = (uint32)locations.size();
	for (uint32 i = 0; i < count; i++)
	{
		DrawSuit(hdc, locations[i].fx, locations[i].fy, width, height, card, false, true, locations[i].fFlip);
	}
	return false; // not done drawing
}

#if 0
int DrawResourceMetafile(HDC hdc, ResourceID resourceID, int32 x, int32 y, uint32 width, uint32 height)
{
	// Read the metafile resource.
	SimpleBuffer simpleBuffer(nil, 0);
	if (Utils::GetResource(resourceID, simpleBuffer))
		return 1;

	// Get a metafile handle from the metafile bytes.
	HENHMETAFILE metafile = SetEnhMetaFileBits(simpleBuffer.ByteCount(), (BYTE *)simpleBuffer.GetPtr());
	if (!metafile)
		return 1;

	// Draw the picture.
	RECT rect;
	SetRect(&rect, x, y, x+width, y+height);
	PlayEnhMetaFile(hdc, metafile, &rect); 

	// Release the metafile handle. 
	DeleteEnhMetaFile(metafile);

	return 0; // success
}
#endif

uint32 ScaleWidth(uint32 width, uint32 number)
{
	return (uint32)(((float)width / 90.0f) * number);
}

class SuitLoc
{
public:
	uint8 xn;
	uint8 xd;
	uint8 yn;
	uint8 yd;
	uint8 flip;
};

#define CountOfArray(arrayItem) (sizeof(arrayItem)/sizeof(arrayItem[0]))

SuitLoc gCardSuitLocation1[] = {1,2,1,2,0};
SuitLoc gCardSuitLocation2[] = {1,2,1,5,0, 1,2,4,5,1};
SuitLoc gCardSuitLocation3[] = {1,2,1,5,0, 1,2,1,2,0, 1,2,4,5,1};
SuitLoc gCardSuitLocation4[] = {1,3,1,5,0, 2,3,1,5,0, 1,3,4,5,1, 2,3,4,5,1};
SuitLoc gCardSuitLocation5[] = {1,3,1,5,0, 2,3,1,5,0, 1,2,1,2,0, 1,3,4,5,1, 2,3,4,5,1};
SuitLoc gCardSuitLocation6[] = {1,3,1,5,0, 2,3,1,5,0, 1,3,1,2,0, 2,3,1,2,0, 1,3,4,5,1, 2,3,4,5,1};
SuitLoc gCardSuitLocation7[] = {1,3,1,5,0, 2,3,1,5,0, 1,2,17,48,0, 1,3,1,2,0, 2,3,2,4,0, 1,3,4,5,1, 2,3,4,5,1};
SuitLoc gCardSuitLocation8[] = {1,3,1,5,0, 2,3,1,5,0, 1,3,2,5,0, 2,3,2,5,0, 1,3,3,5,1, 2,3,3,5,1, 1,3,4,5,1, 2,3,4,5,1};
SuitLoc gCardSuitLocation9[] = {1,3,1,5,0, 2,3,1,5,0, 1,2,1,2,0, 1,3,2,5,0, 2,3,2,5,0, 1,3,3,5,1, 2,3,3,5,1, 1,3,4,5,1, 2,3,4,5,1};
SuitLoc gCardSuitLocation10[] = {1,3,1,5,0, 2,3,1,5,0, 1,3,2,5,0, 2,3,2,5,0, 1,3,3,5,1, 2,3,3,5,1, 1,3,4,5,1, 2,3,4,5,1, 1,2,7,24,0, 1,2,17,24,1};
SuitLoc gCardSuitLocationJack[] = {17,20,3,20,0, 3,20,17,20,1};
SuitLoc gCardSuitLocationQueen[] = {17,20,3,20,0, 3,20,17,20,1};
SuitLoc gCardSuitLocationKing[] = {17,20,3,20,0, 3,20,17,20,1};

SuitLoc *gCardSuitLocations[] = 
{
	gCardSuitLocation1, 
	gCardSuitLocation2, 
	gCardSuitLocation3, 
	gCardSuitLocation4, 
	gCardSuitLocation5, 
	gCardSuitLocation6, 
	gCardSuitLocation7, 
	gCardSuitLocation8, 
	gCardSuitLocation9, 
	gCardSuitLocation10,
	gCardSuitLocationJack,
	gCardSuitLocationQueen,
	gCardSuitLocationKing
};
uint32 gCardSuitLocationCounts[] = 
{
	CountOfArray(gCardSuitLocation1), 
	CountOfArray(gCardSuitLocation2), 
	CountOfArray(gCardSuitLocation3), 
	CountOfArray(gCardSuitLocation4), 
	CountOfArray(gCardSuitLocation5),
	CountOfArray(gCardSuitLocation6),
	CountOfArray(gCardSuitLocation7),
	CountOfArray(gCardSuitLocation8),
	CountOfArray(gCardSuitLocation9),
	CountOfArray(gCardSuitLocation10),
	CountOfArray(gCardSuitLocationJack),
	CountOfArray(gCardSuitLocationQueen),
	CountOfArray(gCardSuitLocationKing)
};

void GetSuitLocations(int32 x, int32 y, uint32 width, uint32 height, const Card &card, std::vector<SuitLocation> &locations)
{
	uint32 number = card.GetNumber();

	uint32 count = gCardSuitLocationCounts[number];
	SuitLoc *suitLocPtr = gCardSuitLocations[number];

	for (uint32 i = 0; i < count; i++)
	{
		SuitLoc suitLoc = suitLocPtr[i];

		int32 xs = x + suitLoc.xn*width/suitLoc.xd;
		int32 ys = y + suitLoc.yn*height/suitLoc.yd;
		locations.push_back(SuitLocation(xs, ys, suitLoc.flip ? true : false));
	}
}


void DrawNumber(HDC hdc, int32 x, int32 y, uint32 width, const Card &card)
{
	TString cardLetter;
	GetCardNumber(card, cardLetter);

	Gdiplus::Color black(0, 0, 0);
	Gdiplus::Color red(255, 0, 0);
	Gdiplus::SolidBrush solidBrush(card.IsRed() ? red : black);

	Gdiplus::Graphics graphics(hdc);
	graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
	Gdiplus::Font font(kFontName, (float)width/9);
	Gdiplus::StringFormat format;

	float letterWidth;
	float letterHeight;
	DrawUtils::MeasureString(graphics, cardLetter.c_str(), font, letterWidth, letterHeight);

	Gdiplus::PointF point((float)x-(letterWidth/2), (float)y);
	graphics.DrawString(cardLetter.c_str(), (int)cardLetter.size(), &font, point, &format, &solidBrush);

}

//	enum Suit {kHearts, kDiamonds, kSpades, kClubs};
uint32 gSuitToResourceID[] = {kSmallHeart, kSmallDiamond, kSmallSpade, kSmallClub};
uint32 gSuitToResourceIDBig[] = {kBigHeart, kBigDiamond, kBigSpade, kBigClub};

int DrawSuit(HDC hdc, int32 x, int32 y, uint32 width, uint32 height, const Card &card, bool drawSmall, bool drawCentered, bool flip)
{
	uint32 diameter;
	if (drawSmall)
		diameter = ScaleWidth(width, 10);
	else
		diameter = ScaleWidth(width, 17);
	DrawSuitCentered(hdc, card, x, y, diameter, flip);
	return 0;
}

int DrawResourceImage(HDC hdc, const ResourceID &resourceID, int32 x, int32 y, uint32 width, bool flip)
{
	// Create a stream from the logo resource.
	IStream *stream = Utils::CreateStreamFromResource(resourceID);
	if (!stream)
		return 1; // missing resource
	AutoRelease<IStream> autoRelease(stream);

	Gdiplus::Image image(stream, false);

	Gdiplus::Graphics graphics(hdc);

	uint32 imageWidth = ScaleWidth(width, image.GetWidth());
	uint32 imageHeight = ScaleWidth(width, image.GetHeight());

	if (flip)
	{
		x += imageWidth;
		y += imageHeight;
	}

	Gdiplus::Point pt;
	pt.X = x;
	pt.Y = y;

	if (flip)
	{
		graphics.RotateTransform(180.0);
		graphics.TransformPoints(Gdiplus::CoordinateSpaceWorld, Gdiplus::CoordinateSpacePage, &pt, 1);
	}

	Gdiplus::Rect rect(pt.X, pt.Y, imageWidth, imageHeight);
	if (graphics.DrawImage(&image, rect))
		return 1;

	return 0; // success
}

int DrawResourceImage(HDC hdc, const ResourceID &resourceID, int32 x, int32 y, uint32 width, uint32 height, bool whiteTransparent)
{
	// Create a stream from the logo resource.
	IStream *stream = Utils::CreateStreamFromResource(resourceID);
	if (!stream)
		return 1; // missing resource
	AutoRelease<IStream> autoRelease(stream);

	Gdiplus::Image image(stream, false);

	Gdiplus::Graphics graphics(hdc);

	// Make white transparent.
	Gdiplus::ImageAttributes imageAttributes;
	if (whiteTransparent)
		imageAttributes.SetColorKey(Gdiplus::Color(250, 250, 250), Gdiplus::Color(255, 255, 255), Gdiplus::ColorAdjustTypeBitmap);

	Gdiplus::Point pt;
	pt.X = x;
	pt.Y = y;

	if (graphics.DrawImage(&image, Gdiplus::Rect(pt.X, pt.Y, width, height), 0, 0, image.GetWidth(), image.GetHeight(), 
		Gdiplus::UnitPixel, &imageAttributes))
		return 1;

	return 0; // success
}

// Draw a round rect with white corners.
void DrawRoundRect(HDC hdc, const RECT &rect, int32 corner, bool drawCorners)
{
	RECT temp = rect;
	temp.bottom -= 1;
	temp.right -= 1;

//	if (drawCorners)
	{
		HRGN oldRegion = CreateRectRgn(0, 0, 0, 0);
		int rc = GetClipRgn(hdc, oldRegion);

		HRGN roundRectRegion = CreateRoundRectRgn(temp.left, temp.top, temp.right, temp.bottom, corner, corner);
		ExtSelectClipRgn(hdc, roundRectRegion, RGN_DIFF);

		// Draw the corners.
		FillRect(hdc, &rect, (HBRUSH)GetStockObject(LTGRAY_BRUSH)); 

		// If there was an existing clip region, put it back.
		if (rc == 1)
			SelectClipRgn(hdc, oldRegion);
		else
			SelectClipRgn(hdc, NULL);

		DeleteObject(oldRegion);
		DeleteObject(roundRectRegion);
	}

	RoundRect(hdc, temp.left, temp.top, temp.right, temp.bottom, corner, corner);
}

void DrawCardBack(HDC hdc, const RECT &rect)
{
	SetBkMode(hdc, OPAQUE);

	SetBrushOrgEx(hdc, rect.left, rect.top, NULL);

	MyBrush myBrush(hdc, RGB(0, 0, 255), HS_FDIAGONAL);
	DrawRoundRect(hdc, rect, ScaleWidth(rect.right - rect.left, 9), false);
}

void DrawEmptyCard(HDC hdc, const RECT &rect)
{
	SetBkMode(hdc, OPAQUE);

	LOGBRUSH logBrush;
	logBrush.lbStyle = BS_SOLID;
	logBrush.lbColor = RGB(0, 0, 0);
	logBrush.lbHatch = 0;
	HPEN pen = ExtCreatePen(PS_GEOMETRIC | PS_DASH | PS_JOIN_ROUND, 2, &logBrush, 0, NULL);
	MySelectObject selectPen(hdc, pen);

	MyBrush myBrush(hdc, RGB(200, 200, 200), HS_DIAGCROSS);

	DrawRectangle(hdc, rect);
}

void DrawRectangle(HDC hdc, const RECT &rect)
{
	Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
}

#if 0
//	DrawResource(hdc, x, y, ResourceID(gInstance, IDR_PNGFILEEMPTYPILE, _T("PNGFILE")));

void DrawResource(HDC hdc, int32 x, int32 y, const ResourceID &resourceID)
{
	// Create a stream from the logo resource.
	IStream *stream = Utils::CreateStreamFromResource(resourceID);
	if (!stream)
		return; // No resource.
	AutoRelease<IStream> autoRelease(stream);

	Gdiplus::Image image(stream, false);

	// Get the resource's width and height;
	int32 width = image.GetWidth();
	int32 height = image.GetHeight();

//	_ASSERT(width == gCardWidth);
//	_ASSERT(height == gCardHeight);

	Gdiplus::Graphics graphics(hdc);
	graphics.DrawImage(&image, x, y);
}
#endif

bool Game::GetHitCard(const std::vector<VisibleRectangle> &visibleRectangles, int32 x, int32 y, VisibleRectangle &hitCard) const
{
	uint32 count = (uint32)visibleRectangles.size();
	for (uint32 i = 0; i < count; i++)
	{
		if (IsOnCard(visibleRectangles[i], x, y))
		{
			hitCard = visibleRectangles[i];
			return true; // hit card
		}
	}
	return false; // not found
}

bool Game::ClickOnAceSpades(int32 x, int32 y)
{
	std::vector<VisibleRectangle> visibleRectangles;
	GetVisibleRectangles(0, 0, visibleRectangles);

	VisibleRectangle hitRectangle;
	if (!GetHitCard(visibleRectangles, x, y, hitRectangle))
		return false; // nothing hit
	
	Card card;
	if (fState.GetCard(hitRectangle.fPileNumber, -1, card))
		return false;

	if (card.GetNumber() == Card::kAce && card.GetSuit() == Card::kSpades)
		return true;

	return false;
}

void Game::RightMouseDown(int32 x, int32 y)
{
	// Right clicking on the ace of spades creates the big card window.
	if (ClickOnAceSpades(x, y))
	{
		ShowWindow(gStatsWindow, SW_HIDE);
		BigCardWindow::ShowBigCardWindow();
	}
}

bool Game::MouseDoubleClick(int32 x, int32 y)
{
	// Check for a move to the foundation.

	// Get a list of cards that we can drag from.
	std::vector<VisibleRectangle> sourceRectangles;
	GetSourceRectangles(0, 0, sourceRectangles);

	// Determine whether we clicked on one of the source cards.
	if (!GetHitCard(sourceRectangles, x, y, fSourceRectangle))
		return false; // not handled, card not hit

	if (fSourceRectangle.fPileNumber == kWastePile ||
		(fSourceRectangle.fPileNumber >= kFaceUp0 && fSourceRectangle.fPileNumber <= kFaceUp6 && fSourceRectangle.fTopOfPile))
	{
		PileNumber foundationPileNumber;
		if (fState.IsAnyFoundationMove(fSourceRectangle.fPileNumber, -1, foundationPileNumber))
		{
			PossibleMove possibleMove(fState, fSourceRectangle.fPileNumber, -1, foundationPileNumber);
			fState.MakeMove(possibleMove);
			Invalidate();
			return true; // handled the double click
		}
	}
	return false; // not handled
}

void Game::MouseDown(int32 x, int32 y)
{
	if (fMouseDown)
		return;

	// Get a list of cards that we can drag from.
	std::vector<VisibleRectangle> sourceRectangles;
	GetSourceRectangles(0, 0, sourceRectangles);

	// Determine whether we clicked on one of the source cards.
	if (!GetHitCard(sourceRectangles, x, y, fSourceRectangle))
		return; // card not hit

	if (FlipCard())
		return;

	// Get the offset to the upper left hand corner of the card.
	RECT rect = GetCardRect(fSourceRectangle);
	fdx = x - rect.left;
	fdy = y - rect.top;

	fState.PickUpCards(fSourceRectangle.fPileNumber, fSourceRectangle.fPosition);

	class DrawGameFunction : public IDrawFunction
	{
	public:
		DrawGameFunction(Game &game, int32 x, int32 y) : fGame(game), fx(x), fy(y) {}
		~DrawGameFunction() {}

		int Draw(HDC hdc) const
		{
			fGame.DrawGame(hdc, -fx, -fy, false);
			return 0;
		}
	private:
		Game &fGame;
		int32 fx;
		int32 fy;
	} drawGameFunction(*this, rect.left, rect.top);

	if (fDragBits.GetBackground(rect.left, rect.top, rect.right-rect.left, GetPickUpHeight(), drawGameFunction))
		return;

	// Draw the game without the cards to save the background.
	class DrawPileFunction : public IDrawFunction
	{
	public:
		DrawPileFunction(Game &game, int32 x, int32 y) : fGame(game), fx(x), fy(y) {}
		~DrawPileFunction() {}
		int Draw(HDC hdc) const
		{
			fGame.DrawPickUpPile(hdc, fx, fy);
			return 0;
		}
	private:
		Game &fGame;
		int32 fx;
		int32 fy;
	} drawPileFunction(*this, 0, 0);

	// Draw the pile to save the foreground.
	if (fDragBits.GetForeground(drawPileFunction))
		return;

	fRectangleDown = false;

	// Capture the mouse.
	SetCapture(gMainWindow);
	fMouseDown = true;
}

void Game::MouseMove(int32 x, int32 y)
{
	if (!fMouseDown)
		return;

	AutoReleaseDC autoReleaseDC;
	HDC hdc = autoReleaseDC.GetDC(gMainWindow);

	if (fRectangleDown)
	{
		// Erase the down rectangle.
		DrawHitRectangle(hdc, fDownRectangle, fDownRectangleClip);
		fRectangleDown = false;
	}

	// Drag the card pile to the new location.
	fDragBits.MouseMove(hdc, x-fdx, y-fdy);

	VisibleRectangle dropSpot;
	if (IsValidDropSpot(x, y, dropSpot))
	{
		// Draw the down rectangle.
		fDownRectangle = GetCardRect(dropSpot);
		SetRect(&fDownRectangleClip, x-fdx, y-fdy, x-fdx+gCardWidth, y-fdy+gCardHeight);
		DrawHitRectangle(hdc, fDownRectangle, fDownRectangleClip);
		fRectangleDown = true;
	}
}

// Determine whether we are over a valid drop spot.
bool Game::IsValidDropSpot(int32 x, int32 y, VisibleRectangle &dropSpot) const
{
	// Get a list of cards we can drag to.
	std::vector<VisibleRectangle> destinationRectangles;
	GetDestinationRectangles(0, 0, destinationRectangles);

	uint32 count = (uint32)destinationRectangles.size();
	for (uint32 i = 0; i < count; i++)
	{
		// Cannot drop on itself.
		if (fSourceRectangle.fPileNumber == destinationRectangles[i].fPileNumber)
			continue;

		if (IsCardOnCard(destinationRectangles[i], x, y))
		{
			if (fState.IsDragMove(destinationRectangles[i].fPileNumber))
			{
				dropSpot = destinationRectangles[i];
				return true;
			}
		}
	}

	return false;
}

bool Game::IsCardOnCard(const VisibleRectangle &visibleRectangle, int32 x, int32 y) const
{
	if (IsOnCard(visibleRectangle, x-fdx, y-fdy))
		return true;
	if (IsOnCard(visibleRectangle, x-fdx+gCardWidth, y-fdy))
		return true;
	if (IsOnCard(visibleRectangle, x-fdx, y-fdy+gCardHeight))
		return true;
	if (IsOnCard(visibleRectangle, x-fdx+gCardWidth, y-fdy+gCardHeight))
		return true;

	return false;
}


// Draw a rectangle so it can be erased by drawing over it a second time.
void DrawHitRectangle(HDC hdc, const RECT &rect, const RECT &rectClip)
{
//	SetBkMode(hdc, TRANSPARENT);
	SetROP2(hdc, R2_XORPEN);
	SelectObject(hdc, GetStockObject(NULL_BRUSH));

	LOGPEN logPen;
	logPen.lopnStyle = PS_SOLID;
	POINT pt;
	pt.x = 1;
	logPen.lopnWidth = pt;
	logPen.lopnColor = RGB(255, 255, 255);
	HPEN pen = CreatePenIndirect(&logPen);
	MySelectObject selectPen(hdc, pen);

	RECT hitRect = rect;
	InflateRect(&hitRect, -3, -3);

	ExcludeClipRect(hdc, rectClip.left, rectClip.top, rectClip.right, rectClip.bottom);

	int32 roundCorner = ScaleWidth(hitRect.right - hitRect.left, 9);
	RoundRect(hdc, hitRect.left, hitRect.top, hitRect.right-1, hitRect.bottom-1, roundCorner, roundCorner);

	SelectClipRgn(hdc, NULL);
}


void Game::MouseUp(int32 x, int32 y)
{
	if (!fMouseDown)
		return;

	VisibleRectangle dropSpot;
	bool validSpot = IsValidDropSpot(x, y, dropSpot);

	// Draw the card either at its new spot or back at its original spot.
	int32 drawX;
	int32 drawY;
	if (validSpot)
	{
		GetNewSpot(dropSpot, drawX, drawY);
	}
	else
	{
		// Get the original top left of the pile.
		RECT drawSpot = GetCardRect(fSourceRectangle);
		drawX = drawSpot.left;
		drawY = drawSpot.top;
	}
	AutoReleaseDC autoReleaseDC;
	HDC hdc = autoReleaseDC.GetDC(gMainWindow);

	if (fRectangleDown)
	{
		// Erase the down rectangle.
		DrawHitRectangle(hdc, fDownRectangle, fDownRectangleClip);
		fRectangleDown = false;
	}

	fDragBits.MouseMove(hdc, drawX, drawY);

	fState.PutBackCards();

	if (validSpot)
	{
		PossibleMove possibleMove(fState, fSourceRectangle.fPileNumber, fSourceRectangle.fPosition, dropSpot.fPileNumber);
		fState.MakeMove(possibleMove);
	}

	ReleaseCapture();
	fMouseDown = false;
//	Invalidate();
}

void Game::GetNewSpot(const VisibleRectangle &dropSpot, int32 &x, int32 &y) const
{
	RECT drawSpot = GetCardRect(dropSpot);
	x = drawSpot.left;
	y = drawSpot.top;

	if (dropSpot.fPileNumber < kFoundation0)
	{
		if (!fState.IsPileEmpty(dropSpot.fPileNumber))
			y += gOverlap;
	}
}

#if 0
void Game::DrawPickUpPileAnnimated(HDC hdc, int32 fromX, int32 fromY, int32 toX, int32 toY, bool annimate)
{
	std::vector<POINT> pts;
	if (annimate)
	{
		std::vector<POINT> allPts;
		GetPointsBetween(fromX, fromY, toX, toY, allPts);

		uint32 count = (uint32)allPts.size();

		uint32 step = count / 8;
		if (step)
		{
			for (uint32 i = step; i < count; i += step)
				pts.push_back(allPts[i]);
		}
	}

	POINT pt;
	pt.x = toX;
	pt.y = toY;
	pts.push_back(pt);

	uint32 count = (uint32)pts.size();
	for (uint32 i = 0; i < count; i += 1)
	{
		fDragBits.MouseMove(hdc, pts[i].x, pts[i].y);
		Sleep(100);

//		DrawPickUpPile(hdc, pts[i].x, pts[i].y);
	}
}

VOID CALLBACK LineFunction(int x, int y, LPARAM lpData)
{
	std::vector<POINT> *pts = (std::vector<POINT> *)lpData;
	POINT pt;
	pt.x = x;
	pt.y = y;
	pts->push_back(pt);
}

void GetPointsBetween(int32 x, int32 y, int32 x2, int32 y2, std::vector<POINT> &pts)
{
	LineDDA(x, y, x2, y2, LineFunction, (LPARAM)&pts);
}
#endif

void Game::GetSourceRectangles(int32 x, int32 y, std::vector<VisibleRectangle> &sourceRectangles)
{
	std::vector<VisibleRectangle> visibleRectangles;
	GetVisibleRectangles(x, y, visibleRectangles);

	// Create a list of all the source piles that we can drag from.
	for (int32 i = 0; i < (int32)visibleRectangles.size(); i++)
	{
		VisibleRectangle visibleRectangle = visibleRectangles[i];
		if (UseRectangle(visibleRectangle))
			sourceRectangles.push_back(visibleRectangle);
	}
}

bool Game::UseRectangle(const VisibleRectangle &visibleRectangle) const
{
	PileNumber pileNumber = visibleRectangle.fPileNumber;

	if (pileNumber == kDeck)
		return true;
	
	if (fState.IsPileEmpty(visibleRectangle.fPileNumber))
		return false;

	if (pileNumber == kWastePile)
		return true;

	if (pileNumber >= kFaceDown0 && pileNumber <= kFaceDown6 && visibleRectangle.fTopOfPile)
		return true;

	if (fState.IsPartialMoveAllowed())
	{
		if (pileNumber >= kFaceUp0 && pileNumber <= kFaceUp6)
			return true;
	}
	else
	{
		if (pileNumber >= kFaceUp0 && pileNumber <= kFaceUp6 && (visibleRectangle.fPosition == 0 || visibleRectangle.fTopOfPile))
			return true;
	}

	return false;
}

// Get a list of cards that we can drag to.
void Game::GetDestinationRectangles(int32 x, int32 y, std::vector<VisibleRectangle> &destinationRectangles) const
{
	std::vector<VisibleRectangle> visibleRectangles;
	GetVisibleRectangles(x, y, visibleRectangles);

	// Create a list of all the destination piles that we can drag to.
	for (int32 i = 0; i < (int32)visibleRectangles.size(); i++)
	{
		VisibleRectangle visibleRectangle = visibleRectangles[i];
		PileNumber pileNumber = visibleRectangle.fPileNumber;
		if (pileNumber >= kFoundation0 && pileNumber <= kFoundation3)
			destinationRectangles.push_back(visibleRectangle);

		if (pileNumber >= kFaceUp0 && pileNumber <= kFaceUp6)
		{
			if (visibleRectangle.fTopOfPile || visibleRectangle.fEmptySpot)
				destinationRectangles.push_back(visibleRectangle);
		}
	}
}

void Game::Invalidate()
{
	InvalidateRect(gMainWindow, NULL, false);
	gStats.RunningTotal(fState.GetCardsUp());
}

bool Game::FlipCard()
{
	if (fSourceRectangle.fPileNumber == kDeck)
	{
		if (fState.IsPileEmpty(kDeck))
		{
			// Determine whether we can flip the waste pile to make a new stock.
			if (!fState.IsPileEmpty(kWastePile))
			{
				if (fState.IsWastePileFlipAllowed())
				{
					PossibleMove possibleMove(fState, kWastePile, 0, kDeck);
					fState.MakeMove(possibleMove);
					Invalidate();
				}
			}
		}
		else
		{
			PossibleMove possibleMove(fState, fSourceRectangle.fPileNumber, fSourceRectangle.fPosition, kWastePile);
			fState.MakeMove(possibleMove);
			Invalidate();
		}
		return true;
	}
	if (fSourceRectangle.fPileNumber >= kFaceDown0 && fSourceRectangle.fPileNumber <= kFaceDown6)
	{
		// If top face down card and the face up pile is empty, flip the card.
		if (fSourceRectangle.fTopOfPile) 
		{
			PossibleMove possibleMove(fState, fSourceRectangle.fPileNumber, fSourceRectangle.fPosition, PILENUMBER(fSourceRectangle.fPileNumber+7));
			fState.MakeMove(possibleMove);
			Invalidate();
			return true;
		}
	}
	return false;
}

// Draw a bitmap on the screen.
int DrawBitmap(HDC hdc, HBITMAP bitmap, int x, int y)
{
	HDC hdcMemory = MyCreateCompatibleDC();
	if (!hdcMemory)
		return 1;
	AutoDeleteDC autoDeleteDC(hdcMemory);

	// Select the bitmap into the memory hdc. 
	// Now drawing to the memory dc, draws on the bitmap.
	HBITMAP oldBitmap = (HBITMAP)SelectObject(hdcMemory, bitmap);
	if (!oldBitmap)
		return 1;
	// Automatically select the old bitmap when going out of scope.
	AutoSelectObject autoSelectObject(hdcMemory, oldBitmap);

	BITMAP bm;
	if (GetObject(bitmap, sizeof(BITMAP), &bm) == 0)
		return 1;

	if (BitBlt(hdc, x, y, bm.bmWidth, bm.bmHeight, hdcMemory, 0, 0, SRCCOPY) == 0)
		return 1;

	return 0;
}

HDC MyCreateCompatibleDC()
{
	AutoReleaseDC autoReleaseDC;
	HDC hdc = autoReleaseDC.GetDC(gMainWindow);

	HDC compatibleHdc = CreateCompatibleDC(hdc);

	// Set the mode so rectangles draw the same.
	SetGraphicsMode(compatibleHdc, GM_ADVANCED);

	return compatibleHdc;
}

/** Draw the source bitmap to the working bitmap at the given location.
	@param wx,wy is the upper left of the working area.
	@param sourceBitmap is the bits to draw to the working area.
	@param x,y is the upper left of the location to put the bits.
	(wx,wy) and (x,y) are in the same coordination system.
*/
int DragBits::DrawToWorking(int32 wx, int32 wy, HBITMAP sourceBitmap, int32 x, int32 y)
{
	HDC hdcSource = MyCreateCompatibleDC();
	if (!hdcSource)
		return 1;
	AutoDeleteDC autoDeleteSource(hdcSource);

	// Select the source bitmap into the memory hdc. 
	// Now drawing to the memory dc, draws on the bitmap.
	HBITMAP oldSourceBitmap = (HBITMAP)SelectObject(hdcSource, sourceBitmap);
	if (!oldSourceBitmap)
		return 1;
	// Automatically select the old bitmap when going out of scope.
	AutoSelectObject autoSelectSource(hdcSource, oldSourceBitmap);
	BITMAP source;
	if (GetObject(sourceBitmap, sizeof(BITMAP), &source) == 0)
		return 1;

	HDC hdcDestination = MyCreateCompatibleDC();
	if (!hdcDestination)
		return 1;
	AutoDeleteDC autoDeleteDestination(hdcDestination);

	// Select the source bitmap into the memory hdc. 
	// Now drawing to the memory dc, draws on the bitmap.
	HBITMAP oldDestinationBitmap = (HBITMAP)SelectObject(hdcDestination, fWorking);
	if (!oldDestinationBitmap)
		return 1;
	// Automatically select the old bitmap when going out of scope.
	AutoSelectObject autoSelectDestination(hdcDestination, oldDestinationBitmap);

	BITMAP destination;
	if (GetObject(fWorking, sizeof(BITMAP), &destination) == 0)
		return 1;

	if (BitBlt(hdcDestination, x-wx, y-wy, source.bmWidth, source.bmHeight, hdcSource, 0, 0, SRCCOPY) == 0)
		return 1;

	return 0;

}

/** Grab the bits from the working area at the given location and copy them to the given bitmap.
	@param wx,wy is the upper left of the working area.
	@param destinationBitmap where the bits are copied to.
	@param x,y is the upper left of the bits.
	(wx,wy) and (x,y) are in the same coordination system.
*/
int DragBits::GrabWorkingBits(int32 wx, int32 wy, HBITMAP destinationBitmap, int32 x, int32 y)
{
	HDC hdcSource = MyCreateCompatibleDC();
	if (!hdcSource)
		return 1;
	AutoDeleteDC autoDeleteSource(hdcSource);

	// Select the source bitmap into the memory hdc. 
	// Now drawing to the memory dc, draws on the bitmap.
	HBITMAP oldSourceBitmap = (HBITMAP)SelectObject(hdcSource, fWorking);
	if (!oldSourceBitmap)
		return 1;
	// Automatically select the old bitmap when going out of scope.
	AutoSelectObject autoSelectSource(hdcSource, oldSourceBitmap);
	BITMAP source;
	if (GetObject(fWorking, sizeof(BITMAP), &source) == 0)
		return 1;

	HDC hdcDestination = MyCreateCompatibleDC();
	if (!hdcDestination)
		return 1;
	AutoDeleteDC autoDeleteDestination(hdcDestination);

	// Select the source bitmap into the memory hdc. 
	// Now drawing to the memory dc, draws on the bitmap.
	HBITMAP oldDestinationBitmap = (HBITMAP)SelectObject(hdcDestination, destinationBitmap);
	if (!oldDestinationBitmap)
		return 1;
	// Automatically select the old bitmap when going out of scope.
	AutoSelectObject autoSelectDestination(hdcDestination, oldDestinationBitmap);

	BITMAP destination;
	if (GetObject(destinationBitmap, sizeof(BITMAP), &destination) == 0)
		return 1;

	if (BitBlt(hdcDestination, 0, 0, destination.bmWidth, destination.bmHeight, hdcSource, x-wx, y-wy, SRCCOPY) == 0)
		return 1;

	return 0;
}

/** Grab the bits from the screen.
	@param hdc is the screen.
	@param destination is the bitmap to write to.
	@param x, y is the upper left screen location.
*/
int DragBits::GrabScreen(HDC hdc, HBITMAP destinationBitmap, int32 x, int32 y)
{
	HDC hdcDestination = MyCreateCompatibleDC();
	if (!hdcDestination)
		return 1;
	AutoDeleteDC autoDeleteDestination(hdcDestination);

	// Select the source bitmap into the memory hdc. 
	// Now drawing to the memory dc, draws on the bitmap.
	HBITMAP oldDestinationBitmap = (HBITMAP)SelectObject(hdcDestination, destinationBitmap);
	if (!oldDestinationBitmap)
		return 1;
	// Automatically select the old bitmap when going out of scope.
	AutoSelectObject autoSelectDestination(hdcDestination, oldDestinationBitmap);

	BITMAP destination;
	if (GetObject(fWorking, sizeof(BITMAP), &destination) == 0)
		return 1;

	if (BitBlt(hdcDestination, 0, 0, destination.bmWidth, destination.bmHeight, hdc, x, y, SRCCOPY) == 0)
		return 1;

	return 0; // success
}

int DragBits::GetBackground(int32 x, int32 y, uint32 width, uint32 height, IDrawFunction &drawFunction)
{
	if (CreateBackgroundBits(x, y, width, height))
		return 1;

	HDC hdcDestination = MyCreateCompatibleDC();
	if (!hdcDestination)
		return 1;
	AutoDeleteDC autoDeleteDestination(hdcDestination);

	// Select the source bitmap into the memory hdc. 
	// Now drawing to the memory dc, draws on the bitmap.
	HBITMAP oldDestinationBitmap = (HBITMAP)SelectObject(hdcDestination, fBackground);
	if (!oldDestinationBitmap)
		return 1;
	// Automatically select the old bitmap when going out of scope.
	AutoSelectObject autoSelectDestination(hdcDestination, oldDestinationBitmap);

	return drawFunction.Draw(hdcDestination);
}

int DragBits::CreateBackgroundBits(int32 x, int32 y, uint32 width, uint32 height)
{
	if (fBackground)
		DeleteObject(fBackground);

	fx0 = x;
	fy0 = y;
	fWidth = width;
	fHeight = height;

	AutoReleaseDC autoReleaseDC;
	HDC hdc = autoReleaseDC.GetDC(gMainWindow);

	fBackground = CreateCompatibleBitmap(hdc, fWidth, fHeight);
	if (!fBackground)
		return 1;

	fWorkingWidth = width * 2;
	fWorkingHeight = height * 2;

	// Create the working area bitmap.
	if (fWorking)
		DeleteObject(fWorking);
	fWorking = CreateCompatibleBitmap(hdc, fWorkingWidth, fWorkingHeight);
	if (!fWorking)
		return 1;

	return 0; // success
}

int DragBits::GetForeground(IDrawFunction &drawFunction)
{
	if (CreateForegroundBits())
		return 1;

	return DrawToBitmap(fForeground, drawFunction);
}

int DrawToBitmap(HBITMAP bitmap, IDrawFunction &drawFunction)
{
	HDC hdcDestination = MyCreateCompatibleDC();
	if (!hdcDestination)
		return 1;
	AutoDeleteDC autoDeleteDestination(hdcDestination);

	// Select the source bitmap into the memory hdc. 
	// Now drawing to the memory dc, draws on the bitmap.
	HBITMAP oldDestinationBitmap = (HBITMAP)SelectObject(hdcDestination, bitmap);
	if (!oldDestinationBitmap)
		return 1;
	// Automatically select the old bitmap when going out of scope.
	AutoSelectObject autoSelectDestination(hdcDestination, oldDestinationBitmap);

	return drawFunction.Draw(hdcDestination);
}

int DragBits::CreateForegroundBits()
{
	if (!fBackground || !fWorking)
		return 1; // call CreateBackgroundBits first.

	if (fForeground)
		DeleteObject(fForeground);

	AutoReleaseDC autoReleaseDC;
	HDC hdc = autoReleaseDC.GetDC(gMainWindow);

	fForeground = CreateCompatibleBitmap(hdc, fWidth, fHeight);
	if (!fForeground)
		return 1;

	return 0; // success
}

DragBits::DragBits() : fBackground(NULL), fForeground(NULL) 
{

}

DragBits::~DragBits()
{
	if (fBackground)
		DeleteObject(fBackground);

	if (fForeground)
		DeleteObject(fForeground);

	if (fWorking)
		DeleteObject(fWorking);
}

int DragBits::MouseMove(HDC hdc, int32 x, int32 y)
{
	// x,y is the new upper left of the card. fx0, fy0 is the old upper left.

	if (!fBackground || !fWorking)
		return 1; // call GrabBackgroundBits first.
	if (!fForeground)
		return 1; // call GrabForegroundBits first.

	RECT rect;
	RECT rect1;
	RECT rect2;
	SetRect(&rect1, fx0, fy0, fx0+fWidth, fy0+fHeight);
	SetRect(&rect2, x, y, x+fWidth, y+fHeight);

	// If the old card and new card position do not overlap, do this.
	if (IntersectRect(&rect, &rect1, &rect2) == 0)
	{
		// Draw the background area to the screen.
		DrawBitmap(hdc, fBackground, fx0, fy0);

		// Grab the new background from the screen.
		GrabScreen(hdc, fBackground, x, y);

		// Draw the foreground to the screen.
		DrawBitmap(hdc, fForeground, x, y);
	}
	else
	{
		// Get the origin of the working area and it's width and height.
		int32 wx = min(x, fx0);
		int32 wy = min(y, fy0);
		uint32 wWidth;
		if (fx0 > x)
			wWidth = fx0 - x + fWidth;
		else
			wWidth = x - fx0 + fWidth;
		uint32 wHeight;
		if (fy0 > y)
			wHeight = fy0 - y + fHeight;
		else
			wHeight = y - fy0 + fHeight;

		// If the new position is too far away from the old position, do something else.
		if (wWidth > fWorkingWidth)
			return 1;
		if (wHeight > fWorkingHeight)
			return 1;

		// Grab the working area from the screen.
		GrabScreen(hdc, fWorking, wx, wy);

		// Draw the background to the working area.
		DrawToWorking(wx, wy, fBackground, fx0, fy0);

		// Grab the new background from the working area.
		GrabWorkingBits(wx, wy, fBackground, x, y);

		// Draw the foreground to the working area.
		DrawToWorking(wx, wy, fForeground, x, y);

		// Draw the working area to the screen.
		DrawBitmap(hdc, fWorking, wx, wy);
	}

	fx0 = x;
	fy0 = y;

	return 0;
}

void GameLabel::GetBoundingBox(HDC hdc, Gdiplus::Font &font, RECT &boundingBox, TString &string) const
{
	Gdiplus::Graphics graphics(hdc);

	string = Utils::GetString(gInstance, fID);

	float width;
	float height;
	DrawUtils::MeasureString(graphics, string.c_str(), font, width, height);

	boundingBox.left = fx;
	boundingBox.right = fx + (uint32)width;
	boundingBox.top = fy - (uint32)height;
	boundingBox.bottom = fy;
}
