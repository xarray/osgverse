#pragma once
#include <string>
#include <vector>

namespace ime { namespace pinyin{

class PinyinHanzi
{
public:
	static void getHanzis(const std::string &pinyin, std::wstring &hanzis);

	static bool isIndependentHanzi(const std::wstring &hanzi);

	static void getPinyinsFromHanzi(const std::wstring &hanzi, std::vector<std::string> &pinyins);
};

}}
