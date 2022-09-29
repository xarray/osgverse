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
	//特殊点匹配属性
	enum MatchAttr
	{
		MA_Match_Ahead_Only,
		MA_Match_Back_Only,
		MA_Match_Pre_Back,
	};

	//计算特殊点n和g的数据
	void calcSpecialN_G(const std::string &sInput);

	//以下循环初步分割保存在m_setOriginalPoint(即不考虑特殊的n和g点)
	//初步分割原则：从0位置开始以最长的拼音为单位分割（即n和g的总是向前匹配的），如maning，分割为man'i'n'g，m_setOriginalPoint为2,3,4,5
	//如果该位置有用户分隔符，就不插入
	void preDivide(const std::string &input, std::vector<int> &points);

	//是否是元音字符
	bool isVowelChar(char ch);

	//获取特殊点匹配属性（只可前匹配如jionga的g，或者只可后匹配如mani的n，以及可前后匹配如ganga的g），如果是向后匹配的点，
	MatchAttr getSpecialPointMatchAttr(const std::string &sInput, int nN_PPosition);

	std::set<int>						m_setMatchBackOnly;
	std::vector<std::pair<int, int>>	m_vtMatchBoth;		//<当前点, 当前点的前一个点>
};

}}