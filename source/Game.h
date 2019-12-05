// Copyright 2007-2019 Flenniken
// All Rights Reserved.

#pragma once

#include "GameState.h"
#include "Card.h"
#include "DragBits.h"
#include "VisibleRectangle.h"
#include "DrawUtils.h"

class GameLabel;

class Game
{
public:
	Game();
	~Game();

	int OneTimeSettup();
	void Play();
	uint32 PlayHand(uint32 drawCount, bool &userStoppedGame);
	void Move();
	void PlayUntilWin();
	void PlayUntil();
	void RedealSameDeck();
	void QuitGame();
	void Options();

	// DrawGame.cpp
	void GetVisibleRectangles(int32 x, int32 y, std::vector<VisibleRectangle> &visibleRectangles) const;
	void DrawGame(HDC hdc, int32 x, int32 y, bool drawlabels) const;
	void MouseDown(int32 x, int32 y);
	void RightMouseDown(int32 x, int32 y);
	void MouseMove(int32 x, int32 y);
	void MouseUp(int32 x, int32 y);
	bool MouseDoubleClick(int32 x, int32 y);
	RECT GetCardRect(const VisibleRectangle &visibleRectangle) const;
	RECT GetVisibleRect(const VisibleRectangle &visibleRectangle) const;
	int32 GetVisibleOverlap(const VisibleRectangle &visibleRectangle) const;
	bool IsOnCard(const VisibleRectangle &visibleRectangle, int32 x, int32 y) const;

	void SetOptions(bool &dealOneAtATime, bool &allowPartialPileMoves, bool &oneTimeThrough);

	void Undo();

	int ResizeCard(int32 direction);
	void ReadCardSize();
	void WriteCardSize();

private:
	void GetVisibleRectangles(int32 x, int32 y, std::vector<VisibleRectangle> &visibleRectangles, std::vector<GameLabel> &gameLabels) const;
	int PickMove(const std::vector<PossibleMove> &moveList, PossibleMove &move);
	bool UseRectangle(const VisibleRectangle &visibleRectangle) const;

	// DrawGame.cpp
	void DrawTopCard(HDC hdc, const VisibleRectangle &visibleRectangle) const;
	void DrawCardBack(HDC hdc, const VisibleRectangle &visibleRectangle) const;
	void DrawCard(HDC hdc, const VisibleRectangle &visibleRectangle) const;
	void DrawVisibleRectangle(HDC hdc, const VisibleRectangle &visibleRectangle) const;
	void DrawPartCardBack(HDC hdc, const VisibleRectangle &visibleRectangle) const;
	void DrawPartCard(HDC hdc, const VisibleRectangle &visibleRectangle) const;
	void DrawGameNow();
	void DrawCard(HDC hdc, const VisibleRectangle &visibleRectangle, int32 x, int32 y) const;
	bool GetHitCard(const std::vector<VisibleRectangle> &visibleRectangles, int32 x, int32 y, VisibleRectangle &hitCard) const;
	void GetSourceRectangles(int32 x, int32 y, std::vector<VisibleRectangle> &sourceRectangles);
	void GetDestinationRectangles(int32 x, int32 y, std::vector<VisibleRectangle> &destinationRectangles) const;
	void Invalidate();
	bool FlipCard();
	void DrawPickUpPile(HDC hdc, int32 x, int32 y) const;
	uint32 GetPickUpHeight();
	void DrawEraseHitRectangle(HDC hdc, int32 x, int32 y);
	bool IsValidDropSpot(int32 x, int32 y, VisibleRectangle &destinationRectangle) const;
	bool IsCardOnCard(const VisibleRectangle &visibleRectangle, int32 x, int32 y) const;
	void GetNewSpot(const VisibleRectangle &dropSpot, int32 &x, int32 &y) const;
	void PlayXGames(uint32 number, uint32 type);
	int CreateFaceCardBitmap();
	void EraseBackground(HDC hdc, const std::vector<VisibleRectangle> &visibleRectangles) const;
	void ResizeTheCard(int32 direction);
	bool ClickOnAceSpades(int32 x, int32 y);
	void DrawCard(HDC hdc, int32 x, int32 y, const Card &card) const;
	int ResizeCard(uint32 width, uint32 height);

	GameState fState;

	// GameMove.cpp
	std::vector<PossibleMove> fMoveList;

	// DrawGame.cpp
	bool fMouseDown;
	int32 fdx;
	int32 fdy;
	VisibleRectangle fSourceRectangle;
	DragBits fDragBits;
	bool fRectangleDown;
	RECT fDownRectangle;
	RECT fDownRectangleClip;
	HBITMAP fCardFace;
	uint32 gCardWidth; // =  90;
	uint32 gCardHeight; // =  131;
	int32 gOverlapFaceDown; // = (gCardHeight/8);
	int32 gOverlap; // = (gCardHeight/4);
	CardSizer fCardSizer;
};

void GetCardName(const Card &card, TString &name);
void GetCardNumber(const Card &card, TString &number);
void DrawSuitCentered(HDC hdc, const Card &card, int32 x, int32 y, uint32 width, bool flip);
void DrawCardDirect(HDC hdc, int32 x, int32 y, uint32 width, uint32 height, const Card &card);
