// Copyright 2006-2019 Flenniken

#pragma once

class FileIO
{
public:
	typedef unsigned long FileOffset;
	typedef unsigned long FileLength;

	FileIO();
	~FileIO();

	/** Open a file.  Open must be called before reading,
		writing, seeking etc.
		@param filename is the name of the file to open.
		@return 0 when the file was successfully opened.
	*/
	int Open(const TString &filename, const TString &mode);

	/** Return true when the file exists.
		@param filename is the name of the file to test.
		@return true when the file exists.
	*/
	bool Exists(const TString &filename) const;

	/** Close the file. 
		Note: The file is automatically closed when the class goes out of scope.
	*/
	void Close();

	/** Read characters from the file.
		@param buffer is the place to put the characters read.
		@param FileLength is the number of characters to read.
		@return 0 when successful.
	*/
	int ReadBytes(void *buffer, FileLength count) const;
	int Read(TCHAR *buffer, FileLength count) const;
	int ReadString(TString &string, FileLength count) const;
	int ReadLine(TString &line) const;
	int ReadBytes(UTF8String &buffer, FileIO::FileLength count) const;
	int ReadShort(unsigned short *value, bool swap) const;
	int ReadLong(unsigned long *value, bool swap) const;

	/** Write characters to the file.
		@param buffer contains the characters to write.
		@param length is the number of characters to write.
		@return 0 when the characters were written sucessfully.
	*/
	int WriteBytes(const void *buffer, FileLength count) const;
	int Write(const TCHAR *buffer, FileLength count) const;
	int WriteString(const TString &string) const;
	int WriteBytes(const UTF8String &metadata) const;

	/** Seek to the given character position in the file.
		@param offset is the position to seek to starting from the beginning of the file.
		@return 0 when successful.
	*/
	int Seek(FileOffset offset) const;
	int SeekFromCurrent(unsigned long offset) const;

	/** Get the current character position.  This is the position where the next 
		read or write will happen.
		@return the current file position.  Return 0 on error.
	*/
	FileOffset GetCurrentPosition() const;

	/** Gets the end of file character offset.  This is the number of character in
		the file.
		@return end of file character offset.  Return 0 on error.
	*/
	FileOffset GetEndOfFile() const;

	/** Return true when the file is open.
	*/
	bool IsOpen() {return fFile ? true : false;}

	/** Return the underlining FILE pointer.
	*/
	FILE *GetFile() {return fFile;}

private:
	FILE *fFile;
};
