#pragma once
#include <string>
#include <set>
#include <vector>

namespace ime{ namespace pinyin{

class StringFunction
{
public:
	//分离字符串，以sSymbol作为分割符号，bSkipEmptyString表示是否忽略空的字符串，比如a'b'c''d'e，返回{a,b,c,d,e}
	static void split(const std::string &sSource, const std::string &sSymbol, std::vector<std::string> &ret, bool bSkipEmptyString = true);

	//在字符串中的位置插入指定字符串，成为新的字符串
	static std::string insert(const std::string &source, const std::string &insert, const std::set<int> &points);

	//字符串替换
	static std::string replace(const std::string &sSource, const std::string &sOldStr, const std::string &sNewStr);

	//查找字符出现第n次的地方，如果未出现n次，返回std::string::npos
	static size_t findCharTimes(const std::string &source, char c, int times);

	//utf8与unicode互转
	static std::wstring utf8ToUnicode(const std::string &utf8);
	static std::string unicodeToUtf8(const std::wstring &unicode);
};

}}