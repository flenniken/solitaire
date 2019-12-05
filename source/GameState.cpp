// Copyright 2007-2019 Flenniken
// All Rights Reserved.

#define _CRT_RAND_S
#include "stdafx.h"
#include "people.h"
#include "Klondike.h"
#include <vector>
#include <algorithm>
#include "GameState.h"
#include "time.h"

GameState::GameState() : 
	fGameStarted(false), 
	fDealOneAtATime(true), 
	fAllowPartialPileMoves(true), 
	fOneTimeThrough(true)
{
	srand((unsigned)time(NULL));

	CreateOrderedDeck();
	InitializePointersToPiles();
	ShuffleAndDeal();
}

class MyRandom
{
public:
	int operator()(int max)
	{
//		double tmp = static_cast<double>(rand()) / static_cast<double>(RAND_MAX);
//		return static_cast<int>(tmp * max);

		unsigned int number;
		rand_s(&number);
		double tmp = static_cast<double>(number) / static_cast<double>(UINT_MAX);
		return static_cast<int>(tmp * max);
	}
};

void GameState::ShuffleAndDeal()
{
	ClearTable();

	fDeck = fStartingDeck;
	MyRandom myRandom;
	random_shuffle(fDeck.begin(), fDeck.end(), myRandom);
	fStartingDeck = fDeck;

	Deal();

	fGameStarted = false;  // Set true on the first move.
}

void GameState::RedealSameDeck()
{
	ClearTable();
	fDeck = fStartingDeck;
	Deal();
}

std::vector<Card> *GameState::ToPile(PileNumber pileNumber) const
{
	_ASSERT(pileNumber < kPileCount);
	_ASSERT(fPointerToPiles.size() == kPileCount);

	return fPointerToPiles[pileNumber];
}

void GameState::InitializePointersToPiles()
{
	fPointerToPiles.clear();
	fPointerToPiles.push_back(&fDeck);			// 0	kDeck
	fPointerToPiles.push_back(&fWastePile);		// 1	kWastePile
	fPointerToPiles.push_back(&fFaceDown[0]);	// 2	kFaceDown0
	fPointerToPiles.push_back(&fFaceDown[1]);	// 3	kFaceDown1
	fPointerToPiles.push_back(&fFaceDown[2]);	// 4	kFaceDown2
	fPointerToPiles.push_back(&fFaceDown[3]);	// 5	kFaceDown3
	fPointerToPiles.push_back(&fFaceDown[4]);	// 6	kFaceDown4
	fPointerToPiles.push_back(&fFaceDown[5]);	// 7	kFaceDown5
	fPointerToPiles.push_back(&fFaceDown[6]);	// 8	kFaceDown6
	fPointerToPiles.push_back(&fFaceUp[0]);		// 9	kFaceUp0
	fPointerToPiles.push_back(&fFaceUp[1]);		// 10	kFaceUp1
	fPointerToPiles.push_back(&fFaceUp[2]);		// 11	kFaceUp2
	fPointerToPiles.push_back(&fFaceUp[3]);		// 12	kFaceUp3
	fPointerToPiles.push_back(&fFaceUp[4]);		// 13	kFaceUp4
	fPointerToPiles.push_back(&fFaceUp[5]);		// 14	kFaceUp5
	fPointerToPiles.push_back(&fFaceUp[6]);		// 15	kFaceUp6
	fPointerToPiles.push_back(&fFoundation0);	// 16	kFoundation0
	fPointerToPiles.push_back(&fFoundation1);	// 17	kFoundation1
	fPointerToPiles.push_back(&fFoundation2);	// 18	kFoundation2
	fPointerToPiles.push_back(&fFoundation3);	// 19	kFoundation3
	fPointerToPiles.push_back(&fPickUp);		// 20	kPickUp
};

bool GameState::IsPileEmpty(PileNumber pileNumber) const
{
	_ASSERT(pileNumber < kPileCount);

	std::vector<Card> *pile = ToPile(pileNumber);
	return pile->empty();
}

uint32 GameState::GetPileSize(PileNumber pileNumber) const
{
	_ASSERT(pileNumber < kPileCount);

	std::vector<Card> *pile = ToPile(pileNumber);
	return (uint32)pile->size();
}


