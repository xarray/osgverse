#include "PinyinBase.h"
#include <map>

using namespace ime::pinyin;
#define PINYIN_MAX_SIZE	6
static std::map<std::string, std::set<std::string>> g_pyBase
{
	{ "YM",{ "iang", "iong", "uang", "ian", "iao", "uan", "uai", "ang", "eng", "ing", "ong", "ua", "uo", "ia", "ai", "ei", "ui", "ao", "ou", "iu", "ie", "ue", "ve", "er", "an", "en", "in", "un", "a", "o", "e", "i", "u", "v" } },
	{ "NOSM",{ "a", "e", "o", "ai", "en", "ei", "er", "ao", "an", "ou", "ang", "eng" } },
	{ "ch",{ "i", "uang", "ang", "eng", "ong", "uai", "uan", "ai", "an", "ao", "ou", "ua", "ui", "un", "uo", "en", "a", "e", "u" } },
	{ "sh",{ "i", "uang", "ang", "eng", "uai", "uan", "ai", "an", "ao", "ei", "en", "ou", "ua", "ui", "un", "uo", "a", "e", "u" } },
	{ "zh",{ "ong", "uang", "ang", "eng", "uai", "uan", "ai", "an", "ao", "ei", "en", "ou", "ua", "ui", "un", "uo", "a", "e", "i", "u" } },
	{ "b",{ "a", "u", "i", "o", "ie", "ao", "ei", "en", "ai", "an", "in", "ian", "iao", "ang", "eng", "ing" } },
	{ "c",{ "ai", "ang", "eng", "ong", "uan", "an", "ao", "en", "ou", "ui", "un", "uo", "a", "e", "i", "u" } },
	{ "d",{ "e", "a", "i", "u", "ao", "ai", "an", "en", "ei", "ou", "un", "uo", "ui", "ia", "ie", "iu", "ian", "iao", "uan", "ang", "eng", "ong", "ing" } },
	{ "f",{ "a", "u", "o", "an", "ou", "ei", "en", "ang", "eng", "iao" } },
	{ "g",{ "e", "u", "uang", "ang", "eng", "ong", "uai", "uan", "ai", "an", "ao", "ei", "en", "ou", "ua", "ui", "un", "uo", "a" } },
	{ "h",{ "ao", "a", "uang", "ang", "eng", "ong", "uai", "uan", "ai", "an", "ei", "en", "ou", "ua", "ui", "un", "uo", "e", "u" } },
	{ "j",{ "iang", "iong", "ing", "uan", "ian", "iao", "ie", "in", "ia", "iu", "ue", "un", "i", "u" } },
	{ "k",{ "an", "ao", "uang", "ang", "eng", "ong", "uai", "uan", "ai", "en", "ei", "ou", "ua", "ui", "un", "uo", "a", "e", "u" } },
	{ "l",{ "a", "e", "i", "o", "u", "v", "iang", "ang", "eng", "ong", "ing", "uan", "ian", "iao", "ai", "an", "ao", "ei", "ia", "ie", "in", "iu", "ou", "ve", "un", "uo" } },
	{ "m",{ "a", "e", "i", "o", "u", "ang", "eng", "ing", "ian", "iao", "ai", "an", "ao", "ei", "en", "ie", "in", "iu", "ou" } },
	{ "n",{ "e", "i", "a", "u", "v", "iang", "ang", "eng", "ing", "ong", "ian", "iao", "uan", "ai", "an", "ao", "ei", "en", "ie", "in", "iu", "ou", "ve", "uo", "un" } },
	{ "p",{ "a", "i", "o", "u", "ang", "eng", "ing", "ian", "iao", "ai", "an", "ao", "ei", "en", "ie", "in", "ou" } },
	{ "q",{ "u", "i", "iang", "iong", "ing", "ian", "iao", "uan", "ia", "ie", "in", "iu", "ue", "un" } },
	{ "r",{ "en", "ang", "eng", "ong", "uan", "an", "ao", "ou", "ui", "un", "ua", "uo", "e", "i", "u" } },
	{ "s",{ "ang", "eng", "ong", "uan", "ai", "an", "ao", "en", "ou", "ui", "un", "uo", "a", "e", "i", "u" } },
	{ "t",{ "a", "e", "i", "u", "ang", "eng", "ing", "ong", "ian", "iao", "uan", "ai", "an", "ao", "ie", "ei", "ou", "ui", "un", "uo" } },
	{ "w",{ "o", "a", "u", "ai", "an", "ei", "en", "ang", "eng" } },
	{ "x",{ "iang", "iong", "ing", "ian", "iao", "uan", "ia", "ie", "in", "iu", "ue", "un", "i", "u" } },
	{ "y",{ "a", "i", "e", "o", "u", "ang", "ing", "ong", "uan", "an", "ao", "in", "ou", "ue", "un" } },
	{ "z",{ "ai", "ang", "eng", "ong", "uan", "an", "ao", "ei", "en", "ou", "ui", "un", "uo", "a", "e", "i", "u" } },
};

