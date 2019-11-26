// Copyright 2007 Keypict
// All Rights Reserved.

#pragma once

class BigCardWindow
{
public:
	static void Resize(uint32 width, uint32 height);
	static int ResizeCard(int32 direction);
	static void ShowBigCardWindow();
	static bool IsWindowVisible();
	static void ChangeCard(WPARAM wParam);
};
