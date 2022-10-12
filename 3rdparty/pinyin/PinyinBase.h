#pragma once
#include <string>
#include <set>

namespace ime{ namespace pinyin{

class PinyinBase
{
public:
	//是否是声母，如"y", "zh"
	static bool isShengmu(const std::string &sm);

	//是否是韵母
	static bool isYunmu(const std::string &ym);

	//是否是无声母拼音,如"ao"、"ang"
	static bool isYunmuIndependent(const std::string &ym);

	//是否是合法的拼音（包括不完整的）,形如"zh"、"zhang"、"ai"，只有声母也返回true
	static bool isValid(const std::string &pinyin);

	//是否是完整的拼音，形如"ji", "jiong", "zhua"，不包括不完整的拼音
	static bool isCompleted(const std::string &pinyin);

	//是否是单向的拼音，标准为GetPossiblePinyins是1个
	static bool isOneway(const std::string &pinyin);

	//提取声母韵母，"zh"，"jiong"，"an"返回true，其他形式如"gg", "win", "xion"返回false
	static std::tuple<bool, std::string, std::string> extractShengmuYunmu(const std::string &pinyin);
	
	//基于当前未完成的拼音可能成为的拼音，包括自身，如jion返回jiong，jio返回jiong，jiong返回jiong
	//interrupt：true表示获取第一个后就返回
	static void getPossibles(const std::string &prefix, std::set<std::string> &ret, bool interrupt = false);

	//获取基于当前拼音串的最优先拼音，如b返回ba，s返回shi，sh返回shi，shi返回shi
	static std::string getDefalut(const std::string &prefix);

};

}}