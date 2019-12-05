// Copyright 2006-2019 Flenniken
// All Rights Reserved.

#pragma once

/** IniFile handles the ini file.
*/
class IniFileAccess
{
public:
	IniFileAccess(const TString &baseName, const TString &sectionName);
	~IniFileAccess();

	/** Get the name of the ini file.
	*/
//	void GetIniFile(TString &filename) const;

	/** Get the given key's bool value.
		@param key is the name of the item to get.
		@param return the key's value.
		@return 0 when successful, otherwise if the value is missing or 
		some other problem non-zero is returned.
	*/
	int GetBoolean(const TString &key, bool &value) const;

	/** Set the given key's bool value.
		@param key is the name of the item to set.
		@param value is the value to use.
		@return 0 when successful.
	*/
	int SetBoolean(const TString &key, bool value);

	int GetInt32(const TString &key, int32 &value) const;
	int SetInt32(const TString &key, int32 value);

	int GetUint32(const TString &key, uint32 &value) const;
	int SetUint32(const TString &key, uint32 value);

	int GetString(const TString &key, TString &value) const;
	int SetString(const TString &key, const TString &value);

	int GetStructure(const TString &key, uint8 *buffer, uint32 bufferSize) const;
	int SetStructure(const TString &key, uint8 *buffer, uint32 bufferSize);

private:
	TString fFilename;
	TString fSectionName;
};


class SolitaireIniFile
{
public:
	/** Get access to the solitaire ini file.
	*/
	static IniFileAccess *GetIniFileAccess();

};
