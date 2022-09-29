#include "Cache.h"
#include <algorithm>
#include "PinyinHanzi.h"
#include "StringFunction.h"

using namespace ime::pinyin;

//为了兼容多个引擎，全局变量应该对象化
std::map<std::string, std::shared_ptr<Query>>		g_queryLineCaches;
std::map<std::string, std::shared_ptr<Query>>		g_combineAssociateLineCaches;
std::map<std::string, std::vector<std::string>>		g_SingleCaches;

std::shared_ptr<Query> QueryLineCache::getQuery(const std::string & validPinyinStr)
{
	auto iter = g_queryLineCaches.find(validPinyinStr);
	if (iter == g_queryLineCaches.end())
	{
		std::shared_ptr<Query> p = std::make_shared<Query>();
		//搜索size=，pinyin like的条目
		p->search(Query::pinyin | Query::cizu | Query::weight, std::count(validPinyinStr.begin(), validPinyinStr.end(), '\'') + 1, Query::Condition::eq,
			validPinyinStr, Query::Condition::like, false, false, "", Query::Condition::none, Query::weight);
		g_queryLineCaches.insert({ validPinyinStr, p });
		return p;
	}
	else
	{
		return iter->second;
	}
}

void QueryLineCache::release()
{
	g_queryLineCaches.clear();
}

///////////////////
std::shared_ptr<Query> CombineLineCache::getQuery(const std::string & validPinyinStr)
{
	auto iter = g_combineAssociateLineCaches.find(validPinyinStr);
	if (iter == g_combineAssociateLineCaches.end())
	{
		std::shared_ptr<Query> p = std::make_shared<Query>();
		//搜索size=，pinyin like的条目
		p->search(Query::pinyin | Query::cizu | Query::priority, std::count(validPinyinStr.begin(), validPinyinStr.end(), '\'') + 1, Query::Condition::eq,
			validPinyinStr, Query::Condition::like, false, false, "", Query::Condition::none, Query::priority);
		g_combineAssociateLineCaches.insert({ validPinyinStr, p });
		return p;
	}
	else
	{
		return iter->second;
	}
}

void CombineLineCache::release()
{
	g_combineAssociateLineCaches.clear();
}

///////////////
void ime::pinyin::SingleCache::getHanzis(const std::string & pinyin, std::vector<std::string>& hanzis)
{
	if (pinyin.empty())
		return;

	std::map<std::string, std::vector<std::string>>::const_iterator iter = g_SingleCaches.find(pinyin);
	if (iter == g_SingleCaches.end())
	{
		std::wstring wsHanzis;
		PinyinHanzi::getHanzis(pinyin, wsHanzis);
		for (int i = 0; i != wsHanzis.size(); ++i)
			hanzis.push_back(StringFunction::unicodeToUtf8(wsHanzis.substr(i, 1)));
	}
	else
	{
		hanzis = iter->second;
	}
}

void SingleCache::release()
{
	g_SingleCaches.clear();
}

////////////////////
void QueryLine::search(const std::string &input)
{
	m_querys.clear();
	std::string sQueryPinyin = input;
	size_t n = sQueryPinyin.rfind('\'');
	while (n != std::string::npos)
	{
		m_querys.push_back(QueryLineCache::getQuery(sQueryPinyin));
		sQueryPinyin.assign(sQueryPinyin.begin(), sQueryPinyin.begin() + n);
		n = sQueryPinyin.rfind('\'');
	}
}

unsigned int QueryLine::queryCount() const
{
	return m_querys.size();
}

std::shared_ptr<Query> QueryLine::at(unsigned int index) const
{
	return m_querys[index];
}

void QueryLine::clear()
{
	m_querys.clear();
}

void CombineLine::search(const std::string & input)
{
	std::string sQueryPinyin = input;
	size_t n = sQueryPinyin.rfind('\'');
	while (n != std::string::npos)
	{
		m_querys.push_back(CombineLineCache::getQuery(sQueryPinyin));
		sQueryPinyin.assign(sQueryPinyin.begin(), sQueryPinyin.begin() + n);
		n = sQueryPinyin.rfind('\'');
	}
}

bool CombineLine::getFirstCandidate(Candidate &can) const
{
	for (auto query : m_querys)
	{
		if (query->recordCount() != 0)
		{
			char *pinyin = nullptr, *cizu = nullptr;
			query->getRecord(0, pinyin, cizu);
			std::string sPinyin(pinyin);
			can = { CandidateType::Combine, (int)std::count(sPinyin.begin(), sPinyin.end(), '\'') + 1 , pinyin, cizu };
			return true;
		}
	}
	return false;
}
