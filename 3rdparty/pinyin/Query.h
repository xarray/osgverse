#pragma once
#include <string>
#include <map>
#include <memory>
#include <exception>

namespace ime{ namespace pinyin{

class Query
{
public:
	enum Field
	{
		none		= 0,
		size		= 0x00000001 << 0,
		pinyin		= 0x00000001 << 1,
		cizu		= 0x00000001 << 2,
		priority	= 0x00000001 << 3,
		weight		= 0x00000001 << 4,
	};

	enum class Condition
	{
		none,	//不做条件判断
		eq,		//	=
		neq,	//	!=
		gt,		//	>
		egt,	//	>=
		lt,		//	<
		elt,	//	<=
		like,	//	like
		notlike,//	not like
		//between,
		//null,
		//notnull,
	};

public:
	Query();
	~Query();

	//执行搜索
	//fields：传入 Size | Pinyin这样的形式来表达字段组合
	//size：长度
	//sizeCondition：size条件
	//validPinyinStr：拼音串
	//pinyinCondition：pinyin条件
	//extended：是否拓展，（ta'po't 拓展查询 [踏破铁鞋无觅处]等
	//cizu：词组
	//cizuCondition：词组条件
	//orderBy：排序字段，传入none表示不排序
	bool search(int fields, int size, Condition sizeCondition, const std::string &validPinyinStr, Condition pinyinCondition, bool extended, bool hazy, const std::string &cizu, Condition cizuCondition, Field orderBy);

	//释放搜索数据
	void release();

	//记录个数
	unsigned int recordCount() const;

	//传入不定个数引用作为字段结果，引用参数应该与构造函数的field对应
	template<class T, class ...Args>
	void getRecord(unsigned int index, T &head, Args&... rest)
	{
		char **p = m_data + m_col;
		head = p[index * m_col + i];
		++i;
		getRecord(index, rest...);
	}

private:
	int i = 0;
	void getRecord(int index) { i = 0; }

	std::string getQueryPinyin(const std::string &validPinyinStr, bool extended, bool hazy) const;
	std::string	getSelectDistinct(int fields) const;
	std::string getSelectInner(int fields, int size, Condition sizeCondition, const std::string &sQueryPinyin, Condition pinyinCondition, const std::string &cizu, Condition cizuCondition) const;
	std::string getOrderBy(Field orderBy) const;
	bool testFiled(int fields, Field f) const;

	char	**m_data;
	int		m_row;
	int		m_col;
};

}}