#include "Ciku.h"
#include "sqlite3.h"
#include "CikuManager.h"
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

using namespace ime::pinyin;
#define ALIAS_PREFIX	"DB_"
sqlite3	*Ciku::m_sqlHandle = nullptr;

bool Ciku::load(const std::string & path)
{
	if (access(path.data(), 0) != 0)
		return false;

	if(sqlHandle() == nullptr)
	{
		if (sqlite3_open(path.data(), &m_sqlHandle) != SQLITE_OK)
			return false;
	}

	char *pErrorMsg = nullptr;
	std::string aliasName = calcNextAliasName();
	std::string sCmd = "attach database \"" + path + "\" as \"" + aliasName + "\"";
	if (sqlite3_exec(sqlHandle(), sCmd.data(), nullptr, nullptr, &pErrorMsg) == SQLITE_OK)
	{
		m_path = path;
		m_alias = aliasName;
		return true;
	}
	else
	{
		return false;
	}
}

const std::string & Ciku::path() const
{
	return m_path;
}

const std::string & Ciku::alias() const
{
	return m_alias;
}

sqlite3 *Ciku::sqlHandle()
{
	return m_sqlHandle;
}

void Ciku::freeSqlHandle()
{
	sqlite3_close(m_sqlHandle);
	m_sqlHandle = nullptr;
}

std::string Ciku::calcNextAliasName() const
{
	int count = CikuManager::getCikus().size();
	return ALIAS_PREFIX + std::to_string(count);
}
