
#define _CRT_RAND_S
#include "Windows.h"
#include "TChar.h"
#include "stdio.h"
#include <vector>
#include "people.h"
#include <time.h>
#include <algorithm>
#include "ElaspedSeconds.h"
#include "math.h"
#include <stdlib.h>


class CardItem
{
public:
	CardItem() : fCard(0), fOrder(0) {}
	CardItem(int8 card) : fCard(card), fOrder(0) {}
	~CardItem() {}

	bool operator< (const CardItem &cardItem) const
	{
		if (fOrder < cardItem.fOrder)
			return true;
		return false;
	}

	uint8 fCard;
	uint32 fOrder;
};

static void SortShuffle(std::vector<CardItem> &deck);
static void AddRandomCards(std::vector<CardItem> &deck);
static void KnuthShuffle(std::vector<CardItem> &deck);
static void KnuthShuffle2(std::vector<CardItem> &deck);
static void STLRandomShuffle(std::vector<CardItem> &deck);

static int CountMatches(std::vector<CardItem> &deck1, std::vector<CardItem> &deck2);
static void ShuffleMany(const TString &string, void shuffle(std::vector<CardItem> &deck));

static uint32 RandomNumber1();
static uint32 RandomNumber2();

#define RANDOMNUMBER RandomNumber1
static uint32 gIterations = 1000;
#define kNumberOfCards 52

int main(int argc, char *argv[])
{
	if (argc == 2)
		gIterations = atoi(argv[1]);

	_tprintf(_T("%d Shuffles\n"), gIterations);

	srand((unsigned)time(NULL));

//	ShuffleMany(_T("Sort Shuffle"), SortShuffle);
//	ShuffleMany(_T("Add Random Cards"), AddRandomCards);
	ShuffleMany(_T("STL Random Shuffle"), STLRandomShuffle);
	ShuffleMany(_T("Knuth Shuffle"), KnuthShuffle);
}


void ShuffleMany(const TString &string, void shuffle(std::vector<CardItem> &deck))
{
	std::vector<CardItem> deck;
	std::vector<CardItem> deckCopy;

	// Create the deck.
	for (uint8 i = 0; i < kNumberOfCards; i++)
		deck.push_back(i);

	ElaspedSeconds theTime;
	theTime.StartTiming();
	
	int matches = 0;
	for (uint32 i = 0; i < gIterations; i++)
	{
		deckCopy = deck;
		shuffle(deck);
		matches += CountMatches(deckCopy, deck);
	}

	theTime.StopTiming();

#if 0
	// Output the deck.
	int32 count = (int32)deck.size();
	for (int i = 0; i < count; i++)
		_tprintf(_T("%d "), deck[i].fCard);
	_tprintf(_T("\n"));
#endif


	// Output the time to shuffle including counting the matches.
	double seconds = theTime.GetSeconds();

	// On average one card should be in the same position as the previous deck. Each card has 1/52 change of being
	// in the same place and there are 52 cards.
	int32 matchesDelta = matches - gIterations;

	// Output the shuffle name.
	_tprintf(_T("%-20s Matches Delta=%5d, Seconds= %f\n"), string.c_str(), matchesDelta, seconds);
}

// Sort Shuffle
void SortShuffle(std::vector<CardItem> &deck)
{
	int32 count = (int32)deck.size();
	for (int i = 0; i < count; i++)
		deck[i].fOrder = RANDOMNUMBER();

	std::sort(deck.begin(), deck.end());

}

// Add random cards to a deck.
void AddRandomCards(std::vector<CardItem> &deck)
{
	// Create a deck of 52 cards.
	std::vector<CardItem> list;
	list = deck;
	deck.clear();

	// Remove all the cards at random and build up a new deck.
	int i;
	for (i = kNumberOfCards; i > 1; i--)
	{
		// Generate a random index into the remaining items in the list.
		uint32 index = RANDOMNUMBER()%i;

//		if (index < 0 || index >= (int)list.size())
//			return;

		// Add the random card to the new deck.
		deck.push_back(list[index]);

		// Remove the card.
		list.erase(list.begin()+index);
	}
	deck.push_back(list[0]);
}

// Knuth
void KnuthShuffle(std::vector<CardItem> &deck)
{
	for (int i = 0; i < kNumberOfCards; i++) 
	{
		int randomIndex = i + RandomNumber1()%(kNumberOfCards - i);

		CardItem tmp = deck[i];
		deck[i] = deck[randomIndex];
		deck[randomIndex] = tmp;
	}	
}

// STL random shuffle
void STLRandomShuffle(std::vector<CardItem> &deck)
{
	random_shuffle(deck.begin(), deck.end());
}

int CountMatches(std::vector<CardItem> &deck1, std::vector<CardItem> &deck2) 
{
	int count = (int)deck1.size();
	if (count != (int)deck2.size())
		return 0;

	int numMatches = 0;
	for (int i = 0; i < count; i++) 
	{
		if (deck1[i].fCard == deck2[i].fCard) 
			numMatches++;
	}

	return numMatches;
}

uint32 RandomNumber1()
{
	return rand();
}

uint32 RandomNumber2()
{
	unsigned int number;
	rand_s(&number);
	return number;
}
