#pragma once
#include <string>
#include <vector>

namespace ime { namespace pinyin{

class PinyinHanzi
{
public:
	//有pinyin查询对应的所有的汉字
	static void getHanzis(const std::string &pinyin, std::wstring &hanzis);

	//是否是特殊独立的汉字
	static bool isIndependentHanzi(const std::wstring &hanzi);

	//获取汉字对应的拼音，可能存在多音字的情况
	static void getPinyinsFromHanzi(const std::wstring &hanzi, std::vector<std::string> &pinyins);
};

}}