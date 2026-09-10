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
#include "stdafx.h"
#include <vector>
#include <string>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <fstream>
#include <float.h>
#include <sstream>
#ifdef WIN32
#include <windows.h>
#endif
#include "CDataFile.h"
#ifdef WIN32
  #define snprintf _snprintf
  #define vsnprintf _vsnprintf
#endif
CDataFile::CDataFile(const std::filesystem::path& fileName)
{
 m_bDirty = false;
 m_szFileName = fileName;
 m_Flags = (AUTOCREATE_SECTIONS | AUTOCREATE_KEYS);
 m_Sections.push_back(*(new t_Section));
 Load(m_szFileName);
}
CDataFile::CDataFile()
{
 Clear();
 m_Flags = (AUTOCREATE_SECTIONS | AUTOCREATE_KEYS);
 m_Sections.push_back(*(new t_Section));
}
CDataFile::~CDataFile()
{
 if (m_bDirty)
  Save();
}
void CDataFile::Clear()
{
 m_bDirty = false;
 m_szFileName.clear();
 m_Sections.clear();
}
void CDataFile::SetFileName(const std::filesystem::path& fileName)
{
 if (!m_szFileName.empty() && m_szFileName != fileName)
 {
  m_bDirty = true;
  Report(E_WARN, "[CDataFile::SetFileName] The filename has changed.");
 }
 m_szFileName = fileName;
}
bool CDataFile::Load(const std::filesystem::path& fileName)
{
 fstream File(fileName, ios::in);
 if (File.is_open())
 {
  bool bDone = false;
  bool bAutoKey = (m_Flags & AUTOCREATE_KEYS) == AUTOCREATE_KEYS;
  bool bAutoSec = (m_Flags & AUTOCREATE_SECTIONS) == AUTOCREATE_SECTIONS;
  t_Str szLine;
  t_Str szComment;
  char buffer[MAX_BUFFER_LEN];
  t_Section* pSection = GetSection("");
  m_Flags |= AUTOCREATE_KEYS;
  m_Flags |= AUTOCREATE_SECTIONS;
  while (!bDone)
  {
   memset(buffer, 0, MAX_BUFFER_LEN);
   File.getline(buffer, MAX_BUFFER_LEN);
   szLine = buffer;
   Trim(szLine);
   bDone = (File.eof() || File.bad() || File.fail());
   if (szLine.find_first_of(CommentIndicators) == 0)
   {
    szComment += "\n";
    szComment += szLine;
   }
   else if (szLine.find_first_of('[') == 0)
   {
    szLine.erase(0, 1);
    szLine.erase(szLine.find_last_of(']'), 1);
    CreateSection(szLine, szComment);
    pSection = GetSection(szLine);
    szComment = t_Str("");
   }
   else if (szLine.size() > 0)
   {
    t_Str szKey = GetNextWord(szLine);
    t_Str szValue = szLine;
    if (szKey.size() > 0 && szValue.size() > 0)
    {
     SetValue(szKey, szValue, szComment, pSection->szName);
     szComment = t_Str("");
    }
   }
  }
  if (!bAutoKey)
   m_Flags &= ~AUTOCREATE_KEYS;
  if (!bAutoSec)
   m_Flags &= ~AUTOCREATE_SECTIONS;
 }
 else
 {
  Report(E_INFO, "[CDataFile::Load] Unable to open file. Does it exist?");
  return false;
 }
 File.close();
 return true;
}
bool CDataFile::Save()
{
 if (KeyCount() == 0 && SectionCount() == 0)
 {
  Report(E_INFO, "[CDataFile::Save] Nothing to save.");
  return false;
 }
 if (m_szFileName.empty())
 {
  Report(E_ERROR, "[CDataFile::Save] No filename has been set.");
  return false;
 }
 fstream File(m_szFileName, ios::out | ios::trunc);
 if (File.is_open())
 {
  SectionItor s_pos;
  KeyItor k_pos;
  t_Section Section;
  t_Key Key;
  for (s_pos = m_Sections.begin(); s_pos != m_Sections.end(); s_pos++)
  {
   Section = (*s_pos);
   bool bWroteComment = false;
   if (Section.szComment.size() > 0)
   {
    bWroteComment = true;
    WriteLn(File, "\n%s", CommentStr(Section.szComment).c_str());
   }
   if (Section.szName.size() > 0)
   {
    WriteLn(File, "%s[%s]",
     bWroteComment ? "" : "\n",
     Section.szName.c_str());
   }
   for (k_pos = Section.Keys.begin(); k_pos != Section.Keys.end(); k_pos++)
   {
    Key = (*k_pos);
    if (Key.szKey.size() > 0 && Key.szValue.size() > 0)
    {
     WriteLn(File, "%s%s%s%s%c%s",
      Key.szComment.size() > 0 ? "\n" : "",
      CommentStr(Key.szComment).c_str(),
      Key.szComment.size() > 0 ? "\n" : "",
      Key.szKey.c_str(),
      EqualIndicators[0],
      Key.szValue.c_str());
    }
   }
  }
 }
 else
 {
  Report(E_ERROR, "[CDataFile::Save] Unable to save file.");
  return false;
 }
 File.flush();
 const bool written = File.good();
 File.close();
 if (!written || File.fail())
  return false;
 m_bDirty = false;
 return true;
}
bool CDataFile::SetKeyComment(t_Str szKey, t_Str szComment, t_Str szSection)
{
 KeyItor k_pos;
 t_Section* pSection;
 if ((pSection = GetSection(szSection)) == NULL)
  return false;
 for (k_pos = pSection->Keys.begin(); k_pos != pSection->Keys.end(); k_pos++)
 {
  if (CompareNoCase((*k_pos).szKey, szKey) == 0)
  {
   (*k_pos).szComment = szComment;
   m_bDirty = true;
   return true;
  }
 }
 return false;
}
bool CDataFile::SetSectionComment(t_Str szSection, t_Str szComment)
{
 SectionItor s_pos;
 for (s_pos = m_Sections.begin(); s_pos != m_Sections.end(); s_pos++)
 {
  if (CompareNoCase((*s_pos).szName, szSection) == 0)
  {
   (*s_pos).szComment = szComment;
   m_bDirty = true;
   return true;
  }
 }
 return false;
}
bool CDataFile::SetValue(t_Str szKey, t_Str szValue, t_Str szComment, t_Str szSection)
{
 t_Key* pKey = GetKey(szKey, szSection);
 t_Section* pSection = GetSection(szSection);
 if (pSection == NULL)
 {
  if (!(m_Flags & AUTOCREATE_SECTIONS) || !CreateSection(szSection, ""))
   return false;
  pSection = GetSection(szSection);
 }
 if (pSection == NULL)
  return false;
 if (pKey == NULL && szValue.size() > 0 && (m_Flags & AUTOCREATE_KEYS))
 {
  pKey = new t_Key;
  pKey->szKey = szKey;
  pKey->szValue = szValue;
  pKey->szComment = szComment;
  m_bDirty = true;
  pSection->Keys.push_back(*pKey);
  return true;
 }
 if (pKey != NULL)
 {
  pKey->szValue = szValue;
  pKey->szComment = szComment;
  m_bDirty = true;
  return true;
 }
 return false;
}
bool CDataFile::SetFloat(t_Str szKey, float fValue, t_Str szComment, t_Str szSection)
{
 char szStr[64];
 _snprintf_s(szStr, 64, "%f", fValue);
 return SetValue(szKey, szStr, szComment, szSection);
}
bool CDataFile::SetInt(t_Str szKey, int nValue, t_Str szComment, t_Str szSection)
{
 char szStr[64];
 _snprintf_s(szStr, 64, "%d", nValue);
 return SetValue(szKey, szStr, szComment, szSection);
}
bool CDataFile::SetUInt(t_Str szKey, uint32_t nValue, t_Str szComment, t_Str szSection)
{
 char szStr[64];
 _snprintf_s(szStr, 64, "%u", nValue);
 return SetValue(szKey, szStr, szComment, szSection);
}
bool CDataFile::SetBool(t_Str szKey, bool bValue, t_Str szComment, t_Str szSection)
{
 t_Str szValue = bValue ? "True" : "False";
 return SetValue(szKey, szValue, szComment, szSection);
}
bool CDataFile::SetString(t_Str szKey, t_Str szValue, t_Str szComment, t_Str szSection)
{
 return SetValue(szKey, szValue, szComment, szSection);
}
std::vector<uint32_t> CDataFile::GetArray(t_Str szKey, t_Str szSection)
{
 std::vector<uint32_t> result;
 t_Str szValue = GetValue(szKey, szSection);
 if (szValue.size() == 0)
  return result;
 std::stringstream ss(szValue);
 std::string item;
 while (std::getline(ss, item, ','))
 {
  Trim(item);
  if (item.size() == 0)
   continue;
  result.push_back(static_cast<uint32_t>(strtoul(item.c_str(), nullptr, 10)));
 }
 return result;
}
bool CDataFile::SetArray(t_Str szKey, const std::vector<uint32_t>& values, t_Str szComment, t_Str szSection)
{
 std::stringstream ss;
 for (size_t i = 0; i < values.size(); i++)
 {
  if (i > 0)
   ss << ",";
  ss << values[i];
 }
 return SetValue(szKey, ss.str(), szComment, szSection);
}
t_Str CDataFile::GetValue(t_Str szKey, t_Str szSection)
{
 t_Key* pKey = GetKey(szKey, szSection);
 return (pKey == NULL) ? t_Str("") : pKey->szValue;
}
t_Str CDataFile::GetString(t_Str szKey, t_Str szSection)
{
 return GetValue(szKey, szSection);
}
float CDataFile::GetFloat(t_Str szKey, t_Str szSection)
{
 t_Str szValue = GetValue(szKey, szSection);
 if (szValue.size() == 0)
  return FLT_MIN;
 return (float)atof(szValue.c_str());
}
int CDataFile::GetInt(t_Str szKey, t_Str szSection)
{
 t_Str szValue = GetValue(szKey, szSection);
 if (szValue.size() == 0)
  return INT_MIN;
 return atoi(szValue.c_str());
}
uint32_t CDataFile::GetUInt(t_Str szKey, t_Str szSection)
{
 t_Str szValue = GetValue(szKey, szSection);
 if (szValue.size() == 0)
  return UINT_MAX;
 return static_cast<uint32_t>(atoll(szValue.c_str()));
}
bool CDataFile::GetBool(t_Str szKey, t_Str szSection)
{
 bool bValue = false;
 t_Str szValue = GetValue(szKey, szSection);
 if (szValue.find("1") == 0
  || CompareNoCase(szValue, "true") == 0
  || CompareNoCase(szValue, "yes") == 0)
 {
  bValue = true;
 }
 return bValue;
}
bool CDataFile::DeleteSection(t_Str szSection)
{
 SectionItor s_pos;
 for (s_pos = m_Sections.begin(); s_pos != m_Sections.end(); s_pos++)
 {
  if (CompareNoCase((*s_pos).szName, szSection) == 0)
  {
   m_Sections.erase(s_pos);
   return true;
  }
 }
 return false;
}
bool CDataFile::DeleteKey(t_Str szKey, t_Str szFromSection)
{
 KeyItor k_pos;
 t_Section* pSection;
 if ((pSection = GetSection(szFromSection)) == NULL)
  return false;
 for (k_pos = pSection->Keys.begin(); k_pos != pSection->Keys.end(); k_pos++)
 {
  if (CompareNoCase((*k_pos).szKey, szKey) == 0)
  {
   pSection->Keys.erase(k_pos);
   return true;
  }
 }
 return false;
}
bool CDataFile::CreateKey(t_Str szKey, t_Str szValue, t_Str szComment, t_Str szSection)
{
 bool bAutoKey = (m_Flags & AUTOCREATE_KEYS) == AUTOCREATE_KEYS;
 bool bReturn = false;
 m_Flags |= AUTOCREATE_KEYS;
 bReturn = SetValue(szKey, szValue, szComment, szSection);
 if (!bAutoKey)
  m_Flags &= ~AUTOCREATE_KEYS;
 return bReturn;
}
bool CDataFile::CreateSection(t_Str szSection, t_Str szComment)
{
 t_Section* pSection = GetSection(szSection);
 if (pSection)
 {
  Report(E_INFO, "[CDataFile::CreateSection] Section <%s> allready exists. Aborting.", szSection.c_str());
  return false;
 }
 pSection = new t_Section;
 pSection->szName = szSection;
 pSection->szComment = szComment;
 m_Sections.push_back(*pSection);
 m_bDirty = true;
 return true;
}
bool CDataFile::CreateSection(t_Str szSection, t_Str szComment, KeyList Keys)
{
 if (!CreateSection(szSection, szComment))
  return false;
 t_Section* pSection = GetSection(szSection);
 if (!pSection)
  return false;
 KeyItor k_pos;
 pSection->szName = szSection;
 for (k_pos = Keys.begin(); k_pos != Keys.end(); k_pos++)
 {
  t_Key* pKey = new t_Key;
  pKey->szComment = (*k_pos).szComment;
  pKey->szKey = (*k_pos).szKey;
  pKey->szValue = (*k_pos).szValue;
  pSection->Keys.push_back(*pKey);
 }
 m_Sections.push_back(*pSection);
 m_bDirty = true;
 return true;
}
int CDataFile::SectionCount()
{
 return static_cast<int>(m_Sections.size());
}
int CDataFile::KeyCount()
{
 int nCounter = 0;
 SectionItor s_pos;
 for (s_pos = m_Sections.begin(); s_pos != m_Sections.end(); s_pos++)
  nCounter += static_cast<int>((*s_pos).Keys.size());
 return nCounter;
}
t_Key* CDataFile::GetKey(t_Str szKey, t_Str szSection)
{
 KeyItor k_pos;
 t_Section* pSection;
 if ((pSection = GetSection(szSection)) == NULL)
  return NULL;
 for (k_pos = pSection->Keys.begin(); k_pos != pSection->Keys.end(); k_pos++)
 {
  if (CompareNoCase((*k_pos).szKey, szKey) == 0)
   return (t_Key*)&(*k_pos);
 }
 return NULL;
}
t_Section* CDataFile::GetSection(t_Str szSection)
{
 SectionItor s_pos;
 for (s_pos = m_Sections.begin(); s_pos != m_Sections.end(); s_pos++)
 {
  if (CompareNoCase((*s_pos).szName, szSection) == 0)
   return (t_Section*)&(*s_pos);
 }
 return NULL;
}
t_Str CDataFile::CommentStr(t_Str szComment)
{
 t_Str szNewStr = t_Str("");
 Trim(szComment);
 if (szComment.size() == 0)
  return szComment;
 if (szComment.find_first_of(CommentIndicators) != 0)
 {
  szNewStr = CommentIndicators[0];
  szNewStr += " ";
 }
 szNewStr += szComment;
 return szNewStr;
}
t_Str GetNextWord(t_Str& CommandLine)
{
 int nPos = static_cast<int>(CommandLine.find_first_of(EqualIndicators));
 t_Str sWord = t_Str("");
 if (nPos > -1)
 {
  sWord = CommandLine.substr(0, nPos);
  CommandLine.erase(0, nPos + 1);
 }
 else
 {
  sWord = CommandLine;
  CommandLine = t_Str("");
 }
 Trim(sWord);
 return sWord;
}
int CompareNoCase(t_Str str1, t_Str str2)
{
#ifdef WIN32
 return _stricmp(str1.c_str(), str2.c_str());
#else
 return strcasecmp(str1.c_str(), str2.c_str());
#endif
}
void Trim(t_Str& szStr)
{
 t_Str szTrimChars = WhiteSpace;
 szTrimChars += EqualIndicators;
 int nPos, rPos;
 nPos = static_cast<int>(szStr.find_first_not_of(szTrimChars));
 if (nPos > 0)
  szStr.erase(0, nPos);
 nPos = static_cast<int>(szStr.find_last_not_of(szTrimChars));
 rPos = static_cast<int>(szStr.find_last_of(szTrimChars));
 if (rPos > nPos && rPos > -1)
  szStr.erase(rPos, szStr.size() - rPos);
}
int WriteLn(std::fstream& stream, const char* fmt, ...)
{
 char buf[MAX_BUFFER_LEN];
 int nLength;
 t_Str szMsg;
 memset(buf, 0, MAX_BUFFER_LEN);
 va_list args;
 va_start(args, fmt);
 nLength = _vsnprintf_s(buf, MAX_BUFFER_LEN, fmt, args);
 va_end(args);
 if (buf[nLength] != '\n' && buf[nLength] != '\r')
  buf[nLength++] = '\n';
 stream.write(buf, nLength);
 return nLength;
}
void Report(e_DebugLevel DebugLevel, const char* fmt, ...)
{
 char buf[MAX_BUFFER_LEN];
 int nLength;
 t_Str szMsg;
 va_list args;
 memset(buf, 0, MAX_BUFFER_LEN);
 va_start(args, fmt);
 nLength = _vsnprintf_s(buf, MAX_BUFFER_LEN, fmt, args);
 va_end(args);
 if (buf[nLength] != '\n' && buf[nLength] != '\r')
  buf[nLength++] = '\n';
 switch (DebugLevel)
 {
 case E_DEBUG:
  szMsg = "<debug> ";
  break;
 case E_INFO:
  szMsg = "<info> ";
  break;
 case E_WARN:
  szMsg = "<warn> ";
  break;
 case E_ERROR:
  szMsg = "<error> ";
  break;
 case E_FATAL:
  szMsg = "<fatal> ";
  break;
 case E_CRITICAL:
  szMsg = "<critical> ";
  break;
 }
 szMsg += buf;
 printf(szMsg.c_str());
}
