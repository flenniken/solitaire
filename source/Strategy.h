// Copyright 2007-2019 Flenniken
// All Rights Reserved.

#pragma once

class Strategy
{
public:
	// Move rules
	enum MoveRule
	{
		kMoveWasteToFoundation,			// Move waste card to foundation.
		kMoveTableauToFoundation,		// Move tableau card to foundation.
		kMoveWasteToTableau,			// Move waste card to tableau.
		kMoveTableauPile,				// Move tableau pile.
		kMovePartialTableauPile,		// Move partial tableau pile when it opens up move to foundation.
		kFlipTabeauFaceDownCard	,		// Flip tableau face down card.
		kFlipDeckCards,					// Flip deck card(s).
		kMoveWasteAceOrTwo,				// Move waste ace or two to foundation.
		kMoveTableauAceOrTwo,			// Move tableau ace or two to foundation.
		kMoveTableauPileFromBigger,		// Move tableau pile from bigger face down pile.
		kMoveTableauPileFromSmaller,	// Move tableau pile from smaller face down pile.
		kMoveTableauToFoundationPlay,	// Move tableau card to foundation when its playable cards are up already.
		kMoveTableauPileMakingKingSpot,	// Move tableau pile making empty spot when king move exists.
		kMoveKingWhenJackIsAvailable,	// Move king when its jack is available.
		kMoveTableauPileUnlessEmpty,	// Move tableau pile unless it leaves an empty spot.
		kFlipDeckWhenWastePileEmpty,	// Flip deck card(s) if waste pile is empty.
		kFlipWastePileMakingNewStock,	// Filp waste pile making new stock.
		kMoveTableauToFoundationOpeningFlip, // Move tableau card to foundation when it on a face down card.
//		kStopBarrier,					// Ignore rules after this one.
		kMoveRuleCount					// The number of MoveRules.
	};

	Strategy();
	~Strategy();

	/** Show a dialog to allow the user to choose which strategy to use.
	*/
	static void ShowDialog();

	void MoveRuleToString(Strategy::MoveRule moveRule, TString &string);
	void GetStrategyLetters(TString &strategyLetters) const;
	void GetStrategy(std::vector<MoveRule> &list) const { list = fStrategy;}

	/** Pick a move based on the current strategy.
		@param moveList is the list of possible moves.
		@param move is returned with the choosen move.
		@return 0 when successful.
	*/
	int PickMove(const GameState &gameState, const std::vector<PossibleMove> &moveList, PossibleMove &move) const;

	// Read the strategy from the ini file.
	void ReadStrategy();

	// Write the strategy to the ini file.
	void WriteStrategy();

private:
	static INT_PTR CALLBACK StrategyDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	void InitializeDialog(HWND hDlg);
	void SaveData(HWND hDlg);


	std::vector<Strategy::MoveRule> fStrategy;
};
