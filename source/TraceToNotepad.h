// Copyright 2006-2019 Flenniken
// All Rights Reserved.

/** Simple class to display trace information to a notepad window.
	To use:
	* Open a notepad window to get a window called "Untitled - Notepad".
	* Call Connect to connect to the untitled window.
	* Call Write or WriteNewline to output text to the window.
*/

class TraceToNotepad
{
public:
	TraceToNotepad() : fEditWindow(0) {};
	~TraceToNotepad() {};

	/** Find an open Notepad window called Untitled.
	*/
	void Connect();

	/** Write the given string to the Notepad window found with Connect.
		@param string is the string to write at the current location (usually the end).
	*/
	void Write(const TString &string) const;

	/** Write a newline.
	*/
	void WriteNewline() const;

private:
	HWND fEditWindow;
};

#ifdef DEBUG
	#define TraceToNotepad_Object TraceToNotepad gTrace
	#define TraceToNotepad_Extern extern TraceToNotepad gTrace
	#define TraceToNotepad_Connect() gTrace.Connect()
	#define TraceToNotepad_Write(message) gTrace.Write(message)
	#define TraceToNotepad_WriteNewline() gTrace.WriteNewline()
#else
	#define TraceToNotepad_Object
	#define TraceToNotepad_Extern
	#define TraceToNotepad_Connect()
	#define TraceToNotepad_Write(message)
	#define TraceToNotepad_WriteNewline()
#endif

