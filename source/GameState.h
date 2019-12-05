// Copyright 2007-2019 Flenniken
// All Rights Reserved.

#pragma once

#include "Card.h"

enum PileNumber
{
	kDeck,
	kWastePile,
	kFaceDown0,
	kFaceDown1,
	kFaceDown2,
	kFaceDown3,
	kFaceDown4,
	kFaceDown5,
	kFaceDown6,
	kFaceUp0,
	kFaceUp1,
	kFaceUp2,
	kFaceUp3,
	kFaceUp4,
	kFaceUp5,
	kFaceUp6,
	kFoundation0,
	kFoundation1,
	kFoundation2,
	kFoundation3,
	kPickedUp,
	kPileCount
};

#define PILENUMBER(a) (PileNumber)(a)

class GameState;
class Card;

/** This describes a move in the given state.
*/
class PossibleMove
{
public:
	PossibleMove() : fSource(PILENUMBER(0)), fPosition(0), fDestination(PILENUMBER(0)) {}
	PossibleMove(const GameState &gameState, PileNumber source, int32 position, PileNumber destination);
	~PossibleMove() {}

	PileNumber fSource;
	int32 fPosition;
	PileNumber fDestination;
};

class GameState
{
public:
	GameState();
	~GameState(){}

	int GetCard(PileNumber pileNumber, int32 position, Card &card) const;
	uint32 GetCardsUp();

	bool IsPileEmpty(PileNumber pileNumber) const;
	uint32 GetPileSize(PileNumber pileNumber) const;

	bool IsDragMove(PileNumber destinationPileNumber) const;
	bool IsMoveToTableau(PileNumber sourcePileNumber, int32 position, PileNumber destinationPileNumber) const;
	bool IsAnyFoundationMove(PileNumber sourcePileNumber, int32 position, PileNumber &foundationPileNumber) const;

	void FindMoves(std::vector<PossibleMove> &moveList);
	int MakeMove(const PossibleMove &move);
	void UndoMove();

	void PickUpCards(PileNumber sourcePileNumber, int32 position);
	void PutBackCards();

	void ShuffleAndDeal();
	void RedealSameDeck();

	bool IsGameStarted() const {return fGameStarted;}

	void SetOptions(bool dealOneAtATime, bool kingToEmpty, bool oneTimeThrough);
	void GetOptions(bool &dealOneAtATime, bool &kingToEmpty, bool &oneTimeThrough) const;

//	bool IsDealOneAtATime() const {return fDealOneAtATime;}
	bool IsPartialMoveAllowed() const {return fAllowPartialPileMoves;}
//	bool IsOneTimeThrough() const {return fOneTimeThrough;}

	bool IsWastePileFlipAllowed() const
	{
		// Flip is allowed when option to go through multiple times is on.
		if (!fOneTimeThrough)
			return true;
		// Flip is allowed when dealing three at at time and we haven't gone through three times.
		if (!fDealOneAtATime && fFlippedWastePileCount < 2)
			return true;
		return false;
	}

	void GetGameString(TString &string) const;

private:
	bool IsFoundationMove(PileNumber sourcePileNumber, int32 position, PileNumber foundationPileNumber) const;
	PileNumber GetFoundation(const Card &card) const;
	std::vector<Card> *ToPile(PileNumber pileNumber) const;
	std::vector<Card> *GetSourcePile(PileNumber sourcePileNumber, int32 position, uint32 &realPosition) const;
	bool IsNeatenMove(PileNumber destinationPileNumber) const;

	void CreateOrderedDeck();
	void InitializePointersToPiles();
	void Deal();
	void ClearTable();

	std::vector<Card> fDeck;
	std::vector<Card> fWastePile;

	// Foundation
	std::vector<Card> fFoundation0;
	std::vector<Card> fFoundation1;
	std::vector<Card> fFoundation2;
	std::vector<Card> fFoundation3;

	// Tableau
	std::vector<Card> fFaceDown[7];
	std::vector<Card> fFaceUp[7];

	// Cards picked up.
	std::vector<Card> fPickUp;

	std::vector<std::vector<Card> *> fPointerToPiles;

	std::vector<Card> fStartingDeck;		// Copy of the deck right after shuffling.
	std::vector<PossibleMove> fMoveHistory; // All the moves to get to the current state.

	PileNumber fPickUpSource;				// Where the picked up cards came from.
	uint32 fPickUpPosition;					// The position of the picked up cards in the source pile.

	bool fGameStarted;						// The game is started when the first move is made.

	// Options
	bool fDealOneAtATime;
	bool fAllowPartialPileMoves;
	bool fOneTimeThrough;

	bool fMadeNonWastePileMove;				// Set true after making a move. 
											// You cannot flip the waste pile if no moves since the last flip.
	uint32 fFlippedWastePileCount;				// Count of times we flipped the waste pile.
};

