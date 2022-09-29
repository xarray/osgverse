#include "Query.h"
#include <vector>
#include "sqlite3.h"
#include "StringFunction.h"
#include "PinyinBase.h"
#include "CikuManager.h"
#include "Ciku.h"

using namespace ime::pinyin;

Query::Query()
	: m_data(nullptr)
	, m_row(0)
	, m_col(0)
{
}

Query::~Query()
{
	release();
}

bool Query::search(int fields, int size, Condition sizeCondition, const std::string &validPinyinStr, Condition pinyinCondition, bool extended, bool hazy, const std::string &cizu, Condition cizuCondition, Field orderBy)
{
	release();

	std::string sQueryPinyin = getQueryPinyin(validPinyinStr, extended, hazy);
	std::string sSelectDistinct = getSelectDistinct(fields);
	std::string sSelectInner = getSelectInner(fields, size, sizeCondition, sQueryPinyin, pinyinCondition, cizu, cizuCondition);
	std::string sOrderString = getOrderBy(orderBy);
	std::string sCmd = sSelectDistinct + sSelectInner + sOrderString;

	char *pErrorMsg = nullptr;
	return sqlite3_get_table(Ciku::sqlHandle(), sCmd.data(), &m_data, &m_row, &m_col, &pErrorMsg) == SQLITE_OK;
}

void Query::release()
{
	sqlite3_free_table(m_data);
	m_data = nullptr;
	m_row = m_col = 0;
}

unsigned int Query::recordCount() const
{
	return m_row;
}

std::string Query::getQueryPinyin(const std::string & validPinyinStr, bool extended, bool hazy) const
{
	std::vector<std::string> pinyins;
	StringFunction::split(validPinyinStr, "'", pinyins);
	if (pinyins.empty())
		return "";

	std::string ret;
	for (auto iter = pinyins.begin(); iter != pinyins.end(); ++iter)
	{
		const std::string &one = *iter;
		if (hazy)
		{
			ret += iter == --pinyins.end() ? (one + "%") : (one + "%''");
		}
		else
		{
			if (iter == --pinyins.end())
			{
				if (extended)
					ret += PinyinBase::isCompleted(one) ? (one + "''%") : (one + "%");		//ta'po't 模糊查询 [踏破铁鞋无觅处]等
				else
					ret += PinyinBase::isCompleted(one) ? one : (one + "%");				//完整的拼音不需要%
			}
			else
			{
				ret += PinyinBase::isCompleted(one) ? (one + "''") : (one + "%''");
			}
		}
	}
	return ret;
}

std::string Query::getSelectDistinct(int fields) const
{
	std::string ret = "select distinct ";
	std::string sFieldStr;
	if (testFiled(fields, Query::size))		sFieldStr += "size, ";
	if (testFiled(fields, Query::pinyin))	sFieldStr += "pinyin, ";
	if (testFiled(fields, Query::cizu))		sFieldStr += "cizu, ";
	if (testFiled(fields, Query::priority))	sFieldStr += "priority, ";
	if (testFiled(fields, Query::weight))	sFieldStr += "weight, ";
	sFieldStr.erase(sFieldStr.end() - 2, sFieldStr.end());
	ret += (sFieldStr + " from (");
	return ret;
}

std::string Query::getSelectInner(int fields, int size, Condition sizeCondition, const std::string &sQueryPinyin, Condition pinyinCondition, const std::string &cizu, Condition cizuCondition) const
{
	std::string ret;
	std::string sTabName = sQueryPinyin.substr(0, 1) + "_" + sQueryPinyin[sQueryPinyin.find("'") + 2];
	//内表
	for (auto ciku : CikuManager::getCikus())
	{
		const std::string &alias = ciku->alias();
		std::string sOneSelect = "select ";
		if (testFiled(fields, Query::size))		sOneSelect += (alias + "." + sTabName + ".size, ");
		if (testFiled(fields, Query::pinyin))	sOneSelect += (alias + "." + sTabName + ".pinyin, ");
		if (testFiled(fields, Query::cizu))		sOneSelect += (alias + "." + sTabName + ".cizu, ");
		if (testFiled(fields, Query::priority))	sOneSelect += (alias + "." + sTabName + ".priority, ");
		if (testFiled(fields, Query::weight))	sOneSelect += (alias + "." + sTabName + ".weight, ");
		sOneSelect.erase(sOneSelect.end() - 2, sOneSelect.end());
		sOneSelect += (" from " + alias + "." + sTabName + " where ");

		switch (sizeCondition)
		{
		case Condition::eq:	sOneSelect += ("size = " + std::to_string(size) + " and ");				break;
		default:																					break;
		}
		switch (pinyinCondition)
		{
		case Query::Condition::eq:		sOneSelect += ("pinyin = '" + sQueryPinyin + "' and ");		break;
		case Query::Condition::like:	sOneSelect += ("pinyin like '" + sQueryPinyin + "' and ");	break;
		default:																					break;
		}
		switch (cizuCondition)
		{
		case Query::Condition::eq:		sOneSelect += ("cizu = '" + cizu + "' and ");				break;
		case Query::Condition::like:	sOneSelect += ("cizu like '" + cizu + "' and ");			break;
		default:																					break;
		}

		sOneSelect = sOneSelect.substr(0, sOneSelect.size() - 5) + " union ";
		ret += sOneSelect;
	}
	//-7表示截掉最后的" union "
	ret = ret.substr(0, ret.size() - 7) + ")";
	return ret;
}

std::string Query::getOrderBy(Field orderBy) const
{
	std::string ret;
	switch (orderBy)
	{
	case Query::size:		ret = " order by size desc";		break;
	case Query::pinyin:		ret = " order by pinyin desc";		break;
	case Query::cizu:		ret = " order by cizu desc";		break;
	case Query::priority:	ret = " order by priority desc";	break;
	case Query::weight:		ret = " order by weight desc";		break;
	default:													break;
	}
	return ret;
}

bool Query::testFiled(int fields, Field f) const
{
	return ((fields & f) == f) && (f != 0 || fields == f);
}
