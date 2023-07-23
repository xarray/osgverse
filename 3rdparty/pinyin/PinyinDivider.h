/*************************************************
*	拼音分割模块
*
*	1、主要依赖于PinyinBasic来作为分割的依据
*
*	2、输出为一个或多个分割结果（如果有特殊点的话）
*
*	3、分割步骤：①初步分割->②去掉可前后匹配的特殊n、g点，
*				同时替换只可后匹配的特殊n、g点->③可前后匹配
*				的特殊n、g点排列成多种组合可能->得到的组合与原来的
*				②结果拼起来。
*
*	4、由于拼音分割使用频繁，所以算法，特别是复制开销，尽量减少stl容器的使用
*	  复杂容器结构的销毁代价也很大
*
**************************************************/
#pragma once
#include <string>
#include <vector>
#include <set>

namespace ime{ namespace pinyin{

class PinyinDivider
{
public:
	std::string divide(const std::string &input);

private:
	enum MatchAttr
	{
		MA_Match_Ahead_Only,
		MA_Match_Back_Only,
		MA_Match_Pre_Back,
	};

	void calcSpecialN_G(const std::string &sInput);

	void preDivide(const std::string &input, std::vector<int> &points);

	bool isVowelChar(char ch);

	MatchAttr getSpecialPointMatchAttr(const std::string &sInput, int nN_PPosition);

	std::set<int>						m_setMatchBackOnly;
	std::vector<std::pair<int, int>>	m_vtMatchBoth;
};

}}
