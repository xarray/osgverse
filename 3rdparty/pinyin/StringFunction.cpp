#include <string.h>
#include <algorithm>
#include "StringFunction.h"

using namespace ime::pinyin;

unsigned int UTF8StrToUnicode(const char*UTF8String, unsigned int UTF8StringLength, wchar_t *OutUnicodeString, unsigned int UnicodeStringBufferSize)
{
	unsigned int UTF8Index = 0;
	unsigned int UniIndex = 0;

	while (UTF8Index < UTF8StringLength)
	{
		unsigned char UTF8Char = UTF8String[UTF8Index];
		if (UnicodeStringBufferSize != 0 && UniIndex >= UnicodeStringBufferSize)
			break;

		if ((UTF8Char & 0x80) == 0)
		{
			const unsigned int cUTF8CharRequire = 1;
			// UTF8字码不足
			if (UTF8Index + cUTF8CharRequire > UTF8StringLength)
				break;

			if (OutUnicodeString)
			{
				wchar_t &WideChar = OutUnicodeString[UniIndex];
				WideChar = UTF8Char;
			}
			UTF8Index++;
		}
		else if ((UTF8Char & 0xE0) == 0xC0)  ///< 110x-xxxx 10xx-xxxx
		{
			const unsigned int cUTF8CharRequire = 2;
			// UTF8字码不足
			if (UTF8Index + cUTF8CharRequire > UTF8StringLength)
				break;

			if (OutUnicodeString)
			{
				wchar_t &WideChar = OutUnicodeString[UniIndex];
				WideChar = (UTF8String[UTF8Index + 0] & 0x3F) << 6;
				WideChar |= (UTF8String[UTF8Index + 1] & 0x3F);
			}
			UTF8Index += cUTF8CharRequire;
		}
		else if ((UTF8Char & 0xF0) == 0xE0)  ///< 1110-xxxx 10xx-xxxx 10xx-xxxx
		{
			const unsigned int cUTF8CharRequire = 3;
			// UTF8字码不足
			if (UTF8Index + cUTF8CharRequire > UTF8StringLength)
				break;

			if (OutUnicodeString)
			{
				wchar_t& WideChar = OutUnicodeString[UniIndex];

				WideChar = (UTF8String[UTF8Index + 0] & 0x1F) << 12;
				WideChar |= (UTF8String[UTF8Index + 1] & 0x3F) << 6;
				WideChar |= (UTF8String[UTF8Index + 2] & 0x3F);
			}
			UTF8Index += cUTF8CharRequire;
		}
		else if ((UTF8Char & 0xF8) == 0xF0)  ///< 1111-0xxx 10xx-xxxx 10xx-xxxx 10xx-xxxx 
		{
			const unsigned int cUTF8CharRequire = 4;
			// UTF8字码不足
			if (UTF8Index + cUTF8CharRequire > UTF8StringLength)
				break;

			if (OutUnicodeString)
			{
				wchar_t& WideChar = OutUnicodeString[UniIndex];

				WideChar = (UTF8String[UTF8Index + 0] & 0x0F) << 18;
				WideChar = (UTF8String[UTF8Index + 1] & 0x3F) << 12;
				WideChar |= (UTF8String[UTF8Index + 2] & 0x3F) << 6;
				WideChar |= (UTF8String[UTF8Index + 3] & 0x3F);
			}
			UTF8Index += cUTF8CharRequire;
		}
		else ///< 1111-10xx 10xx-xxxx 10xx-xxxx 10xx-xxxx 10xx-xxxx 
		{
			const unsigned int cUTF8CharRequire = 5;
			// UTF8字码不足
			if (UTF8Index + cUTF8CharRequire > UTF8StringLength)
				break;

			if (OutUnicodeString)
			{
				wchar_t& WideChar = OutUnicodeString[UniIndex];

				WideChar = (UTF8String[UTF8Index + 0] & 0x07) << 24;
				WideChar = (UTF8String[UTF8Index + 1] & 0x3F) << 18;
				WideChar = (UTF8String[UTF8Index + 2] & 0x3F) << 12;
				WideChar |= (UTF8String[UTF8Index + 3] & 0x3F) << 6;
				WideChar |= (UTF8String[UTF8Index + 4] & 0x3F);
			}
			UTF8Index += cUTF8CharRequire;
		}
		UniIndex++;
	}

	return UniIndex;
}

