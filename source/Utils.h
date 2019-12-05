// Copyright 2007-2019 Flenniken
// All Rights Reserved.

#pragma once

#include <vector>

class ResourceID;

#ifdef _WINDOWS_
#include <shlwapi.h>	// for IStream
#endif

/** Automatically release the specified object when this object goes out of scope.	
	Usage:
	IStream *stream;
	stream = ...
	AutoRelease<IStream> autoReleaseStream(stream);
*/
template<class T>
	class AutoRelease
	{
	public:
		AutoRelease(T *ptr) : fPtr(ptr) {}
		~AutoRelease() {if (fPtr) fPtr->Release();}
	private:
		T *fPtr;
	};

/** Automatically delete the specified object when this object goes out of scope.	
	Usage:
	Gdiplus::Image *image;
	image = ...
	AutoDelete<Gdiplus::Image> autoDeleteImage(image);
*/
template<class T>
	class AutoDelete
	{
	public:
		AutoDelete(T *ptr) : fPtr(ptr) {}
		~AutoDelete() {delete fPtr;}
	private:
		T *fPtr;
	};

class ISimpleBuffer
{
public:
	/** Copy the buffer to the given location.
	*/
	virtual void Copy(void *destinationBuffer) const = 0;

	/** Return the number of bytes in the buffer.
	*/
	virtual uint32 ByteCount() const = 0;
};

class SimpleBuffer : public ISimpleBuffer
{
public:
	SimpleBuffer() : fBuffer(nil), fByteCount(0) {}
	SimpleBuffer(const void *buffer, uint32 byteCount) : 
		fBuffer(buffer), 
		fByteCount(byteCount) 
	{
	}
	void Copy(void *destinationBuffer) const
	{
		memcpy(destinationBuffer, fBuffer, fByteCount);
	}
	uint32 ByteCount() const
	{
		return fByteCount;
	}
	const void *GetPtr() {return fBuffer;}

private:
	const void *fBuffer;
	uint32 fByteCount;
};

#ifdef _WINDOWS_
/** MySelectObject calls SelectObject for the given object in the constructor. 
	It de-selects the object in the destructor and deletes it.
	Usage example:
	MySelectObject selectPen(hdc, CreatePen(PS_SOLID, 1, RGB(100, 100, 100)));
*/

	// Automatically delete a file when going out of scope.
	// Call Release to prevent the file delete.
	class AutoDeleteFile
	{
	public:
		AutoDeleteFile(const TString &filename) : fFilename(filename){}
		~AutoDeleteFile()
		{
			if (!fFilename.empty())
				DeleteFile(fFilename.c_str());		
		}
		void Release() {fFilename.clear();}

	private:
		TString fFilename;
	};



	class MySelectObject
	{
	public:
		MySelectObject(HDC hdc, HGDIOBJ object) : fHDC(hdc), fObject(object)
		{
			fOldObject = SelectObject(fHDC, fObject); 
		}
		~MySelectObject()
		{
			SelectObject(fHDC, fOldObject); 
			DeleteObject(fObject);
		}
		HGDIOBJ Get(){return fObject;};
		HBRUSH GetAsBrush(){return (HBRUSH)fObject;};
	private:
		HDC fHDC;
		HGDIOBJ fObject;	
		HGDIOBJ fOldObject;	
	};

	class ResourceID
	{
	public:
		ResourceID(HINSTANCE theInstance, int32 ID, const TString &typeString) : 
		  instance(theInstance), id(ID), type(typeString) {}
		HINSTANCE instance;
		int32 id;
		TString type;
	};

#endif

class Utils
{
public:
	/** Parse a key set file line and return its components.
		@param line is the key set file line.
		@param delimiter is the delimiter to use, either a comma or a tab.
		@param keyName is the name of the key on the line.
		@param shiftState is the shift state of the line.
		@param assignment is the key assignment of the line.
	*/
//	static int GetLineComponents(const TString &line, TCHAR delimiter, TString &keyName, TString &shiftState, TString &assignment);

	/** Return the directory and base name of the given filename.
		@param filename is the filename to use.
		@param directory is the filename's directory ending with a slash.
		@param baseFilename is the filename's name without the directory.
	*/
	static void FileParts(const TString &filename, TString &directory, TString &baseFilename);