static std::map<std::string, std::string> g_pyDefault
{
	{ "b", "ba" },{ "c", "cai" },{ "d", "de" },{ "f", "fa" },{ "g", "ge" },{ "h", "hao" },{ "j", "jiang" },{ "k", "kan" },{ "l", "le" },{ "m", "ma" },{ "n", "ne" },{ "p", "pa" },
	{ "q", "qu" },{ "r", "ren" },{ "s", "shi" },{ "t", "ta" },{ "w", "wo" },{ "x", "xiang" },{ "y", "ya" },{ "z", "zai" },{ "ch", "chi" },{ "sh", "shi" },{ "zh", "zhong" },
};

bool PinyinBase::isShengmu(const std::string &sm)
{
	return sm != "YM" && sm != "NOSM" && g_pyBase.find(sm) != g_pyBase.end();
}

bool PinyinBase::isYunmu(const std::string &ym)
{
	auto iter = g_pyBase.find("YM");
	return ((iter != g_pyBase.end()) && (iter->second.find(ym) != iter->second.end()));
}

bool PinyinBase::isYunmuIndependent(const std::string &ym)
{
	auto iter = g_pyBase.find("NOSM");
	return ((iter != g_pyBase.end()) && (iter->second.find(ym) != iter->second.end()));
}

bool PinyinBase::isValid(const std::string &pinyin)
{
	return std::get<0>(extractShengmuYunmu(pinyin));
}

bool PinyinBase::isCompleted(const std::string &pinyin)
{
	std::tuple<bool, std::string, std::string> ret = extractShengmuYunmu(pinyin);
	return std::get<0>(ret) && !std::get<2>(ret).empty();
}

bool PinyinBase::isOneway(const std::string &pinyin)
{
	std::set<std::string> ret;
	getPossibles(pinyin, ret);
	return (ret.size() == 1);
}

std::tuple<bool, std::string, std::string> PinyinBase::extractShengmuYunmu(const std::string &pinyin)
{
	bool success = false;
	std::string sm;
	std::string ym;
	if (pinyin.empty() || pinyin.size() > PINYIN_MAX_SIZE)
		return std::make_tuple(success, sm, ym);

	if (pinyin.size() >= 2 && isShengmu(pinyin.substr(0, 2)))
		sm = pinyin.substr(0, 2);
	else if (isShengmu(pinyin.substr(0, 1)))
		sm = pinyin.substr(0, 1);

	if (sm.empty())
	{
		ym = pinyin;
		success = isYunmuIndependent(pinyin);
	}
	else
	{
		ym = pinyin.substr(sm.size(), pinyin.size() - sm.size());
		success = ym.empty() || g_pyBase[sm].find(ym) != g_pyBase[sm].end();
	}
	return std::make_tuple(success, sm, ym);
}

void PinyinBase::getPossibles(const std::string &prefix, std::set<std::string> &ret, bool interrupt)
{
	std::tuple<bool, std::string, std::string> ext = extractShengmuYunmu(prefix);
	bool success = std::get<0>(ext);
	if (!success)
		return;

	std::string sm = std::get<1>(ext);
	std::string ym = std::get<2>(ext);
	//查找拼音表
	for (auto const &str : g_pyBase[ sm.empty() ? "NOSM" : sm])
	{
		if (str.size() >= ym.size() && str.substr(0, ym.size()) == ym)
		{
			ret.insert(sm + str);
			if (interrupt)	return;
		}
	}

	//如果是无韵母且声母是c,s,z，把ch,sh，zh的结果也获取
	if((ym.empty()) && (sm == "c" || sm == "s" || sm == "z"))
	{
		for (auto const &str : g_pyBase[sm + 'h'])
		{
			if (str.size() >= ym.size() && str.substr(0, ym.size()) == ym)
			{
				ret.insert(sm + 'h' + str);
				if (interrupt)	return;
			}
		}
	}
}

std::string PinyinBase::getDefalut(const std::string &prefix)
{
	std::string ret;
	if(isCompleted(prefix) || (prefix == "i" || prefix == "u" || prefix == "v"))
	{
		return prefix;
	}
	else if(isShengmu(prefix))
	{
		return g_pyDefault[prefix];
	}
	else
	{
		std::set<std::string> possibles;
		getPossibles(prefix, possibles, true);
		return possibles.empty() ? "" : *possibles.begin();
	}
}
