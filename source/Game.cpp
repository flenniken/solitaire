// Copyright 2007-2019 Flenniken
// All Rights Reserved.

#include "stdafx.h"
#include "people.h"
#include "Klondike.h"
#include <vector>
#include <algorithm>
#include "TraceToNotepad.h"
#include "Game.h"
#include "Utils.h"
#include "Stats.h"
#include "Strategy.h"
#include "PlayUntil.h"
#include "Options.h"

#pragma warning ( disable : 4996 ) // 4996: '_stprintf' was declared deprecated


/**
Deck of 52 cards.

Klondike
1 deck pile
1 waste pile
7 piles of tableau face down cards.
7 piles of tableau face up cards.
4 piles of foundations. 

1. Set up a deck in order.
2. Shuffle the deck.
3. Deal the cards to form the tableau.
4. Determine the possible moves.
5. Pick one of the moves.
6. Repeat 4 and 5 until no more moves
7. Record stats.
8. Go to 2.

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

number + (suit * 13)
*/


static TCHAR *gNumber[13] = {_T("A"), _T("2"), _T("3"), _T("4"), _T("5"), _T("6"), _T("7"), _T("8"), _T("9"), _T("10"), _T("J"), _T("Q"), _T("K")};
static char gSuit[4] = {'H', 'D', 'S', 'C'};
TraceToNotepad_Extern;

extern HWND gMainWindow;
extern Stats gStats;
extern Strategy gStrategy;

static bool UserStoppedGame();


// This is called when the user quits the program.
void Game::QuitGame()
{
	if (fState.IsGameStarted())
		gStats.GameFinished(fState.GetCardsUp());
}

void Game::Play()
{
	if (fState.IsGameStarted())
		gStats.GameFinished(fState.GetCardsUp());

	fState.ShuffleAndDeal();
	TString gameString;
	fState.GetGameString(gameString);

	Invalidate();
}

void Game::Move()
{
	fState.FindMoves(fMoveList);

	PossibleMove move;
	if (gStrategy.PickMove(fState, fMoveList, move))
		return;

	fState.MakeMove(move);
	Invalidate();
}

uint32 Game::PlayHand(uint32 drawCount, bool &userStoppedGame)
{
	fState.FindMoves(fMoveList);

	userStoppedGame = false;

	uint32 drawCounter = 0;

	PossibleMove move;
	while (!gStrategy.PickMove(fState, fMoveList, move))
	{
		fState.MakeMove(move);

		if (++drawCounter > drawCount)
		{
			DrawGameNow();
			drawCounter = 0;
		}

		if (UserStoppedGame())
		{
			userStoppedGame = true;
			break;
		}

		fState.FindMoves(fMoveList);
	}

	DrawGameNow();

	uint32 cardsUp = fState.GetCardsUp();

	return cardsUp;
}

bool UserStoppedGame()
{
	// Stop playing when the user clicks the mouse.
	uint32 messageTypes = HIWORD(GetQueueStatus(QS_MOUSEBUTTON | QS_KEY));
	
	if (messageTypes & QS_MOUSEBUTTON)
		return true;

	if (messageTypes & QS_KEY)
	{
		// Look for the key down message.
		MSG msg;
		if (PeekMessage(&msg, gMainWindow, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE))
			return true;
	}
	return false;
}

void Game::PlayUntilWin()
{
	PlayXGames(52, PlayUntilUtils::kCardsUp);
}

void Game::PlayUntil()
{
	uint32 number;
	PlayUntilUtils::Type type;
	if (PlayUntilUtils::ShowDialog(number, type))
		return; // user canceled

	PlayXGames(number, type);
}

void Game::Options()
{
	bool dealOneAtATime;
	bool allowPartialPileMoves;
	bool oneTimeThrough;
	fState.GetOptions(dealOneAtATime, allowPartialPileMoves, oneTimeThrough);

	if (OptionsUtils::ShowDialog(dealOneAtATime, allowPartialPileMoves, oneTimeThrough))
		return; // user canceled

	SetOptions(dealOneAtATime, allowPartialPileMoves, oneTimeThrough);
}

void Game::SetOptions(bool &dealOneAtATime, bool &allowPartialPileMoves, bool &oneTimeThrough)
{
	fState.SetOptions(dealOneAtATime, allowPartialPileMoves, oneTimeThrough);
	gStats.GameOptions(dealOneAtATime, allowPartialPileMoves, oneTimeThrough);
}

void Game::PlayXGames(uint32 number, uint32 type)
{
	_ASSERT(type < 3);

	uint32 gamesPlayed = 0;
	uint32 gamesWon = 0;

	while (1)
	{
		bool userStoppedGame;
		uint32 cardsUp = PlayHand(15, userStoppedGame);
		if (userStoppedGame)
			break;

		gamesPlayed++;
		if (cardsUp == 52)
			gamesWon++;

		if (type == 1 && cardsUp == number)
			break; // done

		Play();

		if (type == 0 && gamesPlayed == number)
			break; // done
		else if (type == 2 && gamesWon == number)
			break; // done
	}
}

void Game::RedealSameDeck()
{
	fState.RedealSameDeck();
	Invalidate();
}

void Game::Undo()
{
	fState.UndoMove();
	Invalidate();
}

