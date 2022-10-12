#include "Pinyin_Internal.h"
#include "pinyin/Pinyin.h"
#include "PinyinBase.h"
#include "PinyinHanzi.h"
#include "CikuManager.h"
#include "StringFunction.h"
#include <algorithm>
#include <cstdlib>

using namespace ime::pinyin;

Pinyin_Internal::Pinyin_Internal()
	: m_init(false)
	, m_AICombineCandidate(true)
	, m_associateCandidate(true)
	, m_candidatePageSize(5)
{
}

Pinyin_Internal::~Pinyin_Internal()
{
	deinit();
}

bool Pinyin_Internal::init(const std::string &systemCikuPath, const std::string &learnCikuPath)
{
	deinit();
	CikuManager::configure(systemCikuPath, learnCikuPath);
	m_init = CikuManager::init();
	return m_init;
}

bool Pinyin_Internal::hasInit() const
{
	return m_init;
}

bool Pinyin_Internal::deinit()
{
	m_init = false;
	clear();
	return CikuManager::deinit();
}

void Pinyin_Internal::enableAICombineCandidate(bool enable)
{
	m_AICombineCandidate = enable;
}

bool Pinyin_Internal::isEnableAICombineCandidate() const
{
	return m_AICombineCandidate;
}

void Pinyin_Internal::enableAssociateCandidate(bool enable)
{
	m_associateCandidate = enable;
}

bool Pinyin_Internal::isEnableAssociateCandidate() const
{
	return m_associateCandidate;
}

bool Pinyin_Internal::addUserCiku(const std::string &path)
{
	return CikuManager::addUserCiku(path);
}

bool Pinyin_Internal::remoteUserCiku(const std::string &path)
{
	return CikuManager::remoteUserCiku(path);
}

void Pinyin_Internal::clearUserCiku()
{
	CikuManager::clearUserCiku();
}

void Pinyin_Internal::getUserCikuPaths(std::vector<std::string> &paths) const
{
	return CikuManager::getAllUserCikuPaths(paths);
}

void Pinyin_Internal::setCandidatePageSize(unsigned int size)
{
	if(size == 0)
		throw std::out_of_range("setCandidatePageSize size by 0.");
	m_candidatePageSize = size;
}

unsigned int Pinyin_Internal::getCandidatePageSize() const
{
	return m_candidatePageSize;
}

bool Pinyin_Internal::search(const std::string &input)
{
	checkInit();
	if (!checkLegalPinyinString(input))
	{
		clear();
		return false;
	}
	else
	{
		m_dividedPinyin = m_divider.divide(input);
		//分别执行三种候选词搜索
		m_combineAndAssociateCandidates.setInput(m_dividedPinyin, isEnableAICombineCandidate(), isEnableAssociateCandidate());
		m_queryCandidates.setInput(m_dividedPinyin);
		std::string sFirstCompletePinyin = PinyinBase::getDefalut(m_dividedPinyin.substr(0, m_dividedPinyin.find("'")));
		m_singleCandidates.setPinyin(sFirstCompletePinyin);
		return true;
	}
}

std::string Pinyin_Internal::getSearchingDividedPinyin() const
{
	checkInit();
	return m_dividedPinyin;
}

unsigned int Pinyin_Internal::getCandidatePageCount() const
{
	checkInit();
	auto count = getCandidateCount();
	return  count %  getCandidatePageSize() == 0 ? count / getCandidatePageSize() : count / getCandidatePageSize() + 1;
}

void Pinyin_Internal::getCandidateByPage(unsigned int page, std::vector<std::string> &candidates) const
{
	checkInit();
	getCandidate(page * getCandidatePageSize(), getCandidatePageSize(), candidates);
}

unsigned int Pinyin_Internal::getCandidateCount() const
{
	checkInit();
	return m_combineAndAssociateCandidates.getCandidateCount() + m_queryCandidates.getCandidateCount() + m_singleCandidates.getCandidateCount();
}

void Pinyin_Internal::getCandidate(unsigned int index, unsigned int count, std::vector<std::string> &candidates) const
{
	checkInit();
	std::vector<Candidate> cans;
	getCandidate(index, count, cans);
	std::for_each(cans.begin(), cans.end(), [&candidates](Candidate &can) {candidates.push_back(can.cizu); });
}

void Pinyin_Internal::getCandidateInfo(unsigned int page, unsigned int index, CandidateInfo &info) const
{
	checkInit();
	return getCandidateInfo(getCandidatePageSize() * page + index, info);
}

void Pinyin_Internal::getCandidateInfo(unsigned int index, CandidateInfo &info) const
{
	checkInit();
	if (index >= getCandidateCount())
		throw std::out_of_range(std::string(__FUNCTION__) + "->index(" + std::to_string(index) + ") is out of range [0, " + std::to_string(getCandidateCount()) + ")");

	std::vector<Candidate> candidates;
	getCandidate(index, 1, candidates);
	const Candidate &can = candidates[0];
	info.canForget = can.type == CandidateType::Associate || can.type == CandidateType::Query;
	info.pinyin = can.pinyin;
	info.cizu = can.cizu;

	size_t n = StringFunction::findCharTimes(m_dividedPinyin, '\'', can.size);
	info.devidedPinyin = m_dividedPinyin.substr(0, n);
}

