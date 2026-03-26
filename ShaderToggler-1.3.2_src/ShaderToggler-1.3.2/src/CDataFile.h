//
// CDataFile Class Implementation
//
// The purpose of this class is to provide a simple, full featured means to
// store persistent data to a text file.  It uses a simple key/value paradigm
// to achieve this.  The class can read/write to standard Windows .ini files,
// and yet does not rely on any windows specific calls.  It should work as
// well in a linux environment (with some minor adjustments) as it does in
// a Windows one.
//
// Written July, 2002 by Gary McNickle <gary#sunstorm.net>
// If you use this class in your application, credit would be appreciated.
//
#pragma once

#include "stdafx.h"
#include <vector>
#include <fstream>
#include <string>
#include <cstdint>

using namespace std;

// Globally defined structures, defines, & types
//////////////////////////////////////////////////////////////////////////////////

#define AUTOCREATE_SECTIONS     (1L<<1)
#define AUTOCREATE_KEYS         (1L<<2)
#define MAX_BUFFER_LEN          512

enum e_DebugLevel
{
	E_DEBUG = 0,
	E_INFO,
	E_WARN,
	E_ERROR,
	E_FATAL,
	E_CRITICAL
};

typedef std::string t_Str;

const t_Str CommentIndicators = t_Str(";#");
const t_Str EqualIndicators   = t_Str("=:");
const t_Str WhiteSpace        = t_Str(" \t\n\r");

typedef struct st_key
{
	t_Str szKey;
	t_Str szValue;
	t_Str szComment;

	st_key()
	{
		szKey = t_Str("");
		szValue = t_Str("");
		szComment = t_Str("");
	}

} t_Key;

typedef std::vector<t_Key> KeyList;
typedef KeyList::iterator KeyItor;

typedef struct st_section
{
	t_Str szName;
	t_Str szComment;
	KeyList Keys;

	st_section()
	{
		szName = t_Str("");
		szComment = t_Str("");
		Keys.clear();
	}

} t_Section;

typedef std::vector<t_Section> SectionList;
typedef SectionList::iterator SectionItor;

void    Report(e_DebugLevel DebugLevel, const char *fmt, ...);
t_Str   GetNextWord(t_Str& CommandLine);
int     CompareNoCase(t_Str str1, t_Str str2);
void    Trim(t_Str& szStr);
int     WriteLn(fstream& stream, const char* fmt, ...);

class CDataFile
{
public:
	CDataFile();
	CDataFile(t_Str szFileName);
	virtual ~CDataFile();

	bool Load(t_Str szFileName);
	bool Save();

	t_Str    GetValue(t_Str szKey, t_Str szSection = t_Str(""));
	t_Str    GetString(t_Str szKey, t_Str szSection = t_Str(""));
	float    GetFloat(t_Str szKey, t_Str szSection = t_Str(""));
	int      GetInt(t_Str szKey, t_Str szSection = t_Str(""));
	uint32_t GetUInt(t_Str szKey, t_Str szSection = t_Str(""));
	bool     GetBool(t_Str szKey, t_Str szSection = t_Str(""));

	bool SetValue(t_Str szKey, t_Str szValue,
		t_Str szComment = t_Str(""), t_Str szSection = t_Str(""));

	bool SetFloat(t_Str szKey, float fValue,
		t_Str szComment = t_Str(""), t_Str szSection = t_Str(""));

	bool SetInt(t_Str szKey, int nValue,
		t_Str szComment = t_Str(""), t_Str szSection = t_Str(""));

	bool SetUInt(t_Str szKey, uint32_t nValue,
		t_Str szComment = t_Str(""), t_Str szSection = t_Str(""));

	bool SetBool(t_Str szKey, bool bValue,
		t_Str szComment = t_Str(""), t_Str szSection = t_Str(""));

	bool SetKeyComment(t_Str szKey, t_Str szComment, t_Str szSection = t_Str(""));
	bool SetSectionComment(t_Str szSection, t_Str szComment);

	bool DeleteKey(t_Str szKey, t_Str szFromSection = t_Str(""));
	bool DeleteSection(t_Str szSection);

	bool CreateKey(t_Str szKey, t_Str szValue,
		t_Str szComment = t_Str(""), t_Str szSection = t_Str(""));

	bool CreateSection(t_Str szSection, t_Str szComment = t_Str(""));
	bool CreateSection(t_Str szSection, t_Str szComment, KeyList Keys);

	int  SectionCount();
	int  KeyCount();
	void Clear();
	void SetFileName(t_Str szFileName);
	t_Str CommentStr(t_Str szComment);

	// -----------------------------------------------------------------
	// Added compatibility helpers for ShaderToggler Advanced
	// -----------------------------------------------------------------
	std::vector<uint32_t> GetArray(t_Str szKey, t_Str szSection = t_Str(""));
	bool SetArray(t_Str szKey, const std::vector<uint32_t>& values,
		t_Str szComment = t_Str(""), t_Str szSection = t_Str(""));
	bool SetString(t_Str szKey, t_Str szValue,
		t_Str szComment = t_Str(""), t_Str szSection = t_Str(""));

protected:
	t_Key* GetKey(t_Str szKey, t_Str szSection);
	t_Section* GetSection(t_Str szSection);

public:
	long m_Flags;

protected:
	SectionList m_Sections;
	t_Str m_szFileName;
	bool m_bDirty;
};