/** Get the card in the given pile at the given position. 
	@param pileNumber is the pile to use.
	@param position is the position of the card in the pile, 0, 1,... or -1 for the top card.
	@param card is returned.
	@return 0 when a card exists, otherwise return 1.
*/
int GameState::GetCard(PileNumber pileNumber, int32 position, Card &card) const
{
	uint32 realPosition;
	std::vector<Card> *sourcePile = GetSourcePile(pileNumber, position, realPosition);
	if (realPosition >= (uint32)sourcePile->size())
		return 1;

	card = (*sourcePile)[realPosition];

	return 0; // success
}


/** Get the foundation corresponding to the given card.
	@param card
	@return the pile number to match the suit of the given card.
*/
PileNumber GameState::GetFoundation(const Card &card) const
{
	// Look through each foundation to find the suit.
	PileNumber emptyFoundationPileNumber;
	for (int32 i = 0; i < 4; i++)
	{
		PileNumber foundationPileNumber = PILENUMBER(kFoundation0+i);
		Card foundationCard;
		if (GetCard(foundationPileNumber, 0, foundationCard) == 0)
		{
			if (foundationCard.GetSuit() == card.GetSuit())
				return foundationPileNumber;
		}
		else
			emptyFoundationPileNumber = foundationPileNumber;
	}
	// Could not find a foundation with the suit. Use the last empty one.
	return emptyFoundationPileNumber;
}



void GameState::ClearTable()
{
	fDeck.clear();
	fWastePile.clear();
	fFoundation0.clear();
	fFoundation1.clear();
	fFoundation2.clear();
	fFoundation3.clear();
	for (uint32 i = 0; i < 7; i++)
	{
		fFaceDown[i].clear();
		fFaceUp[i].clear();
	}
	fMoveHistory.clear();

	fMadeNonWastePileMove = false;	// This tells whether we made a move since the last flip.
	fFlippedWastePileCount = 0;		// This tells how many times we flipped the waste pile to make a new stock.
}

void GameState::CreateOrderedDeck()
{
	// Create the deck with all cards in order.
	for (int i = 0; i < 13; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			Card card((Card::Number)i, (Card::Suit)j);
#ifdef DEBUG
			Card::Number number = card.GetNumber();
			_ASSERT(number == i);
			Card::Suit suit = card.GetSuit();
			_ASSERT(suit == j);
#endif
			fStartingDeck.push_back(card);
		}
	}

	_ASSERT(fStartingDeck.size() == 52);
}

void GameState::Deal()
{
	// Deal out the tableau.
	for (int j = 0; j < 7; j++)
	{
		// Deal the top card of the deck face up.
		Card card = fDeck.back();
		fDeck.pop_back();
		fFaceUp[j].push_back(card);

		// Deal the remaining piles face down.
		for (int i = j+1; i < 7; i++)
		{
			// Get the card at the top.
			Card card = fDeck.back();
			fDeck.pop_back();

			fFaceDown[i].push_back(card);
		}
	}

	// Deal the top card of the deck to the waste pile.
//	Card card = fDeck.back();
//	fDeck.pop_back();
//	fWastePile.push_back(card);

#ifdef DEBUG
	// Verify that the piles contains the correct number of cards.
	uint32 size = (uint32)fDeck.size();
	_ASSERT(size == 52-28);
	for (uint32 i = 0; i < 7; i++)
	{
		size = (uint32)fFaceUp[i].size();
		_ASSERT(size == 1);
	}
	for (uint32 i = 0; i < 7; i++)
	{
		size = (uint32)fFaceDown[i].size();
		_ASSERT(size == i);
	}
#endif
}

bool GameState::IsFoundationMove(PileNumber sourcePileNumber, int32 position, PileNumber foundationPileNumber) const
{
	if (foundationPileNumber < kFoundation0 || foundationPileNumber > kFoundation3)
		return false; // not foundation pile

	Card sourceCard;
	if (GetCard(sourcePileNumber, position, sourceCard))
		return false;

	Card foundationTopCard;
	if (GetCard(foundationPileNumber, -1, foundationTopCard) == 0)
	{
		if (sourceCard.GetNumber() == foundationTopCard.GetNumber()+1)
		{
			if (sourceCard.GetSuit() == foundationTopCard.GetSuit())
				return true;
		}
	}
	else
	{
		if (sourceCard.GetNumber() == Card::kAce)
			return true;
	}
	return false;
}

