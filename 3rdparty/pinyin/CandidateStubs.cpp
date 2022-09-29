#include "CandidateStubs.h"
#include "Query.h"
#include "StringFunction.h"
#include "PinyinBase.h"
#include "PinyinHanzi.h"
#include <algorithm>

using namespace ime::pinyin;

void CombineAssociateCandidate::setInput(const std::string &divided, bool enableCombineCandidate, bool enableAssociateCandidate)
{
	clear();
	m_divided = divided;
	//如果只有单个拼音，则没有组合词组
	if (m_divided.find("'") == std::string::npos)
		return;

	//以下获取不可调换顺序
	//按顺序搜索获取组合词（成功表示最长长度的pinyin不能搜索到词组；反之搜到了词组，不再获取组合词和联想词）
	std::string sFirstPinyins;
	std::wstring sFirstCizu;
	std::string sSecondPinyin;
	bool bGetFirst = false;
	if (enableCombineCandidate)
	{
		bGetFirst = getFirstCombine(m_divided, sFirstPinyins, sFirstCizu, sSecondPinyin);
		if (!bGetFirst)
			return;
	}

	//如果最长长度的pinyin不能搜索到词组，获取模糊词组
	if (enableAssociateCandidate)
		getAssociate(m_divided);

	//如果最长长度的pinyin不能搜索到词组，才执行下面
	//且第一个拼音是特殊独立的汉字拼音比如“我”，“你”，“他”，“这”等
	//sFirstCizu.size()是1的话第二组合词肯定等于第一个组合词了，没必要再获取
	bool bGetSecond = false;
	if (enableCombineCandidate && bGetFirst && sFirstCizu.size() != 1)
		bGetSecond = getSecondCombine(m_divided);

	//sCombineCizu2未插入且第二个词为特殊独立的汉字拼音就添加一个组合词
	if (enableCombineCandidate && !bGetSecond)
		bool bGetThird = getThirdCombine(m_divided, sFirstPinyins, sFirstCizu, sSecondPinyin);
}

unsigned int CombineAssociateCandidate::getCandidateCount() const
{
	return m_candidates.size();
}

Candidate CombineAssociateCandidate::getCandidate(unsigned int index) const
{
	return{ m_candidates[index].associate ? CandidateType::Associate : CandidateType::Combine, m_candidates[index].size, (char *)m_candidates[index].pinyin.data(), (char *)m_candidates[index].cizu.data() };
}

void CombineAssociateCandidate::getCandidate(unsigned int index, unsigned int count, std::vector<Candidate>& candidates) const
{
	if (index >= m_candidates.size())
		return;
	auto end = m_candidates.begin() + index + count >= m_candidates.end() ? m_candidates.end() : m_candidates.begin() + index + count;
	std::for_each(m_candidates.begin(), end, [&candidates](const CombineCandidate &can) {
		candidates.push_back(Candidate{ can.associate ? CandidateType::Associate : CandidateType::Combine, can.size, (char *)can.pinyin.data(), (char *)can.cizu.data() }); }
	);
}

void CombineAssociateCandidate::clear()
{
	m_divided.clear();
	m_candidates.clear();
}

bool CombineAssociateCandidate::getFirstCombine(const std::string &input, std::string &firstPinyins, std::wstring &firstCizu, std::string &secondPinyin)
{
	std::string sSearch = input;
	CombineCandidate can{ false };
	int nKick = 0;
	while (!sSearch.empty())
	{
		if (nKick == 1)
			secondPinyin = sSearch.substr(0, sSearch.find("'"));

		CombineLine cl;
		Candidate firstCan;
		cl.search(sSearch);
		bool bHasQueryRecord = cl.getFirstCandidate(firstCan);
		if (bHasQueryRecord)
		{
			if (nKick == 0)
			{
				firstPinyins = firstCan.pinyin;
				firstCizu = StringFunction::utf8ToUnicode(firstCan.cizu);
			}

			//如果最长长度的pinyin能搜索到词组，取消组合词
			if (sSearch == m_divided && firstCan.size == std::count(input.begin(), input.end(), '\'') + 1)
			{
				return false;
			}
			else
			{
				can.size += firstCan.size;
				can.pinyin += ("'" + std::string(firstCan.pinyin));
				can.cizu += firstCan.cizu;
				size_t n = StringFunction::findCharTimes(sSearch, '\'', firstCan.size);
				sSearch = n == std::string::npos ? "" : sSearch.substr(n + 1);
			}
		}
		else
		{
			std::string sFirstPinyin = sSearch.substr(0, sSearch.find_first_of("'"));
			std::string sCompletePinyin = PinyinBase::getDefalut(sFirstPinyin);
			std::wstring wsAllSingleWords;
			PinyinHanzi::getHanzis(sCompletePinyin, wsAllSingleWords);
			can.size += 1;
			can.pinyin += ("'" + sCompletePinyin);
			can.cizu += StringFunction::unicodeToUtf8(wsAllSingleWords.substr(0, 1));
			size_t n = StringFunction::findCharTimes(sSearch, '\'', 1);
			sSearch = n == std::string::npos ? "" : sSearch.substr(n + 1);
			if (nKick == 0)
			{
				firstPinyins = can.pinyin;
				firstCizu = wsAllSingleWords.substr(0, 1);
			}
		}
		++nKick;
	}
	m_candidates.push_back(can);
	return true;
}

