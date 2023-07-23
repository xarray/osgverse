/**********************************************
*	Ciku词库
*
*	1、使用sqlite3
*
*	2、多数据库联合查询需要给数据库起别名，数据库队列
*		别名规则DB_0到DB_n
*
************************************************/
#pragma once
#include <string>

struct sqlite3;

namespace ime{ namespace pinyin{

class Ciku final
{
public:
	Ciku() = default;
	~Ciku() = default;
	Ciku(const Ciku &other) = delete;
	void operator = (const Ciku &other) = delete;

	bool load(const std::string &path);

	const std::string &path() const;

	const std::string &alias() const;

	static sqlite3 *sqlHandle();
	static void freeSqlHandle();

private:
	std::string calcNextAliasName() const;

	static sqlite3	*m_sqlHandle;
	std::string		m_path;
	std::string		m_alias;
};

}}
