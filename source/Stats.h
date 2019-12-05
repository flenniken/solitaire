// Copyright 2007-2019 Flenniken
// All Rights Reserved.

#pragma once

/*
	A game starts on the first move.
	It is over when a new game is dealt, or the user quits.
*/

class Stats
{
public:
	Stats();
	~Stats();

	void GameFinished(uint32 cardsUp);

	uint32 GetTotalGamesFinished() const {return fTotalGamesFinished;}
	uint32 GetCardsUp() const {return fCardsUp;}

	double GetAverageCardsUp() const
	{
		if (fTotalGamesFinished == 0)
			return 0.0;
		return (double)fTotalCardsUp / (double)fTotalGamesFinished;	
	}

	double GetPercentWon() const
	{
		if (fTotalGamesFinished == 0)
			return 0.0;
		return (double)fTotalGamesWon * 100.0 / (double)fTotalGamesFinished;	
	}

	uint32 GetGamesWon() const {return fTotalGamesWon;}

	void RunningTotal(uint32 cardsUp);

	void Resize(uint32 parentWidth, uint32 parentHeight);

	void GameOptions(bool dealOneAtATime, bool allowPartialPileMoves, bool oneTimeThrough);

	bool GetDealOneAtATime() const {return fDealOneAtATime;}
	bool GetAllowPartialPileMoves() const {return fAllowPartialPileMoves;}
	bool GetOneTimeThrough() const {return fOneTimeThrough;}

	void SetStrategyLetters(const TString &string);
	void GetStrategyLetters(TString &string) const {string = fStrategyLetters;}

	void GetGameString(TString &string) const {string = fGameString;}

	// Clear the stats.
	void Reset();

	// Read the stats from the ini file.
	void ReadStats();

	// Write the stats to the ini file.
	void WriteStats();

private:
	uint32 fTotalCardsUp;		// The number of cards moved to the foundation totaled for all games.
	uint32 fTotalGamesFinished;	// Number of completed games.
	uint32 fTotalGamesWon;		// Number of games won.
	uint32 fCardsUp;			// Number of cards in the foundation.

	bool fDealOneAtATime;
	bool fAllowPartialPileMoves;
	bool fOneTimeThrough;

	TString fStrategyLetters;
	TString fGameString;
};

class StatsUtils
{
public:
	static int CreateStatisticsWindow();
};
