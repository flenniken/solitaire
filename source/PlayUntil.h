// Copyright 2006-2007 Keypict
// All Rights Reserved.

#pragma once

class PlayUntilUtils
{
public:
	enum Type
	{
		kGames,		// number of games
		kCardsUp,	// number of cards up
		kWins,		// number of wins
		kCount		// count to types
	};

	/** Show a dialog to allow the user to choose which PlayUntil to use.
		@param type is returned with the type. 
		@return true when the user cancels.
	*/
	static bool ShowDialog(uint32 &number, Type &type);
};