void CombineAssociateCandidate::getAssociate(const std::string &input)
{
	//模糊搜索，与size无关
	Query longestQuery;
	longestQuery.search(Query::size | Query::pinyin | Query::cizu | Query::weight, -1, Query::Condition::none, input, Query::Condition::like, true, false, "", Query::Condition::none, Query::weight);
	for (int i = 0; i != longestQuery.recordCount(); ++i)
	{
		char * size = nullptr, *pinyin = nullptr, *cizu = nullptr;
		longestQuery.getRecord(i, size, pinyin, cizu);
		m_candidates.push_back({ true, std::stoi(size), pinyin, cizu });
	}
}

bool CombineAssociateCandidate::getSecondCombine(const std::string &input)
{
	std::string sSearch = input;
	CombineCandidate can{ false };

	std::string sFirstPinyin = sSearch.substr(0, sSearch.find_first_of("'"));
	std::string sCompletePinyin = PinyinBase::getDefalut(sFirstPinyin);
	std::wstring wsAllSingleWords;
	PinyinHanzi::getHanzis(sCompletePinyin, wsAllSingleWords);
	std::wstring wsFirstHanzi = wsAllSingleWords.substr(0, 1);
	if (PinyinHanzi::isIndependentHanzi(wsFirstHanzi))
	{
		can.size += 1;
		can.pinyin += ("'" + sCompletePinyin);
		can.cizu += StringFunction::unicodeToUtf8(wsFirstHanzi);
		size_t n = StringFunction::findCharTimes(sSearch, '\'', 1);
		sSearch = n == std::string::npos ? "" : sSearch.substr(n + 1);
		while (!sSearch.empty())
		{
			CombineLine cl;
			Candidate firstCan;
			cl.search(sSearch);
			bool bHasQueryRecord = cl.getFirstCandidate(firstCan);
			if (bHasQueryRecord)
			{
				can.size += firstCan.size;
				can.pinyin += firstCan.pinyin;
				can.cizu += firstCan.cizu;
				size_t n = StringFunction::findCharTimes(sSearch, '\'', firstCan.size);
				sSearch = n == std::string::npos ? "" : sSearch.substr(n + 1);
			}
			else
			{
				std::string sFirstPinyin = sSearch.substr(0, sSearch.find_first_of("'"));
				std::string sCompletePinyin = PinyinBase::getDefalut(sFirstPinyin);
				std::wstring wsAllSingleWords;
				PinyinHanzi::getHanzis(sCompletePinyin, wsAllSingleWords);
				can.size += 1;
				can.pinyin += ("'" + sCompletePinyin);
				can.cizu += StringFunction::unicodeToUtf8(wsAllSingleWords.substr(0, 1));
				size_t n = StringFunction::findCharTimes(sSearch, '\'', 1);
				sSearch = n == std::string::npos ? "" : sSearch.substr(n + 1);
			}
		}
	}
	else
	{
		return false;
	}

	for(auto const &one : m_candidates)
		if (one.cizu == can.cizu)
			return false;
	m_candidates.push_back(can);
	return true;
}