	/** Make a full path to the user data folder.
		@param baseName is a filename without directory information.
		@param fullPath is returned with a full path to the data folder.
	*/
	static void MakeFullPath(const TString &baseName, TString &fullPath);

	/** Swap bytes.
	*/
	static void Utils::Swap(unsigned short *value);
	static void Utils::Swap(unsigned long *value);

#ifdef _WINDOWS_
	/** Get a string from the resource file. See strings.rc.
		@param instance is the module instance.
		@param id is the id of the string to get.
		@return a string.
	*/
	static TString Utils::GetString(HINSTANCE instance, unsigned int id);
#endif

	/** Remove leading and trailing spaces from the string.
	*/
//	static void StripSpaces(TString &string);

	/** Return a filename that doesn't exist in the same directory 
		as the filename parameter.
		@param filename which specifies the directory to use.
		@param newFilename is returned with a new filename that doesn't exist.
		@param extension is the extension to use for the new file, i.e. _T(".jpg").
		@return 0 when successful.
	*/
	static int GetTempName(const TString &filename, TString &newFilename, const TString &extension);

	/** Get the list of files and sub directories in the specified directory.
		@param parentDirectory is the directory to look in.
		@param fileList is returned with all the files.
		@param subDirList is returned with all the sub directories.
	*/
	static void GetFileList(const TString &parentDirectory, std::vector<TString> &fileList, std::vector<TString> &subDirList);

	/** Get a resource as a string.
		@param ID is the resource ID to get.
		@param typeString is the resource type string.
		@param string is filled in with the resource string data. If there is an error,
		string is untouched.
	*/
	static void GetStringResource(const ResourceID &resourceID, TString &string);

	/** Return true when we are running on Windows 2000 or above.
	*/
	static bool IsWindows2000OrAbove();

	/** Get the bytes of a resource.
		@param id is the resource id.
		@param typeString is the resource type string.
		@param simpleBuffer is returned pointing to the resource bytes.
		@return 0 when successful.
	*/
	static int32 GetResource(const ResourceID &resourceID, SimpleBuffer &simpleBuffer);

	/** Create a filename of a standard folder.
		@param folderID is the ID of a standard folder.
			CSIDL_COMMON_PROGRAMS is the standard menu folder.
			CSIDL_DESKTOP is the desktop folder
			CSIDL_PROGRAM_FILES is the "Program Files" folder.
			CSIDL_APPDATA is the users roaming data folder.
		@param name is the name of a file to append to the standard folder. Name may be empty.
		@param filename is returned with the name appended to the standard folder. On error it is untouched.
	*/
	static void GetStandardFilename(int32 folderID, const TString &name, TString &filename);

	/** Combine a path and a name to create a full path.
		@param folder is a folder name, with or without an ending slash.
		@param name is the name to append. If name is empty, the returned folder
			will have an ending slash.
		@param filename is returned with the result.  It is untouched on an error.
	*/
	static void AppendName(const TString &folder, const TString &name, TString &filename);

	// Return the size of the file.
	static uint64 GetFileSize(const TString &filename);

	// Get the full path to the running exe.
	static void GetExeFilename(TString &exeFilename);

	// Create a number string.
	static TString GetNumberString(uint64 number);

	// Create a number string.
	static TString GetNumberString(int32 number);

	/** Replace part of a string with another string.
		@param message is the string to change.
		@param findString is a sub-string of message to be replaced.
		@param replacementString is the replacement string.
	*/
	static void ReplaceString(TString &message, const TString &findString, const TString &replacementString);

	/** Get an array of keywords.
		@param keywords is a keyword comma separated string. You can pass any string.
		@param keys is returned with an array of strings one for each keyword.
		The leading and trailing space is removed from each keyword. Extra commas
		are ignored.
	*/
	static void ParseKeywords(const TString &keywords, std::vector<TString> &keys);
	static void ParseKeywords(const TString &keywords, std::vector<UTF8String> &keys);

	/** Make a keyword string from an array of keywords.
		@param keys is an array of strings one for each keyword.
		@param keywords is a keyword comma separated string.
	*/
	static void MakeKeywordString(const std::vector<TString> &keys, TString &keywords);