unsigned int UniCharToUTF8(wchar_t UniChar, char *OutUTFString)
{
	unsigned int UTF8CharLength = 0;
	if (UniChar < 0x80)
	{
		if (OutUTFString)
			OutUTFString[UTF8CharLength++] = (char)UniChar;
		else
			UTF8CharLength++;
	}
	else if (UniChar < 0x800)
	{
		if (OutUTFString)
		{
			OutUTFString[UTF8CharLength++] = 0xc0 | (UniChar >> 6);
			OutUTFString[UTF8CharLength++] = 0x80 | (UniChar & 0x3f);
		}
		else
		{
			UTF8CharLength += 2;
		}
	}
	else if (UniChar < 0x10000)
	{
		if (OutUTFString)
		{
			OutUTFString[UTF8CharLength++] = 0xe0 | (UniChar >> 12);
			OutUTFString[UTF8CharLength++] = 0x80 | ((UniChar >> 6) & 0x3f);
			OutUTFString[UTF8CharLength++] = 0x80 | (UniChar & 0x3f);
		}
		else
		{
			UTF8CharLength += 3;
		}
	}
	else if (UniChar < 0x200000)
	{
		if (OutUTFString)
		{
			OutUTFString[UTF8CharLength++] = 0xf0 | ((int)UniChar >> 18);
			OutUTFString[UTF8CharLength++] = 0x80 | ((UniChar >> 12) & 0x3f);
			OutUTFString[UTF8CharLength++] = 0x80 | ((UniChar >> 6) & 0x3f);
			OutUTFString[UTF8CharLength++] = 0x80 | (UniChar & 0x3f);
		}
		else
		{
			UTF8CharLength += 4;
		}
	}

	return UTF8CharLength;
}

void StringFunction::split(const std::string &sSource, const std::string &sSymbol, std::vector<std::string> &ret, bool bSkipEmptyString)
{
	if(sSource.empty() || sSymbol.empty())
		return;

	std::string sCurString = sSource;
	int nSymbolSize = sSymbol.size();

	int nPos = sCurString.find_first_of(sSymbol);
	do
	{
		if(nPos == std::string::npos)
		{
			ret.push_back(sCurString);
			break;
		}
		else
		{
			std::string sInsert = sCurString.substr(0, nPos);
			if(!(sInsert.empty() && bSkipEmptyString))
			{
				ret.push_back(sInsert);
			}
			
			if(nPos + nSymbolSize == sCurString.size() && !bSkipEmptyString)
			{
				ret.push_back("");
				break;
			}
			else
			{
				sCurString = sCurString.substr(nPos + nSymbolSize);
			}
		}
		nPos = sCurString.find_first_of(sSymbol);
	}
	while(!sCurString.empty());
}

std::string StringFunction::insert(const std::string &source, const std::string &insert, const std::set<int> &points)
{
	std::string sRet;
	int beg = 0;
	for(auto pos : points)
	{
		sRet += source.substr(beg, pos + 1 - beg) + insert;
		beg = pos + 1;
	}
	sRet += source.substr(beg);

	return sRet;
}

std::string StringFunction::replace(const std::string &sSource, const std::string &sOldStr, const std::string &sNewStr)
{
	std::string sRet = sSource;
	for(std::string::size_type pos = 0; pos != std::string::npos; pos += sNewStr.size())
	{
		if((pos = sRet.find(sOldStr, pos)) != std::string::npos)
			sRet.replace(pos, sOldStr.size(), sNewStr);
		else
			break;
	}
	return sRet;
}

size_t StringFunction::findCharTimes(const std::string &source, char c, int times)
{
	int findTimes = 0;
	size_t n = -1;
	while((n = source.find(c, n + 1)) != std::string::npos)
	{
		if (++findTimes == times)
			return n;
	}
	return std::string::npos;
}

std::wstring StringFunction::utf8ToUnicode(const std::string &utf8)
{
	size_t n = strlen(utf8.data());
	unsigned int nUnicodeLen = UTF8StrToUnicode(utf8.data(), n, nullptr, 0);
	wchar_t *pUnicode = new wchar_t[nUnicodeLen + 1];
	UTF8StrToUnicode(utf8.data(), n, pUnicode, nUnicodeLen);
	pUnicode[nUnicodeLen] = 0;
	std::wstring wsRet = pUnicode;
	delete []pUnicode;
	return wsRet;
}

std::string StringFunction::unicodeToUtf8(const std::wstring &unicode)
{
	std::string sRet;
	for (auto const &wCh : unicode)
	{
		int nUtf8Len = UniCharToUTF8(wCh, nullptr);
		char *pUtf8 = new char[nUtf8Len + 1];
		memset(pUtf8, 0, nUtf8Len + 1);
		UniCharToUTF8(wCh, pUtf8);
		sRet += pUtf8;
		delete[]pUtf8;
	}
	return sRet;
}