bool GameState::IsAnyFoundationMove(PileNumber sourcePileNumber, int32 position, PileNumber &foundationPileNumber) const
{
	Card sourceCard;
	if (GetCard(sourcePileNumber, position, sourceCard))
		return false;

	foundationPileNumber = GetFoundation(sourceCard);
	return IsFoundationMove(sourcePileNumber, position, foundationPileNumber);
}

/** Determine whether the picked up cards can be dragged to the destination card pile.
*/
bool GameState::IsDragMove(PileNumber destinationPileNumber) const
{
	uint32 count = GetPileSize(kPickedUp);
	if (!count)
		return false;

	// Allow the user to move tableau piles without any cards under, to empty tableau spots.
	if (IsNeatenMove(destinationPileNumber))
		return true;

	if (count == 1)
	{
		if (IsFoundationMove(kPickedUp, 0, destinationPileNumber))
			return true;
	}

	if (!fAllowPartialPileMoves)
	{
		if (fPickUpSource != kWastePile && fPickUpPosition != 0)
			return false;
	}

	return IsMoveToTableau(kPickedUp, 0, destinationPileNumber);
}

bool GameState::IsNeatenMove(PileNumber destinationPileNumber) const
{
	if (fPickUpPosition != 0)
		return false;

	if (!IsPileEmpty(fPickUpSource))
		return false;

	if (fPickUpSource < kFaceUp0 || fPickUpSource > kFaceUp6)
		return false; // not valid move
	if (destinationPileNumber < kFaceUp0 || destinationPileNumber > kFaceUp6)
		return false; // not valid move
	if (!IsPileEmpty(destinationPileNumber))
		return false;
	if (!IsPileEmpty(PILENUMBER(fPickUpSource-7)))
		return false; // Source pile has face down cards under it.

	return true;
}

bool GameState::IsMoveToTableau(PileNumber sourcePileNumber, int32 position, PileNumber destinationPileNumber) const
{
	Card sourceCard;
	if (GetCard(sourcePileNumber, position, sourceCard))
		return false;

	if (destinationPileNumber < kFaceUp0 || destinationPileNumber > kFaceUp6)
		return false; // not valid move

	Card destinationCard;
	if (GetCard(destinationPileNumber, -1, destinationCard))
	{
		if (sourceCard.GetNumber() != Card::kKing)
			return false; // not valid move
		// Determine whether there are any face down cards at the destination.
		if (!IsPileEmpty(PILENUMBER(destinationPileNumber-7)))
			return false;
	}
	else
	{
		if (destinationCard.GetNumber() != sourceCard.GetNumber()+1)
			return false; // not valid move

		if (destinationCard.IsRed() == sourceCard.IsRed())
			return false; // not valid move
	}

	return true;
}

uint32 GameState::GetCardsUp()
{
	return (uint32)fFoundation0.size()+fFoundation1.size()+fFoundation2.size()+fFoundation3.size();
}

void GameState::UndoMove()
{
	if (fMoveHistory.empty())
		return;

	std::vector<PossibleMove> moveHistory = fMoveHistory;

	RedealSameDeck();

	uint32 count = (uint32)moveHistory.size();
	if (count <= 1)
		return;

	for (uint32 i = 0; i < count-1; i++)
		MakeMove(moveHistory[i]);
}

// Move the cards to the pick up pile.
void GameState::PickUpCards(PileNumber sourcePileNumber, int32 position)
{
	_ASSERT(sourcePileNumber < kPileCount);
	_ASSERT(fPickUp.empty());

	uint32 realPosition;
	std::vector<Card> *sourcePile = GetSourcePile(sourcePileNumber, position, realPosition);

	_ASSERT(realPosition < (uint32)sourcePile->size());

	fPickUp.insert(fPickUp.end(), sourcePile->begin()+realPosition, sourcePile->end());

	sourcePile->erase(sourcePile->begin()+realPosition, sourcePile->end());

	fPickUpSource = sourcePileNumber;
	fPickUpPosition = position;
}

// Move the cards from the pick up pile back from where the came.
void GameState::PutBackCards()
{
	_ASSERT(!fPickUp.empty());

	std::vector<Card> *sourcePile = ToPile(fPickUpSource);

	sourcePile->insert(sourcePile->end(), fPickUp.begin(), fPickUp.end());

	fPickUp.clear();
}

