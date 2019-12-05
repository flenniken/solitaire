// Copyright 2007-2019 Flenniken
// All Rights Reserved.

#pragma once

class Card
{
public:
	enum Suit {kHearts, kDiamonds, kSpades, kClubs};

	enum Number {kAce, kTwo, kThree, kFour, kFive, kSix, kSeven, kEight, kNine, kTen, kJack, kQueen, kKing};

	Card() { Set(kAce, kHearts); }
	Card(Number number, Suit suit) { Set(number, suit); }

	Number GetNumber() const { return (Number)(fCard & 0xf); }
	Suit GetSuit() const {return (Suit)((fCard & 0x30) >> 4);  }
	bool IsRed() const {return (fCard & 0x20 ? false : true); } 

	// Get the card's number from 0 to 51, counting in suit order.
	uint32 GetIndex52() const
	{
		Number number = GetNumber();
		Suit suit = GetSuit();
		return (number + (suit * 13));
	}

private:
	void Set(Number number, Suit suit)
	{
		_ASSERT(number < 13);
		_ASSERT(suit < 4);

#ifdef DEBUG
		fNumber = number;
		fSuit = suit;
#endif
		fCard = (suit << 4);
		fCard += number;
	}

#ifdef DEBUG
	Number fNumber;
	Suit fSuit;
#endif
	uint8 fCard;
};