/** Construct a move object. Move part of the source pile to the destination pile.
	@param game is a pointer to a game object.
	@param source is the source pile
	@param position is the index into the source pile. The position to the end is moved.
	@param destination is the destination pile.
	@param middleMove is true when the move is a middle move, otherwise false.
*/
PossibleMove::PossibleMove(const GameState &gameState, PileNumber source, int32 position, PileNumber destination) :
	fSource(source), fPosition(position), fDestination(destination)
{
#ifdef DEBUG
	_ASSERT(fSource < kFoundation0);
	_ASSERT(fDestination < kPileCount);

	uint32 size = gameState.GetPileSize(source);
	_ASSERT(size > 0);

	if (position == -1)
		position = size-1;

	if (destination == kDeck)
	{
		_ASSERT(source == kWastePile);
		uint32 destSize = gameState.GetPileSize(destination);
		_ASSERT(destSize == 0);
	}
	else if (source <= kFaceDown6) // Top card from these piles only.
	{
		_ASSERT(position+1 == size);
	}
		
#endif
}

#if 0
void GetCardName(const Card &card, TString &name)
{
	uint32 number = card.GetNumber();
	_ASSERT(number < 13);

	uint32 suit = card.GetSuit();
	_ASSERT(suit < 4);

	TCHAR n = (TCHAR)gNumber[number];
	TCHAR s = (TCHAR)gSuit[suit];
	TString string;
	name += n;
	name += s;
}
#endif

void GetCardNumber(const Card &card, TString &string)
{
	uint32 number = card.GetNumber();
	_ASSERT(number < 13);

	string = gNumber[number];
}

/** Move rules, strategy

	Move waste card to foundation.
	Move tableau card to foundation.
	Move waste card to tableau.
	Move tableau pile.
	Move partial tableau pile when it opens up move to foundation.
	Flip tableau face down card.
	Flip deck card(s).
	Move waste ace or two to foundation.
	Move tableau ace or two to foundation.
	Move tableau pile from bigger face down pile.
	Move tableau pile from smaller face down pile.
	Move tableau card to foundation when its playable cards are up already.
	Move tableau pile making empty spot when king move exists.
	Move king when its jack is available.
	Move tableau pile unless it leaves an empty spot.
	Move tableau pile when it opens up face down card.
	Flip deck card(s) if waste pile is empty.

	Options

	Turn over one card at a time or three at a time.
	Go once through the deck or multiple times through.
	Move king to empty pile or move any bottom card to empty pile.

	Deal once, or deal until 1, 2, 3 or 4 aces appear.
	Play manually.
	Play automatically, push key to make move.
	Play automatically, stop after x games played.
	Let the user specify the deck order.
	Replay same deck.
	Show moves.

	Statistics

	Show deck using 104 characters. A,2,3,4,5,6,7,8,9,T,J,Q,K,H,D,S,C.
	Show number of cards moved to the foundation.
	Show stratagy as an order string of letters, e.g. abcdefghijklmn.
	Show Options?
	Show games played.
	Show average cards moved to foundation.

	Statistics are persistent. Store them in ini file.

	Support undo
*/

void GameState::SetOptions(bool dealOneAtATime, bool allowPartialPileMoves, bool oneTimeThrough)
{
	fDealOneAtATime = dealOneAtATime;
	fAllowPartialPileMoves = allowPartialPileMoves;
	fOneTimeThrough = oneTimeThrough;
}

void GameState::GetOptions(bool &dealOneAtATime, bool &allowPartialPileMoves, bool &oneTimeThrough) const
{
	dealOneAtATime = fDealOneAtATime;
	allowPartialPileMoves = fAllowPartialPileMoves;
	oneTimeThrough = fOneTimeThrough;
}

RECT Game::GetCardRect(const VisibleRectangle &visibleRectangle) const
{
	RECT rect;
	rect.left = visibleRectangle.fx;
	rect.top = visibleRectangle.fy;
	rect.right = visibleRectangle.fx+gCardWidth;
	rect.bottom = visibleRectangle.fy+gCardHeight;
	return rect;
}

RECT Game::GetVisibleRect(const VisibleRectangle &visibleRectangle) const
{
	int32 height;
	if (visibleRectangle.fTopOfPile || visibleRectangle.fEmptySpot)
		return GetCardRect(visibleRectangle);
	else if (visibleRectangle.fPileNumber >= kFaceUp0)
		height = gOverlap;
	else
		height = gOverlapFaceDown;

	RECT rect;
	rect.left = visibleRectangle.fx;
	rect.top = visibleRectangle.fy;
	rect.right = visibleRectangle.fx+gCardWidth;
	rect.bottom = visibleRectangle.fy+height;
	return rect;
}

int32 Game::GetVisibleOverlap(const VisibleRectangle &visibleRectangle) const
{
	if (visibleRectangle.fTopOfPile)
		return 0;
	else if (visibleRectangle.fPileNumber >= kFaceUp0)
		return gOverlap;

	return gOverlapFaceDown;
}

bool Game::IsOnCard(const VisibleRectangle &visibleRectangle, int32 x, int32 y) const
{
	RECT rect = GetVisibleRect(visibleRectangle);
	if (x < rect.left)
		return false;
	if (x > rect.right)
		return false;
	if (y < rect.top)
		return false;
	if (y > rect.bottom)
		return false;
	return true;
}