std::vector<Card> *GameState::GetSourcePile(PileNumber sourcePileNumber, int32 position, uint32 &realPosition) const
{
	_ASSERT(sourcePileNumber < kPileCount);

	std::vector<Card> *sourcePile = ToPile(sourcePileNumber);

	if (position == -1)
	{
		uint32 sourcePileCount = (uint32)sourcePile->size();
		realPosition = sourcePileCount - 1;
	}
	else
		realPosition = position;

	return sourcePile;
}

int GameState::MakeMove(const PossibleMove &move)
{
	uint32 realPosition;
	std::vector<Card> *sourcePile = GetSourcePile(move.fSource, move.fPosition, realPosition);

	_ASSERT(realPosition < (uint32)sourcePile->size());

	std::vector<Card> *destinationPile = ToPile(move.fDestination);

	if (move.fDestination == kDeck)
	{
		// Turn over the waste pile to make a new stock.

		_ASSERT(destinationPile->size() == 0);
		int32 count = (int32)sourcePile->size();
		_ASSERT(count > 0);

		for (int32 i = count-1; i >= 0; i--)
		{
			Card card = (*sourcePile)[i];
			destinationPile->push_back(card);
		}
		sourcePile->clear();

		fFlippedWastePileCount++;
	}
	else if (move.fSource == kDeck && !fDealOneAtATime)
	{
		// Dealing three at a time.
		// Flip over three cards or as many as there are.
		uint32 count = (uint32)sourcePile->size();
		if (count > 3)
			count = 3;
		for (uint32 i = 0; i < count; i++)
		{
			Card topCard = sourcePile->back();
			destinationPile->push_back(topCard);
			sourcePile->pop_back();
		}
	}
	else
	{
		if (move.fDestination != kWastePile)
			fMadeNonWastePileMove = true;

		destinationPile->insert(destinationPile->end(), sourcePile->begin()+realPosition, sourcePile->end());

		sourcePile->erase(sourcePile->begin()+realPosition, sourcePile->end());
	}

	fMoveHistory.push_back(move);

	fGameStarted = true;

	return 0; // success
}

