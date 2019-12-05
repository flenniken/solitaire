// Copyright 2007-2019 Flenniken
// All Rights Reserved.

#pragma once

class OptionsUtils
{
public:
	/** Show a dialog to allow the user to choose which PlayUntil to use.
		@param dealOneAtATime IN/OUT true to deal one card at a time, else three.
		@param allowPartialPileMoves IN/OUT true to allow partial pile moves.
		@param oneTimeThrough IN/OUT true when you go through the deck once, else multiple times.
		@return true when the user cancels.
	*/
	static bool ShowDialog(bool &dealOneAtATime, bool &allowPartialPileMoves, bool &oneTimeThrough);

	// Read options from the ini file.
	static void ReadOptions();

	// Write options to the ini file.
	static void WriteOptions();

	static void GetOptions(bool &dealOneAtATime, bool &allowPartialPileMoves, bool &oneTimeThrough);
};
