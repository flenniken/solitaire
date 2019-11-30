// Copyright 2006-2007 Keypict
// All Rights Reserved.

#include "People.h"
#include "FileIO.h"
#include "stdlib.h"
#include "Utils.h" // for Utils::Swap

FileIO::FileIO() :
	fFile(nil)
{
}

FileIO::~FileIO()
{
	Close();
}

int FileIO::Open(const TString &filename, const TString &mode)
{
	if (fFile)
		return 1; // file already open.
	if (filename.empty())
		return 1; // nothing to open
	if (mode.empty())
		return 1; // need mode

	// Open the input file.
#if (_MSC_VER > 1310)
	if (_tfopen_s(&fFile, filename.c_str(), mode.c_str()))
		return 1; // Unable to open the file.
#else
	fFile = _tfopen(filename.c_str(), mode.c_str());
	if (!fFile)
		return 1; // Unable to open the file.
#endif

	return 0; // Success.
}

void FileIO::Close()
{
	if (!fFile)
		return;

	fclose(fFile);
	fFile = nil;
}

#if 0
bool FileIO::Exists(const TString &filename) const
{
	DWORD attributes = GetFileAttributes(filename.c_str());
	if (attributes == INVALID_FILE_ATTRIBUTES)
		return false;
	if (attributes == FILE_ATTRIBUTE_DIRECTORY)
		return false;
	return true;	
}
#endif

bool FileIO::Exists(const TString &filename) const
{
	if (_taccess(filename.c_str(), 0) != 0)
		return false;
	return true;
}

int FileIO::WriteBytes(const void *buffer, FileLength count) const
{
	if (!fFile)
		return 1;

	if (fwrite(buffer, 1, count, fFile) != count)
		return 1;

	return 0;
}

// Seek from the start of the file.
int FileIO::Seek(FileOffset offset) const
{
	if (!fFile)
		return 1;

	if (fseek(fFile, offset, SEEK_SET) != 0)
		return 1;

	return 0; // success
}

int FileIO::ReadBytes(void *buffer, FileLength count) const
{
	if (!fFile)
		return 1;

	if (fread(buffer, 1, count, fFile) != count)
		return 1; // problem reading
	return 0; // success
}

int FileIO::Read(TCHAR *buffer, FileLength count) const
{
	if (!fFile)
		return 1;

	if (fread(buffer, sizeof(TCHAR), count, fFile) != count)
		return 1; // error reading the bytes

	return 0; // success
}

int FileIO::ReadString(TString &string, FileLength count) const
{
	string.resize(count);

	return Read(&string[0], count);
}

int FileIO::Write(const TCHAR *buffer, FileLength count) const
{
	if (!fFile)
		return 1;

	if (fwrite(buffer, sizeof(TCHAR), count, fFile) != count)
		return 1;

	return 0;
}

int FileIO::WriteString(const TString &string) const
{
	return Write(string.c_str(), (FileLength)string.size());
}

FileIO::FileOffset FileIO::GetCurrentPosition() const
{
	if (!fFile)
		return 0;

	return (FileOffset)ftell(fFile);
}

FileIO::FileOffset FileIO::GetEndOfFile() const
{
	if (!fFile)
		return 0;

	fseek(fFile, 0, SEEK_END);
	return (FileOffset)ftell(fFile);
}

// Read a line of text. Return 0 when we get it.
int FileIO::ReadLine(TString &line) const
{
	// Read the file character by character looking for a 0x0d or 0x0a.

	int length = 1024*4; // Max line length.
	line.clear();

	for (int i = 0; i < length-1; i++)
	{
		TCHAR character;
		if (Read(&character, 1))
		{
			if (i == 0)
				return 1; // didn't get a line of characters

			return 0; // got a line of characters
		}

		if (character == 0x0d || character == 0x0a)
		{
			while (1)
			{
				// eat all the end of line characters and blank lines.
				if (Read(&character, 1))
					break; // ran out of characters.

				if (character != 0x0d && character != 0x0a)
				{
					// Found a character at the beginning of the next line, seek back one character
					// so the next read gets this character.
					Seek(GetCurrentPosition()-sizeof(TCHAR));
					break;
				}
			}
			if (i == 0)
				return 1; // didn't get a line of characters

			return 0; // got a line of characters
		}

		line += character;
	}

	return 1; // line too long
}

int FileIO::ReadBytes(UTF8String &buffer, FileIO::FileLength count) const
{
	buffer.resize(count);

	if (ReadBytes(&buffer[0], count))
		return 1; // problem reading

	return 0; // Success.
}

int FileIO::WriteBytes(const UTF8String &metadata) const
{
	if (WriteBytes(&metadata[0], (FileIO::FileLength)metadata.size()))
		return 1; // problem writing

	return 0; // Success.
}

int FileIO::ReadShort(unsigned short *value, bool swap) const
{
	if (ReadBytes(value, 2))
		return 1; // Error reading.

	if (swap)
		Utils::Swap(value);

	return 0; // Success.
}

int FileIO::ReadLong(unsigned long *value, bool swap) const
{
	if (ReadBytes(value, 4))
		return 1; // Error reading.

	if (swap)
		Utils::Swap(value);

	return 0; // Success.
}

int FileIO::SeekFromCurrent(unsigned long offset) const
{
	if (!fFile)
		return 1;

	if (fseek(fFile, offset, SEEK_CUR) != 0)
		return 1;

	return 0; // success
}