void GameState::FindMoves(std::vector<PossibleMove> &moveList)
{
	moveList.clear();

	// Check tableau face down card to its empty face up pile.
	for (uint32 i = 0; i < 7; i++)
	{
		PileNumber destinationPileNumber = PILENUMBER(kFaceUp0+i);

		if (IsPileEmpty(destinationPileNumber))
		{
			PileNumber sourcePileNumber = PILENUMBER(kFaceDown0+i);

			if (!IsPileEmpty(sourcePileNumber))
			{
				PossibleMove possibleMove(*this, sourcePileNumber, -1, destinationPileNumber);
				moveList.push_back(possibleMove);
			}
		}
	}

	// Check bottom tableau piles to other tableau piles.
	for (uint32 i = 0; i < 7; i++)
	{
		PileNumber sourcePileNumber = PILENUMBER(kFaceUp0+i);

		for (uint32 j = 0; j < 7; j++)
		{
			if (i == j)
				continue;

			PileNumber destinationPileNumber = PILENUMBER(kFaceUp0+j);

			if (IsMoveToTableau(sourcePileNumber, 0, destinationPileNumber))
			{
				Card card;
				GetCard(sourcePileNumber, 0, card);

				// Don't move the king if it's doesn't have any face down cards under it.
				if (card.GetNumber() == Card::kKing)
				{
					if (IsPileEmpty(PILENUMBER(kFaceDown0+i)))
						continue;
				}

				PossibleMove possibleMove(*this, sourcePileNumber, 0, destinationPileNumber);
				moveList.push_back(possibleMove);
			}
		}
	}

	// Check tableau face up piles to the foundation.
	for (uint32 i = 0; i < 7; i++)
	{
		PileNumber sourcePileNumber = PILENUMBER(kFaceUp0+i);

		PileNumber foundationPileNumber;
		if (IsAnyFoundationMove(sourcePileNumber, -1, foundationPileNumber))
		{
			PossibleMove possibleMove(*this, sourcePileNumber, -1, foundationPileNumber);
			moveList.push_back(possibleMove);
		}
	}

	// Check moving the waste card.
	{
		PileNumber destinationPileNumber;

		// Check waste card to tableau pile.
		for (uint32 i = 0; i < 7; i++)
		{
			destinationPileNumber = PILENUMBER(kFaceUp0+i);
			if (IsMoveToTableau(kWastePile, -1, destinationPileNumber))
			{
				PossibleMove possibleMove(*this, kWastePile, -1, destinationPileNumber);
				moveList.push_back(possibleMove);
			}
		}

		// Check waste card to the foundation.
		if (IsAnyFoundationMove(kWastePile, -1, destinationPileNumber))
		{
			PossibleMove possibleMove(*this, kWastePile, -1, destinationPileNumber);
			moveList.push_back(possibleMove);
		}
	}

	// Check tableau part pile to another tableau pile.
	if (fAllowPartialPileMoves)
	{
		for (uint32 i = 0; i < 7; i++)
		{
			PileNumber sourcePileNumber = PILENUMBER(kFaceUp0+i);

			uint32 count = GetPileSize(sourcePileNumber);
			if (count < 2)
				continue; // We're looking at the middle of the pile, not the bottom.

			// Look at all face up cards in this column except the bottom.
			for (uint32 j = 1; j < count; j++)
			{
				// Look at the top card in the other columns.
				for (uint32 k = 0; k < 7; k++)
				{
					if (k == i)
						continue;

					PileNumber destinationPileNumber = PILENUMBER(kFaceUp0+k);

					if (IsMoveToTableau(sourcePileNumber, j, destinationPileNumber))
					{
						// Don't make a middle move unless it opens up a move to the foundation.
						// If the card we are moving to can play on the foundation too, then
						// foundation moves are not being taken and we will get in a cycle.
						PileNumber foundationPileNumber;
						if (IsAnyFoundationMove(sourcePileNumber, j-1, foundationPileNumber))
						{
							if (!IsAnyFoundationMove(destinationPileNumber, -1, foundationPileNumber))
							{
								PossibleMove possibleMove(*this, sourcePileNumber, j, destinationPileNumber);
								moveList.push_back(possibleMove);
							}
						}
					}
				}
			}
		}
	}

	// As long as there is a card in the deck, you can move it to the waste pile.
	if (!IsPileEmpty(kDeck))
	{
		PossibleMove possibleMove(*this, kDeck, -1, kWastePile);
		moveList.push_back(possibleMove);
	}

	// See whether we can flip over the waste pile to make a new stock.
	if (IsPileEmpty(kDeck))
	{
		if (!IsPileEmpty(kWastePile))
		{
			// If we are playing multiple times though the deck and we made a move since the last flip.
			// Or we are dealing three at a time and we haven't flipped three times.
			if ((!fOneTimeThrough && fMadeNonWastePileMove) || 
				(!fDealOneAtATime && fFlippedWastePileCount < 2))
			{
				PossibleMove possibleMove(*this, kWastePile, 0, kDeck);
				moveList.push_back(possibleMove);
				fMadeNonWastePileMove = false;
			}
		}
	}
}

static TCHAR *gNumber[13] = {_T("A"), _T("2"), _T("3"), _T("4"), _T("5"), _T("6"), _T("7"), _T("8"), _T("9"), _T("T"), _T("J"), _T("Q"), _T("K")};
static char gSuit[4] = {'H', 'D', 'S', 'C'};

/*
There are 13 cards and 4 suits with 2 colors.
0-12, 4 bits for the cards, ace, 1, 2, ..., 9, Jack, Queen, King.
0-3, 2 bits for the suit. Hearts, Diamonds, Spades, Clubs. High bit is color, red, black.
0000 0 ace
0001 1 2
0010 2 3
0011 3 4
0100 4 5
0101 5 6
0110 6 7
0111 7 8
1000 8 9
1001 9 ten
1010 10 jack
1011 11 queen
1100 12 king
1101 13 empty
1110 14 empty
1111 15 empty
00 hearts
01 diamonds
10 spades
11 clubs


*/

void GameState::GetGameString(TString &string) const
{
	_ASSERT(fStartingDeck.size() == 52);

	for (uint32 i = 0; i < 52; i++)
	{
		Card card = fStartingDeck[i];
		string += gNumber[card.GetNumber()];
		string += gSuit[card.GetSuit()];
	}
}

