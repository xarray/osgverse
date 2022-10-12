#pragma once
#include <vector>
#include "Query.h"
#include "Share.h"

namespace ime{ namespace pinyin{

class QueryLineCache
{
public:
	static std::shared_ptr<Query> getQuery(const std::string &validPinyinStr);
	static void release();
};

class CombineLineCache
{
public:
	static std::shared_ptr<Query> getQuery(const std::string &validPinyinStr);
	static void release();
};

class SingleCache
{
public:
	static void getHanzis(const std::string &pinyin, std::vector<std::string> &hanzis);
	static void release();
};

class QueryLine
{
public:
	void search(const std::string &input);
	unsigned int queryCount() const;
	std::shared_ptr<Query> at(unsigned int index) const;
	void clear();

private:
	std::vector<std::shared_ptr<Query>>					m_querys;
};

class CombineLine
{
public:
	void search(const std::string &input);
	bool getFirstCandidate(Candidate &can) const;

private:
	std::vector<std::shared_ptr<Query>>					m_querys;
};

}}