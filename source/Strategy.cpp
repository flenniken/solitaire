// Copyright 2007-2019 Flenniken
// All Rights Reserved.

#include "stdafx.h"
#include "people.h"
#include "Klondike.h"
#include <vector>
#include "Game.h"
#include <shlwapi.h>
#include <gdiplus.h>
#include "Stats.h"
#include "Strategy.h"
#include <WindowsX.h>
#include "Utils.h"
#include <commctrl.h>
#include <algorithm>
#include "SolitaireIniFile.h"
#include "DrawUtils.h"

class CardAndPile
{
public:
	CardAndPile(const Card &card, PileNumber pileNumber) :  fCard(card), fPileNumber(pileNumber) {}
	~CardAndPile() {}

	Card fCard;
	PileNumber fPileNumber;
};

class MoveAndRules
{
public:
	MoveAndRules(const PossibleMove &possibleMove, Strategy::MoveRule moveRule) :
	  fPossibleMove(possibleMove), fMoveRule(moveRule) {}
	 ~MoveAndRules() {}

	PossibleMove fPossibleMove;
	Strategy::MoveRule fMoveRule;
};


Strategy gStrategy;

extern HWND gMainWindow;
extern HINSTANCE gInstance;

static int32 gX = -1; // Last position of dialog.
static int32 gY = -1;
static uint32 gDragListMsgString;	// Window message for dragging in the listbox.
static int32 gDraggingItem;			// Listbox item being dragged.
static TCHAR gStrategyStr[] = _T("Strategy"); // ini file

