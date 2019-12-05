
#include "stdafx.h"
#include "people.h"
#include "Klondike.h"
#include <vector>
#include <algorithm>
#include "TraceToNotepad.h"
#include "Game.h"
#include "Utils.h"

void Game::Move()
{
	fState.FindMoves(fMoveList);

	if (!fMoveList.empty())
	{
		MakeFirstMove();
		Invalidate();
	}
}

void Game::PickMove()
{
}

void Game::MakeFirstMove()
{
	if (fMoveList.empty())
		return;

	const PossibleMove move = fMoveList[0];

	fState.MakeMove(move);
}


#if 0
void Game::DisplayMoves() const
{
	for (size_t i = 0; i < fMoveList.size(); i++)
	{
		Card sourceCard;
		GetCard(fMoveList[i].fSource, fMoveList[i].fPosition, sourceCard);
		Card destinationCard;
		GetCard(fMoveList[i].fDestination, -1, destinationCard);

		TString sourceCardName;
		GetCardName(sourceCard, sourceCardName);

		TString destinationCardName;
		GetCardName(destinationCard, destinationCardName);

		TCHAR buffer[256];

		PileNumber sourcePileNumber = fMoveList[i].fSource;
		if (sourcePileNumber == kDeck)
		{
			_stprintf(buffer, _T("waste card %s"), sourceCardName.c_str());
			TraceToNotepad_Write(buffer);
		}
		else if (sourcePileNumber >= kFaceDown0 && sourcePileNumber <= kFaceDown6)
		{
			TraceToNotepad_Write(_T("flip card"));
		}
		else
		{
			if (sourceCard.GetNumber() == Card::kKing)
				_stprintf(buffer, _T("%s to empty"), sourceCardName.c_str());
			else
				_stprintf(buffer, _T("%s -> %s"), sourceCardName.c_str(), destinationCardName.c_str());
			TraceToNotepad_Write(buffer);
		}
		TraceToNotepad_WriteNewline();
	}
	TraceToNotepad_WriteNewline();
}
#endif