	/** Copy a string to a buffer. 
		If the string is longer than the buffer, the buffer is filled with as many
		characters as possible including an ending 0.
		@param string to copy
		@param buffer points to the location to copy the string.
		@param bufferCount is the number of bytes in the buffer.
		@return a pointer to the buffer.
	*/
	static TCHAR *CopyString(const TString &string, TCHAR *buffer, uint32 bufferSize);

	/** Set the main window title.
		@param metadataTitle is the title from the metadata.
		@param filename is the current filename being displayed.
		@param message is a message to show at the end of the title.
	*/
	static void SetWindowTitle(const TString &metadataTitle, const TString &filename, const TString &message);

	/** Orient the width and height. Swap the width and height when needed.
		@param orientation is the TIFF standard orientation value.
		@param width is an in/out parameter. It's the width of the image.
		@param height is an in/out parameter. It's the height of the image.
	*/
	static void Orient(uint32 orientation, uint32 &width, uint32 &height);

	/** Open the file, read bytes, then close it.
		@param filename is the file to read.
		@param fileOffset is the offset to begin reading.
		@param length is the number of bytes to read.
		@param data is filled with the bytes read.
		@return 0 when successful.
	*/
	static int OpenReadClose(const TString &filename, uint32 fileOffset, uint32 length, UTF8String &data);

#ifdef _WINDOWS_
	// Display a message box. See Windows MessageBox.
	static int32 MessageBox(HWND owner, const TString &message, uint32 flags);

	/** Convert an ITEMIDLIST to a TString folder with an ending slash.
		@param itemIDList is the item list to convert.
		@param folder is returned with the item's folder.
		folder is untouched on an error.
	*/
	static void GetFolderFromItemIDList(ITEMIDLIST *itemIDList, TString &myPicturesFolder);

	// Free an ITEMIDLIST.
	static void FreeItemIDList(ITEMIDLIST *itemIDList);

	/** Create a stream out of a resource.
		@param id is the resource ID to get.
		@param typeString is the resource type string.
		@return a pointer to an IStream for the resource. 
		Release the stream when you are done with it.
	*/
	static IStream *CreateStreamFromResource(const ResourceID &resourceID);

	/** Center window over its owner window.
	*/
	static void CenterWindow(HWND hwndChild);

	/** Create a stream from a buffer of memory.
		@param simpleBuffer is the buffer to make a stream out of. See SimpleBuffer class.
		@return a pointer to a stream or nil.
	*/
	static IStream *BufferToStream(const ISimpleBuffer &simpleBuffer);

	/** Get the editbox text.
		@param hDlg is the editbox's dialog window.
		@param editboxID is the editbox ID.
		@param string is returned with the editbox text.
	*/
	static void GetEditboxText(HWND hDlg, int editboxID, TString &string);

	/** Open the file in it's default editor.
		@param filename is the full path to the file.
	*/
	static void OpenInDefault(const TString &filename);

	/** Get size and position of rectangles for the current file and the total files.
		@param hdc is the device context.
		@param current is the number of the current file in the current directory.
		@param total is the total number of files in the current directory.
		@param bigBox is the area around other boxes.
		@param starBox is the area for the star ratings.
		@param countBox is the area for the current file.
		@param totalBox is the area for the total count.
	*/
	static void GetCountBoxes(HDC hdc, int current, int total, RECT &bigBox, RECT &starBox, RECT &countBox, RECT &totalBox);

	/** Return the area for the read only and modified icon.
	*/
	static void GetIconBox(RECT &iconBox);
#endif

	/** Find a filename in a sorted list of filenames.
		@param list is a sorted list of filenames.
		@param filename is the filename to find in the list.
		@return the index to file found item or -1 when not found.
	*/
	static int FindFile(const std::vector<TString> &list, const TString &filename);

	/** Compare two filenames.
		@param one is a filename.
		@param two is a filename.
		@return true when filename one is less than filename two.
	*/
	static bool CompareFilenamesLess(const TString &one, const TString &two);

	/** Compare two filenames.
		@param one is a filename.
		@param two is a filename.
		@return true when filename one is the same as filename two.
	*/
	static bool CompareFilenames(const TString &one, const TString &two);

};

