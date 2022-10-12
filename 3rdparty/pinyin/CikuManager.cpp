#include "CikuManager.h"
#include "Ciku.h"
#include "StringFunction.h"
#include "PinyinBase.h"
#include "sqlite3.h"
#include <algorithm>

using namespace ime::pinyin;

std::string	CikuManager::m_systemCikuPath;
std::string	CikuManager::m_learnCikuPath;
std::vector<std::shared_ptr<Ciku>> CikuManager::m_cikus;

void CikuManager::configure(const std::string & systemCikuPath, const std::string & learnCikuPath)
{
	m_systemCikuPath = systemCikuPath;
	m_learnCikuPath = learnCikuPath;
}

bool CikuManager::init()
{
	//添加system-db和learn-db
	return addUserCiku(m_systemCikuPath) & addUserCiku(m_learnCikuPath);
}

bool CikuManager::deinit()
{
	m_cikus.clear();
	Ciku::freeSqlHandle();
	return true;
}

bool CikuManager::addUserCiku(const std::string &path)
{
	std::shared_ptr<Ciku> ciku = std::make_shared<Ciku>();
	if (ciku->load(path))
	{
		m_cikus.push_back(ciku);
		return true;
	}
	else
	{
		printf("load ciku [%s] fail.\n", path.data());
		return false;
	}
}

bool CikuManager::remoteUserCiku(const std::string &path)
{
	for (unsigned int i = 2; i < m_cikus.size(); ++i)
	{
		if (m_cikus[i]->path() == path)
		{
			m_cikus.erase(m_cikus.begin() + i);
			return true;
		}
	}
	return false;
}

void CikuManager::clearUserCiku()
{
	if(m_cikus.size() > 2)
		m_cikus.erase(m_cikus.begin() + 2, m_cikus.end());
}

void CikuManager::getAllUserCikuPaths(std::vector<std::string> &cikuPaths)
{
	for (unsigned int i = 2; i != cikuPaths.size(); ++i)
		cikuPaths.push_back(m_cikus[i]->path());
}

bool CikuManager::insertRecord(int size, const std::string & completedPinyinStr, const std::string & cizu, int priority, int weight)
{
	size_t n = completedPinyinStr.find('\'');
	if (completedPinyinStr.empty() || n == std::string::npos || n == completedPinyinStr.size() - 1)
		return false;

	std::string pinyin = StringFunction::replace(completedPinyinStr, "'", "''");
	std::string sTabName = pinyin.substr(0, 1) + "_" + pinyin[n + 2];

	std::shared_ptr<Ciku> learnCiku = m_cikus[1];
	std::string sCmd = "insert into " + learnCiku->alias() + "." + sTabName + " values ('" + std::to_string(size) + "', '" + pinyin + "', '" + cizu + "', '" + std::to_string(priority) + "', '" + std::to_string(weight) + "')";
	char *pErrorMsg = nullptr;
	return sqlite3_exec(Ciku::sqlHandle(), sCmd.data(), nullptr, nullptr, &pErrorMsg) == SQLITE_OK;
}

bool CikuManager::deleteRecord(const std::string & completedPinyinStr, const std::string & cizu)
{
	size_t n = completedPinyinStr.find('\'');
	if (completedPinyinStr.empty() || n == std::string::npos || n == completedPinyinStr.size() - 1)
		return false;

	std::string pinyin = StringFunction::replace(completedPinyinStr, "'", "''");
	std::string sTabName = pinyin.substr(0, 1) + "_" + pinyin[n + 2];
	//一次删除一个词库的数据
	bool bRet = true;
	for (auto ciku : m_cikus)
	{
		std::string sCmd = "delete from " + ciku->alias() + "." + sTabName + " where pinyin = '" + pinyin + "' and cizu = '" + cizu + "'";
		char *pErrorMsg = nullptr;
		bRet &= (sqlite3_exec(Ciku::sqlHandle(), sCmd.data(), nullptr, nullptr, &pErrorMsg) == SQLITE_OK);
	}
	return bRet;
}

bool CikuManager::updateRecord(const std::string & srcCompletedPinyin, const std::string & srcCizu, int newSize, const std::string & newCompletedPinyinStr, const std::string & newCizu, int newPriority, int newWeight)
{
	size_t n = srcCompletedPinyin.find('\'');
	if (srcCompletedPinyin.empty() || n == std::string::npos || n == srcCompletedPinyin.size() - 1)
		return false;

	std::string pinyin = StringFunction::replace(srcCompletedPinyin, "'", "''");
	std::string sTabName = pinyin.substr(0, 1) + "_" + pinyin[n + 2];

	std::string sNewPinyin = StringFunction::replace(newCompletedPinyinStr, "'", "''");
	bool ret = false;
	for (auto ciku : m_cikus)
	{
		std::string sCmd = "update " + ciku->alias() + "." + sTabName + " set ";
		if (newSize != -1)		sCmd += ("size = " + std::to_string(newSize) + ", ");
		if(!sNewPinyin.empty())	sCmd += ("pinyin = " + sNewPinyin + ", ");
		if(!newCizu.empty())	sCmd += ("cizu = " + newCizu + ", ");
		if (newPriority != -1)	sCmd += ("priority = " + std::to_string(newPriority) + ", ");
		if (newWeight != -1)	sCmd += ("weight = " + std::to_string(newWeight) + ", ");
		sCmd.erase(sCmd.end() - 2);
		sCmd += (" where pinyin = '" + StringFunction::replace(srcCompletedPinyin, "'", "''") + "' and cizu = '" + srcCizu + "'");

		char *pErrorMsg = nullptr;
		ret |= (sqlite3_exec(Ciku::sqlHandle(), sCmd.data(), nullptr, nullptr, &pErrorMsg) == SQLITE_OK);
	}
	return ret;
}

std::vector<std::shared_ptr<Ciku>> CikuManager::getCikus()
{
	return m_cikus;
}