bool CombineAssociateCandidate::getThirdCombine(const std::string &input, const std::string &firstPinyins, std::wstring &firstCizu, std::string &secondPinyin)
{
	std::string sCompletePinyin = PinyinBase::getDefalut(secondPinyin);
	std::wstring wsSecondAllSingleWords;
	PinyinHanzi::getHanzis(sCompletePinyin, wsSecondAllSingleWords);
	std::wstring wsHanzi = wsSecondAllSingleWords.substr(0, 1);
	if (!PinyinHanzi::isIndependentHanzi(wsHanzi))
		return false;

	CombineCandidate can{ false };
	size_t n = StringFunction::findCharTimes(input, '\'', firstCizu.size() + 1);
	std::string sSearch = n == std::string::npos ? "" : input.substr(n + 1);
	can.size += (firstCizu.size() + 1);
	can.pinyin += (firstPinyins + "'" + sCompletePinyin + "'");
	can.cizu += (StringFunction::unicodeToUtf8(firstCizu) + StringFunction::unicodeToUtf8(wsHanzi));
	while (!sSearch.empty())
	{
		CombineLine cl;
		Candidate firstCan;
		cl.search(sSearch);
		bool bHasQueryRecord = cl.getFirstCandidate(firstCan);
		if (bHasQueryRecord)
		{
			can.size += firstCan.size;
			can.pinyin += firstCan.pinyin;
			can.cizu += firstCan.cizu;
			size_t n = StringFunction::findCharTimes(sSearch, '\'', firstCan.size);
			sSearch = n == std::string::npos ? "" : sSearch.substr(n + 1);
		}
		else
		{
			std::string sFirstPinyin = sSearch.substr(0, sSearch.find_first_of("'"));
			std::string sCompletePinyin = PinyinBase::getDefalut(sFirstPinyin);
			std::wstring wsAllSingleWords;
			PinyinHanzi::getHanzis(sCompletePinyin, wsAllSingleWords);
			can.size += 1;
			can.pinyin += ("'" + sCompletePinyin);
			can.cizu += StringFunction::unicodeToUtf8(wsAllSingleWords.substr(0, 1));
			size_t n = StringFunction::findCharTimes(sSearch, '\'', 1);
			sSearch = n == std::string::npos ? "" : sSearch.substr(n + 1);
		}
	}

	for (auto const &one : m_candidates)
		if (one.cizu == can.cizu)
			return false;
	m_candidates.push_back(can);
	return true;
}

void QueryCandidate::setInput(const std::string & input)
{
	m_queryLine.search(input);
}

unsigned int QueryCandidate::getCandidateCount() const
{
	unsigned int ret = 0;
	for (int i = 0; i != m_queryLine.queryCount(); ++i)
		ret += m_queryLine.at(i)->recordCount();
	return ret;
}

void QueryCandidate::getCandidate(unsigned int index, unsigned int count, std::vector<Candidate>& candidates) const
{
	if (count == 0)
		return;

	unsigned int nGetCount = 0;
	//计算起始query，并获取该query数据
	int nStartQueryIndex = 0;
	for (int n = 0; nStartQueryIndex != m_queryLine.queryCount(); ++nStartQueryIndex)
	{
		std::shared_ptr<Query> query = m_queryLine.at(nStartQueryIndex);
		n += query->recordCount();
		if ((int)index <= n - 1)	//强制转换为int，因为n - 1可能被隐性转为unsgned int，导致比较结果错误
		{
			int beg = index - (n - query->recordCount());
			int size = m_queryLine.queryCount() + 1 - nStartQueryIndex;
			for (int i = beg; i != query->recordCount(); ++i)
			{
				char *pinyin = nullptr, *cizu = nullptr;
				query->getRecord(i, pinyin, cizu);
				candidates.push_back(Candidate{ CandidateType::Query, size, pinyin, cizu });
				if (++nGetCount >= count)
					return;
			}
			break;
		}
	}

	//从nStartQueryIndex的下一个query继续获取数据
	for (int i = nStartQueryIndex + 1; i != m_queryLine.queryCount(); ++i)
	{
		int size = m_queryLine.queryCount() + 1 - i;
		std::shared_ptr<Query> query = m_queryLine.at(i);
		for(int j = 0; j != query->recordCount(); ++j)
		{
			char *pinyin = nullptr, *cizu = nullptr;
			query->getRecord(j, pinyin, cizu);
			candidates.push_back(Candidate{ CandidateType::Query, size, pinyin, cizu });
			if (++nGetCount >= count)
				return;
		}
	}
}

void QueryCandidate::clear()
{
	m_queryLine.clear();
}

void SinglePinyinCandidate::setPinyin(const std::string & complete)
{
	m_comoletedPinyin = complete;
	m_hanzis.clear();
	SingleCache::getHanzis(complete, m_hanzis);
}

unsigned int SinglePinyinCandidate::getCandidateCount() const
{
	return m_hanzis.size();
}

Candidate SinglePinyinCandidate::getCandidate(unsigned int index) const
{
	return Candidate{ CandidateType::Hanzi, 1, (char *)m_comoletedPinyin.data(), (char *)m_hanzis[index].data() };
}

void SinglePinyinCandidate::getCandidate(unsigned int index, unsigned int count, std::vector<Candidate>& candidates) const
{
	int nEnd = index + count >= m_hanzis.size() ? m_hanzis.size() : index + count;
	for (int i = 0; i != nEnd; ++i)
		candidates.push_back(Candidate{ CandidateType::Hanzi, 1, (char *)m_comoletedPinyin.data(), (char *)m_hanzis[i].data() });
}

void SinglePinyinCandidate::clear()
{
	m_comoletedPinyin.clear();
	m_hanzis.clear();
}
