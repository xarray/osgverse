/**********************************************
*	PRTCandidateCombineAssociate
*
*	组合词候选词和联想候选词管理器
*	1、如果最长的query有结果，此模块候选词为空
*
*	2、如果最长query没有结果，插入组合词1(顺序)
*
*	3、如果最长query没有结果，插入最长like得到的结果
*
*	4、如果最长query没有结果，且第一个词为特殊汉字，插入组合词2
*
*	5、如果不满足4、且第二个词为特殊汉字，插入组合词3
*
*	6、注意，组合词的顺序不能随意调换
*
************************************************/
#pragma once
#include <string>
#include <vector>
#include "Cache.h"
#include "Share.h"

namespace ime{ namespace pinyin{

class CombineAssociateCandidate
{
public:
	CombineAssociateCandidate() = default;
	~CombineAssociateCandidate() = default;
	CombineAssociateCandidate(const CombineAssociateCandidate &other) = delete;
	void operator = (const CombineAssociateCandidate &other) = delete;

	void setInput(const std::string &divided, bool enableCombineCandidate, bool enableAssociateCandidate);
	unsigned int getCandidateCount() const;
	Candidate getCandidate(unsigned int index) const;
	void getCandidate(unsigned int index, unsigned int count, std::vector<Candidate> &candidates) const;
	void clear();

private:
	struct CombineCandidate
	{
		bool		associate;
		int			size;
		std::string pinyin;
		std::string	cizu;
	};

	bool getFirstCombine(const std::string &input, std::string &firstPinyins, std::wstring &firstCizu, std::string &secondPinyin);
	//获取最长模糊查询结果，比如"zai'suo'you'ren'shi'yi'fei'de'jing'se'li'wo'zui'xi'huan'ni",
	//输入"zai'suo'you'ren'shi'"也会搜索"zai'suo'you'ren'shi%"
	void getAssociate(const std::string &input);
	bool getSecondCombine(const std::string &input);
	bool getThirdCombine(const std::string &input, const std::string &firstPinyins, std::wstring &firstCizu, std::string &secondPinyin);

	std::string						m_divided;
	std::vector<CombineCandidate>	m_candidates;
};

class QueryCandidate
{
public:
	void setInput(const std::string &input);
	unsigned int getCandidateCount() const;
	void getCandidate(unsigned int index, unsigned int count, std::vector<Candidate> &candidates) const;
	void clear();

private:
	QueryLine	m_queryLine;
};

class SinglePinyinCandidate
{
public:
	void setPinyin(const std::string &complete);
	unsigned int getCandidateCount() const;
	Candidate getCandidate(unsigned int index) const;
	void getCandidate(unsigned int index, unsigned int count, std::vector<Candidate> &candidates) const;
	void clear();

private:
	std::string						m_comoletedPinyin;
	std::vector<std::string>		m_hanzis;
};

}}