static INT_PTR CALLBACK StrategyDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static void PositionDialog(HWND hDlg);
static void RememberPosition(HWND hDlg);
static void MoveRuleToString(Strategy::MoveRule moveRule, TString &string);
static void MoveRuleToLetter(Strategy::MoveRule moveRule, TString &string);
static bool HandleDragging(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static void GetListBoxList(HWND hDlg, std::vector<Strategy::MoveRule> &list);
static void SetListboxList(HWND hDlg, const std::vector<Strategy::MoveRule> &list);
static void UpdateStrategyLetters(HWND hDlg, const std::vector<Strategy::MoveRule> &list);
static void GetStrategyLetters(const std::vector<Strategy::MoveRule> &list, TString &strategyLetters);
static void GetFoundationTopCards(const GameState &gameState, std::vector<Card> &foundationTopCards);
static bool ArePlayableUp(const GameState &gameState, const Card &card);
static void GetBottomDraggableCards(const GameState &gameState, std::vector<CardAndPile> &draggableCards);
static void MoveToRules(const GameState &gameState, const PossibleMove &move, std::vector<MoveAndRules> &moveAndRulesList);
static void LabelMoves(const GameState &gameState, const std::vector<PossibleMove> &moveList, std::vector<MoveAndRules> &moveAndRulesList);
static int LettersToStrategy(const TString &strategyLetters, std::vector<Strategy::MoveRule> &strategy);
static int LetterToMoveRule(TCHAR letter, Strategy::MoveRule &moveRule);

Strategy::Strategy()
{
	// Set the initial strategy list.
	fStrategy.clear();
	for (uint32 i = 0; i < Strategy::kMoveRuleCount; i++)
		fStrategy.push_back((Strategy::MoveRule)i);
}

Strategy::~Strategy()
{
}

void Strategy::ShowDialog()
{
	DialogBox(gInstance, MAKEINTRESOURCE(IDD_STRATEGY), gMainWindow, StrategyDialog);
}

INT_PTR CALLBACK Strategy::StrategyDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	// Handle dragging items in the list.
	if (message == gDragListMsgString)
	{
		if (HandleDragging(hDlg, message, wParam, lParam))
		{
			std::vector<Strategy::MoveRule> list;
			GetListBoxList(hDlg, list);
			UpdateStrategyLetters(hDlg, list);
		}

		return (INT_PTR)TRUE;
	}

	switch (message)
	{
	case WM_INITDIALOG:
		{
			gStrategy.InitializeDialog(hDlg);
			return (INT_PTR)TRUE;
		}
	
	case WM_DRAWITEM:
		switch (wParam)
		{
		case kStrategyPicture:
			DrawUtils::PaintControlPicture(hDlg, ((LPDRAWITEMSTRUCT)lParam)->hDC, kStrategyPicture, Card::kDiamonds);
			break;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			gStrategy.SaveData(hDlg);
			// fall thru
		case IDCANCEL:
			RememberPosition(hDlg);
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
	}
	return (INT_PTR)FALSE;
}

// Handle dragging listbox items.  Return true for a good drop.
bool HandleDragging(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND listbox = GetDlgItem(hDlg, IDC_STRATEGYLIST);
	if (!listbox)
		return false; 

	DRAGLISTINFO *dragListInfo = (DRAGLISTINFO *)lParam;

	switch (dragListInfo->uNotification)
	{
	case DL_BEGINDRAG:
		gDraggingItem = LBItemFromPt(listbox, dragListInfo->ptCursor, false);
		if (gDraggingItem == -1)
			break;

		// Return the result using SetWindowLong.
		SetWindowLong(hDlg, DWL_MSGRESULT, (LONG)true); // OK to begin dragging.
		break;

	case DL_DRAGGING:
		{
			int32 cursorItem = LBItemFromPt(listbox, dragListInfo->ptCursor, false);
			if (cursorItem == -1)
			{
				SetWindowLong(hDlg, DWL_MSGRESULT, (LONG)DL_STOPCURSOR);
				break;
			}
			SetWindowLong(hDlg, DWL_MSGRESULT, (LONG)DL_MOVECURSOR);

			DrawInsert(hDlg, listbox, cursorItem);
			break;
		}

	case DL_CANCELDRAG:
		gDraggingItem = -1;
		break;

	case DL_DROPPED:
		{
			int32 cursorItem = LBItemFromPt(listbox, dragListInfo->ptCursor, false);
			if (cursorItem == -1)
				break;

			Strategy::MoveRule moveRule = (Strategy::MoveRule)ListBox_GetItemData(listbox, gDraggingItem);
			TString draggingString;
			MoveRuleToString(moveRule, draggingString);

			// Insert the dragging item before the cursor item.
			if (gDraggingItem > cursorItem)
			{
				ListBox_DeleteString(listbox, gDraggingItem);
				ListBox_InsertString(listbox, cursorItem, draggingString.c_str());
				ListBox_SetItemData(listbox, cursorItem, moveRule);
			}
			else
			{
				ListBox_InsertString(listbox, cursorItem, draggingString.c_str());
				ListBox_SetItemData(listbox, cursorItem, moveRule);
				ListBox_DeleteString(listbox, gDraggingItem);
			}
			return true; // good drop
		}
	}

	return false; // not good drop
}

void Strategy::InitializeDialog(HWND hDlg)
{
	// Populate the listbox and get the listbox ready.
	SetListboxList(hDlg, fStrategy);

	// Update the strategy letters in the dialog.
	UpdateStrategyLetters(hDlg, fStrategy);

	// Position the dialog at the last place it was.
	PositionDialog(hDlg);
}

void UpdateStrategyLetters(HWND hDlg, const std::vector<Strategy::MoveRule> &list)
{
	TString strategyLetters;
	GetStrategyLetters(list, strategyLetters);

	HWND letters = GetDlgItem(hDlg, IDC_STRATEGYLETTERS);
	if (!letters)
		return;
	SetWindowText(letters, strategyLetters.c_str());

}

void Strategy::GetStrategyLetters(TString &strategyLetters) const
{
	::GetStrategyLetters(fStrategy, strategyLetters);
}

// Get a string of the strategy letters in order.
void GetStrategyLetters(const std::vector<Strategy::MoveRule> &list, TString &strategyLetters)
{
	TString string;
	MoveRuleToLetter(list[0], string);
	strategyLetters += string;

	for (uint32 i = 1; i < Strategy::kMoveRuleCount; i++)
	{
		strategyLetters += _T(",");

		TString string;
		MoveRuleToLetter(list[i], string);
		strategyLetters += string;
	}
}

void PositionDialog(HWND hDlg)
{
	if (gX == -1)
		Utils::CenterWindow(hDlg);
	else
	{
		RECT rect;
		GetWindowRect(hDlg, &rect);
		MoveWindow(hDlg, gX, gY, rect.right-rect.left, rect.bottom-rect.top, false);
	}
}

void RememberPosition(HWND hDlg)
{
	RECT rect;
	GetWindowRect(hDlg, &rect);
	gX = rect.left;
	gY = rect.top;
}

static TCHAR *gMoveRuleLetter[] = {
	_T("a"),							// kMoveWasteToFoundation
	_T("b"),							// kMoveTableauToFoundation
	_T("c"),							// kMoveWasteToTableau
	_T("d"),							// kMoveTableauPile
	_T("e"),							// kMovePartialTableauPile
	_T("f"),							// kFlipTabeauFaceDownCard	
	_T("g"),							// kFlipDeckCards
	_T("h"),							// kMoveWasteAceOrTwo
	_T("i"),							// kMoveTableauAceOrTwo
	_T("j"),							// kMoveTableauPileFromBigger
	_T("k"),							// kMoveTableauPileFromSmaller
	_T("l"),							// kMoveTableauToFoundationPlay
	_T("m"),							// kMoveTableauPileMakingKingSpot
	_T("n"),							// kMoveKingWhenJackIsAvailable
	_T("o"),							// kMoveTableauPileUnlessEmpty
	_T("p"),							// kFlipDeckWhenWastePileEmpty		
	_T("q"),							// kFlipWastePileMakingNewStock		
	_T("r"),							// kMoveTableauToFoundationOpeningFlip		
};

void Strategy::MoveRuleToString(Strategy::MoveRule moveRule, TString &string)
{
	::MoveRuleToString(moveRule, string);
}

void MoveRuleToString(Strategy::MoveRule moveRule, TString &string)
{
	if (moveRule >= Strategy::kMoveRuleCount)
		return;

	uint32 id = moveRule + kStrMoveWasteToFoundation;
	if (id > kStrMoveTableauToFoundationOpeningFlip)
		return;

	string = Utils::GetString(gInstance, id);
}

void MoveRuleToLetter(Strategy::MoveRule moveRule, TString &string)
{
	if (moveRule >= Strategy::kMoveRuleCount)
		return;

	string = gMoveRuleLetter[moveRule];
}

void Strategy::SaveData(HWND hDlg)
{
	// Read the move rules from the listbox.
	std::vector<Strategy::MoveRule> list;
	GetListBoxList(hDlg, list);

	fStrategy = list;
}

// Get the strategy list from the listbox.
void GetListBoxList(HWND hDlg, std::vector<Strategy::MoveRule> &list)
{
	HWND listbox = GetDlgItem(hDlg, IDC_STRATEGYLIST);
	if (!listbox)
		return;

	uint32 count = ListBox_GetCount(listbox);
	if (count != Strategy::kMoveRuleCount)
		return;

	list.clear();

	for (uint32 i = 0; i < Strategy::kMoveRuleCount; i++)
	{
		LRESULT result = ListBox_GetItemData(listbox, i);
		if (result == LB_ERR)
			return;
		Strategy::MoveRule moveRule = (Strategy::MoveRule)result;
		list.push_back(moveRule);
	}
}

void SetListboxList(HWND hDlg, const std::vector<Strategy::MoveRule> &list)
{
	HWND listbox = GetDlgItem(hDlg, IDC_STRATEGYLIST);
	if (!listbox)
		return;

	// Make the listbox support dragging.
	if (MakeDragList(listbox) == 0)
		return;

	// Remember the message for dragging.
	gDragListMsgString = RegisterWindowMessage(DRAGLISTMSGSTRING);
	if (gDragListMsgString == 0)
		return;

	// Populate the listbox with strings.
	ListBox_ResetContent(listbox);
	for (uint32 i = 0; i < Strategy::kMoveRuleCount; i++)
	{
		TString string;
		MoveRuleToString(list[i], string);

		// Add the string to the listbox.
		ListBox_AddString(listbox, string.c_str());

		// Add the rule to the listbox.
		ListBox_SetItemData(listbox, i, list[i]);
	}
}

// Pick the move based on the current strategy.
int Strategy::PickMove(const GameState &gameState, const std::vector<PossibleMove> &moveList, PossibleMove &move) const
{
	uint32 count = (uint32)moveList.size();

	if (count == 0)
		return 1; // empty list

	if (count == 1)
		move = moveList[0];

	std::vector<MoveAndRules> moveAndRulesList;
	LabelMoves(gameState, moveList, moveAndRulesList);

	uint32 moveAndRulesListCount = (uint32)moveAndRulesList.size();
	_ASSERT(moveAndRulesListCount);

	// Look at each of the possible move rules and keep track of the best one (first in strategy list).
	PossibleMove theMove;
	std::vector<Strategy::MoveRule>::const_iterator lowestIt = fStrategy.end();
	for (uint32 i = 0; i < moveAndRulesListCount; i++)
	{
		// Find the rule in the strategy list.
		std::vector<Strategy::MoveRule>::const_iterator it = std::find(fStrategy.begin(), fStrategy.end(), 
			moveAndRulesList[i].fMoveRule);

		_ASSERT(it < fStrategy.end());

		// Remember the best one.
		if (it < lowestIt)
		{
			lowestIt = it;
			theMove = moveAndRulesList[i].fPossibleMove;
		}
	}

	_ASSERT(lowestIt < fStrategy.end());

	move = theMove;

	return 0; // success
}

// For each move return its rule.
void LabelMoves(const GameState &gameState, const std::vector<PossibleMove> &moveList, std::vector<MoveAndRules> &moveAndRulesList)
{
	for (uint32 i = 0; i < moveList.size(); i++)
		MoveToRules(gameState, moveList[i], moveAndRulesList);
}


// Get all the move rules associated with the move.
void MoveToRules(const GameState &gameState, const PossibleMove &move, std::vector<MoveAndRules> &moveAndRulesList)
{
#define PUSHMOVEANDRULE(a) moveAndRulesList.push_back(MoveAndRules(move, Strategy::a))

	if (move.fSource == kDeck)
	{
		// _T("g. Flip deck card(s)."),										// kFlipDeckCards
		// _T("q. Flip deck card(s) if waste pile is empty."),				// kFlipDeckWhenWastePileEmpty		

		_ASSERT(move.fDestination == kWastePile);

		// Is waste pile empty?
		if (gameState.IsPileEmpty(kWastePile))
			PUSHMOVEANDRULE(kFlipDeckWhenWastePileEmpty);

		PUSHMOVEANDRULE(kFlipDeckCards);
		return;
	}
	else if (move.fDestination == kDeck)
	{
		PUSHMOVEANDRULE(kFlipWastePileMakingNewStock);
	}
	else if (move.fSource == kWastePile)
	{
		// _T("h. Move waste ace or two to foundation."),						// kMoveWasteAceOrTwo
		// _T("a. Move waste card to foundation."),							// kMoveWasteToFoundation
		// _T("c. Move waste card to tableau."),								// kMoveWasteToTableau

		if (move.fDestination >= kFoundation0)
		{
			Card card;
			gameState.GetCard(move.fSource, move.fPosition, card);

			if (card.GetNumber() == Card::kAce || card.GetNumber() == Card::kTwo)
				PUSHMOVEANDRULE(kMoveWasteAceOrTwo);

			PUSHMOVEANDRULE(kMoveWasteToFoundation);
		}
		else
			PUSHMOVEANDRULE(kMoveWasteToTableau);

		return;
	}
	else if (move.fSource <= kFaceDown6)
	{
		// _T("f. Flip tableau face down card."),								// kFlipTabeauFaceDownCard	
		
		PUSHMOVEANDRULE(kFlipTabeauFaceDownCard);

		return;
	}
	else
	{
		_ASSERT(move.fSource <= kFaceUp6);

		if (move.fDestination >= kFoundation0)
		{
			// _T("i. Move tableau ace or two to foundation."),					// kMoveTableauAceOrTwo
			// _T("l. Move tableau card to foundation when its playable cards are up already."),	// kMoveTableauToFoundationPlay
			// _T("b. Move tableau card to foundation."),							// kMoveTableauToFoundation
			// _T("r. Move tableau card to foundation when it's on a face down card."),	// kMoveTableauToFoundationOpeningFlip		

			Card card;
			gameState.GetCard(move.fSource, move.fPosition, card);

			if (card.GetNumber() == Card::kAce || card.GetNumber() == Card::kTwo)
				PUSHMOVEANDRULE(kMoveTableauAceOrTwo);
			
			if (ArePlayableUp(gameState, card))
				PUSHMOVEANDRULE(kMoveTableauToFoundationPlay);

			uint32 pileCount = gameState.GetPileSize(move.fSource);
			if (pileCount == 1 && !gameState.IsPileEmpty(PILENUMBER(move.fSource-7)))
				PUSHMOVEANDRULE(kMoveTableauToFoundationOpeningFlip);

			PUSHMOVEANDRULE(kMoveTableauToFoundation);
			return;
		}

		_ASSERT(move.fSource >= kFaceUp0);
		_ASSERT(move.fSource <= kFaceUp6);

		_ASSERT(move.fDestination >= kFaceUp0);
		_ASSERT(move.fDestination <= kFaceUp6);

		// Get the bottom draggable cards and the top waste card.
		std::vector<CardAndPile> draggableCards;
		GetBottomDraggableCards(gameState, draggableCards);

		_ASSERT(move.fPosition != -1);

		if (move.fPosition == 0)
		{
			// Determine whether the face down pile is empty under the source.
			if (gameState.IsPileEmpty(PILENUMBER(move.fSource-7)))
			{
				for (uint32 i = 0; i < draggableCards.size(); i++)
				{
					// _T("m. Move tableau pile making empty spot when king move exists."),// kMoveTableauPileMakingKingSpot
					if (move.fSource != draggableCards[i].fPileNumber && draggableCards[i].fCard.GetNumber() == Card::kKing)
					{
						if (draggableCards[i].fPileNumber == kWastePile || !gameState.IsPileEmpty(PILENUMBER(draggableCards[i].fPileNumber-7)))
						{
							PUSHMOVEANDRULE(kMoveTableauPileMakingKingSpot);
							break;
						}
					}
				}
			}
			else
			{
				// _T("o. Move tableau pile unless it leaves an empty spot."),			// kMoveTableauPileUnlessEmpty
				PUSHMOVEANDRULE(kMoveTableauPileUnlessEmpty);
			}

			Card card;
			gameState.GetCard(move.fSource, move.fPosition, card);

			if (card.GetNumber() == Card::kKing)
			{
				for (uint32 i = 0; i < draggableCards.size(); i++)
				{
					// _T("n. Move king when its jack is available."),						// kMoveKingWhenJackIsAvailable
					if (draggableCards[i].fCard.GetNumber() == Card::kJack && draggableCards[i].fCard.IsRed() != card.IsRed())
					{
						PUSHMOVEANDRULE(kMoveKingWhenJackIsAvailable);
						break;
					}
				}
			}

			for (uint32 i = 0; i < draggableCards.size(); i++)
			{
				if (draggableCards[i].fPileNumber == move.fSource)
					continue; // skip itself
				if (draggableCards[i].fPileNumber == kWastePile)
					continue; // skip waste pile

				// Determine whether there is another card the same as this card.
				if (card.GetNumber() == draggableCards[i].fCard.GetNumber() && card.IsRed() == draggableCards[i].fCard.IsRed())
				{
					// Which face down pile is bigger?
					if (gameState.GetPileSize(PILENUMBER(move.fSource-7)) < gameState.GetPileSize(PILENUMBER(draggableCards[i].fPileNumber-7)))
						// _T("k. Move tableau pile from smaller face down pile."),			// kMoveTableauPileFromSmaller
						PUSHMOVEANDRULE(kMoveTableauPileFromSmaller);
					else
						// _T("j. Move tableau pile from bigger face down pile."),				// kMoveTableauPileFromBigger
						PUSHMOVEANDRULE(kMoveTableauPileFromBigger);
					break;
				}
			}
		}
		else
		{
			// _T("e. Move partial tableau pile when it opens up move to foundation."),	// kMovePartialTableauPile
			PileNumber foundationPileNumber;
			if (gameState.IsAnyFoundationMove(move.fSource, move.fPosition-1, foundationPileNumber))
				PUSHMOVEANDRULE(kMovePartialTableauPile);
		}

		// _T("d. Move tableau pile."),										// kMoveTableauPile
		PUSHMOVEANDRULE(kMoveTableauPile);
	}
}

// Get the bottom draggable cards and the top waste pile card.
void GetBottomDraggableCards(const GameState &gameState, std::vector<CardAndPile> &draggableCards)
{
	Card card;
	if (gameState.GetCard(kWastePile, -1, card) == 0)
		draggableCards.push_back(CardAndPile(card, kWastePile));

	for (uint32 i = 0; i < 7; i++)
	{
		if (gameState.GetCard(PILENUMBER(kFaceUp0+i), 0, card) == 0)
			draggableCards.push_back(CardAndPile(card, PILENUMBER(kFaceUp0+i)));
	}
}

// Return true when cards that can play on the given card are in the foundation.
bool ArePlayableUp(const GameState &gameState, const Card &card)
{
	// If the card is red/black, check the two black/red piles for card-1.

	std::vector<Card> foundationTopCards;
	GetFoundationTopCards(gameState, foundationTopCards);

	uint32 countFound = 0;
	for (uint32 i = 0; i < foundationTopCards.size(); i++)
	{
		if (foundationTopCards[i].IsRed() != card.IsRed())
		{
			if (foundationTopCards[i].GetNumber()+1 < card.GetNumber())
				return false;
			countFound++;
		}
	}
	if (countFound < 2)
		return false;

	return true;
}

void GetFoundationTopCards(const GameState &gameState, std::vector<Card> &foundationTopCards)
{
	for (uint32 i = 0; i < 4; i++)
	{
		Card foundationTopCard;
		if (gameState.GetCard(PILENUMBER(kFoundation0+i), -1, foundationTopCard) == 0)
			foundationTopCards.push_back(foundationTopCard);
	}
}

// Read the strategy from the ini file.
void Strategy::ReadStrategy()
{
	IniFileAccess *iniFileAccess = SolitaireIniFile::GetIniFileAccess();

	TString strategyLetters;
	if (iniFileAccess->GetString(gStrategyStr, strategyLetters))
		return;

	LettersToStrategy(strategyLetters, fStrategy);
}

// Write the strategy to the ini file.
void Strategy::WriteStrategy()
{
	IniFileAccess *iniFileAccess = SolitaireIniFile::GetIniFileAccess();

	TString strategyLetters;
	GetStrategyLetters(strategyLetters);

	iniFileAccess->SetString(gStrategyStr, strategyLetters);
}

// Convert the strategy letters to a strategy list.
// a,b,c,d,e,f,...,r
int LettersToStrategy(const TString &strategyLetters, std::vector<Strategy::MoveRule> &strategy)
{
	std::vector<Strategy::MoveRule> temp;

	uint32 count = (uint32)strategyLetters.size();
	for (uint32 i = 0; i < count; i++)
	{
		TCHAR letter = strategyLetters[i];
		if (letter == (TCHAR)',')
			continue; // skip commas

		Strategy::MoveRule moveRule;
		if (LetterToMoveRule(letter, moveRule))
			return 1; // invalid letter

		// Make sure we haven't already used the rule.
		std::vector<Strategy::MoveRule>::const_iterator it = std::find(temp.begin(), temp.end(), moveRule);
		if (it != temp.end())
			return 1; // letter already used

		temp.push_back(moveRule);
	}

	if (temp.size() != Strategy::kMoveRuleCount)
		return 1;

	strategy = temp;
	return 0; // success
}

int LetterToMoveRule(TCHAR letter, Strategy::MoveRule &moveRule)
{
	if (letter < (TCHAR)'a' || letter > (TCHAR)'r')
		return 1; // invalid letter

	moveRule = (Strategy::MoveRule)(letter - (TCHAR)'a');
	return 0; // success
}