bool Pinyin_Internal::learn(const std::string &pinyin, const std::string &cizu, bool &alreadyExist)
{
	checkInit();
	//必须两个拼音以上且每个拼音都是isValid
	if (!checkLegalPinyinString(pinyin))
		return false;

	std::vector<std::string> pys;
	StringFunction::split(pinyin, "'", pys);

	Query q;
	q.search(Query::pinyin | Query::cizu, -1, Query::Condition::none, pinyin, Query::Condition::eq, false, false, cizu, Query::Condition::eq, Query::none);
	//如果词条不存在，以最高weight插入新纪录
	if (q.recordCount() == 0)
	{
		alreadyExist = false;
		int highestWeight = getFirstLettersHighestWeight(pys);
		return CikuManager::insertRecord(pys.size(), pinyin, cizu, 0, highestWeight + 1);
	}
	else
	{
		alreadyExist = true;
		return true;
	}
}

bool Pinyin_Internal::forget(const std::string &pinyin, const std::string &cizu)
{
	checkInit();
	clear();
	return CikuManager::deleteRecord(pinyin, cizu);
}

bool Pinyin_Internal::promote(const std::string & pinyin, const std::string & cizu)
{
	checkInit();
	clear();
	std::vector<std::string> pys;
	StringFunction::split(pinyin, "'", pys);
	int highestWeight = getFirstLettersHighestWeight(pys);
	return CikuManager::updateRecord(pinyin, cizu, -1, "", "", -1, highestWeight + 1);
}

void Pinyin_Internal::checkInit() const
{
	if (!m_init)
	{
		printf("any api must called after init success, exit.\r\n");
		std::exit(-1);
	}
}

bool Pinyin_Internal::checkLegalPinyinString(const std::string &input)
{
	if (input.empty() || input[0] == '\'')
		return false;

	for (int i = 0; i != input.size(); ++i)
	{
		const char &ch = input[i];
		if (!((ch >= 'a' && ch <= 'z') || (ch == '\'')))
			return false;
		if (ch == '\'')
		{
			if (i + 1 != input.size() && input[i + 1] == '\'')
				return false;
		}
	}
	return true;
}

void Pinyin_Internal::getCandidate(unsigned int index, unsigned int count, std::vector<Candidate> &candidates) const
{
	checkInit();
	if (count == 0)
		return;

	if (index >= getCandidateCount())
		throw std::out_of_range(std::string(__FUNCTION__) + "->index(" + std::to_string(index) + ") is out of range [0, " + std::to_string(getCandidateCount()) + ")");

	int nGet = 0;
	if (index < m_combineAndAssociateCandidates.getCandidateCount())	//开始点落在组合词内
	{
		for (int i = index; i != m_combineAndAssociateCandidates.getCandidateCount(); ++i)
		{
			candidates.push_back(m_combineAndAssociateCandidates.getCandidate(i));
			if (++nGet == count)
				return;
		}
		//如果组合联想词不够，填上query的数据
		int nRecordGet = count - nGet > m_queryCandidates.getCandidateCount() ? m_queryCandidates.getCandidateCount() : count - nGet;
		m_queryCandidates.getCandidate(0, nRecordGet, candidates);
		nGet += nRecordGet;
		if (nGet == count)
			return;
		//如果query不够，填上首位拼音汉字组
		for (int i = 0; i != m_singleCandidates.getCandidateCount(); ++i)
		{
			candidates.push_back(m_singleCandidates.getCandidate(i));
			if (++nGet == count)
				return;
		}
	}
	else if ((int)index >= (int)m_combineAndAssociateCandidates.getCandidateCount() && (int)index <= (int)m_combineAndAssociateCandidates.getCandidateCount() + (int)m_queryCandidates.getCandidateCount() - 1)	//开始点落在m_pQueryDeque
	{
		//搜集query的数据
		int nStart = index - m_combineAndAssociateCandidates.getCandidateCount();
		int nRecordGet = nStart + count >= m_queryCandidates.getCandidateCount() ? m_queryCandidates.getCandidateCount() - nStart : count;
		m_queryCandidates.getCandidate(nStart, nRecordGet, candidates);
		nGet += nRecordGet;
		if (nGet == count)
			return;
		//如果query不够，填上首位拼音汉字组
		for (int i = 0; i != m_singleCandidates.getCandidateCount(); ++i)
		{
			candidates.push_back(m_singleCandidates.getCandidate(i));
			if (++nGet == count)
				return;
		}
	}
	else	//开始点落在首位拼音汉字组
	{
		for (int i = index - (m_queryCandidates.getCandidateCount() + m_combineAndAssociateCandidates.getCandidateCount()); i != m_singleCandidates.getCandidateCount(); ++i)
		{
			candidates.push_back(m_singleCandidates.getCandidate(i));
			if (++nGet == count)
				return;
		}
	}
}

int Pinyin_Internal::getFirstLettersHighestWeight(const std::vector<std::string>& pinyins) const
{
	Query query;
	std::string firstLetters;
	for (auto const &p : pinyins)
		firstLetters += (p.substr(0, 1) + '\'');
	firstLetters.pop_back();
	query.search(Query::size | Query::pinyin | Query::cizu | Query::weight, pinyins.size(), Query::Condition::eq, firstLetters, Query::Condition::like, false, true, "", Query::Condition::none, Query::weight);
	int ret = 0;
	if (query.recordCount() != 0)
	{
		char *size = nullptr, *pinyin = nullptr, *cizu = nullptr, *weight = nullptr;
		query.getRecord(0, size, pinyin, cizu, weight);
		ret = std::stoi(weight);
	}
	return ret;
}

void Pinyin_Internal::clear()
{
	m_dividedPinyin.clear();
	m_combineAndAssociateCandidates.clear();
	m_queryCandidates.clear();
	m_singleCandidates.clear();
	CombineLineCache::release();
	QueryLineCache::release();
	SingleCache::release();
}
