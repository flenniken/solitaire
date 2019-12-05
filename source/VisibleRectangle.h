// Copyright 2007-2019 Flenniken
// All Rights Reserved.

#pragma once

class VisibleRectangle
{
public:
	VisibleRectangle() :
		fx(0), fy(0), fPileNumber(kDeck), fPosition(-1), fTopOfPile(false), fEmptySpot(true) {}
	VisibleRectangle(int32 x, int32 y, PileNumber pileNumber) : 
		fx(x), fy(y), fPileNumber(pileNumber), fPosition(-1), fTopOfPile(true), fEmptySpot(false) {}
	VisibleRectangle(int32 x, int32 y, PileNumber pileNumber, uint32 position) : 
		fx(x), fy(y), fPileNumber(pileNumber), fPosition(position), fTopOfPile(false), fEmptySpot(false) {}
	~VisibleRectangle(){};

	int32 fx;
	int32 fy;
	PileNumber fPileNumber;
	int32 fPosition; // -1 means top card of pile number.
	bool fTopOfPile; // Top of face down and face up together.
	bool fEmptySpot; // When face down and face up piles are both empty.
};
