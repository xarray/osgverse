#include "pinyin/Pinyin.h"
#include "Pinyin_Internal.h"
#include <exception>
#include <cstdlib>

using namespace ime::pinyin;

static bool g_instanced = false;

Pinyin::Pinyin()
	: m_internal(nullptr)
{
	if (!g_instanced)
	{
		m_internal = new Pinyin_Internal();
		g_instanced = true;
	}
	else
	{
		printf("not allow to create two pinyin engines in one process, exit with code 1.\n");
		std::exit(1);
	}
}

Pinyin::~Pinyin()
{
    g_instanced = false;
	delete m_internal;
}

bool Pinyin::init(const std::string & systemCikuPath, const std::string & learnCikuPath)
{
	return m_internal->init(systemCikuPath, learnCikuPath);
}

bool Pinyin::hasInit() const
{
	return m_internal->hasInit();
}

bool Pinyin::deinit()
{
	return m_internal->deinit();
}

void Pinyin::enableAICombineCandidate(bool enable)
{
	m_internal->enableAICombineCandidate(enable);
}

bool Pinyin::isEnableAICombineCandidate() const
{
	return m_internal->isEnableAICombineCandidate();
}

void Pinyin::enableAssociateCandidate(bool enable)
{
	m_internal->enableAssociateCandidate(enable);
}

bool Pinyin::isEnableAssociateCandidate() const
{
	return m_internal->isEnableAssociateCandidate();
}

bool Pinyin::addUserCiku(const std::string & path)
{
	return m_internal->addUserCiku(path);
}

bool Pinyin::remoteUserCiku(const std::string & path)
{
	return m_internal->remoteUserCiku(path);
}

void Pinyin::clearUserCiku()
{
	m_internal->clearUserCiku();
}

void Pinyin::getUserCikuPaths(std::vector<std::string>& paths) const
{
	m_internal->getUserCikuPaths(paths);
}

void Pinyin::setCandidatePageSize(unsigned int size)
{
	m_internal->setCandidatePageSize(size);
}

unsigned int Pinyin::getCandidatePageSize() const
{
	return m_internal->getCandidatePageSize();
}

bool Pinyin::search(const std::string & input)
{
	return m_internal->search(input);
}

std::string Pinyin::getSearchingDividedPinyin() const
{
	return m_internal->getSearchingDividedPinyin();
}

unsigned int Pinyin::getCandidatePageCount() const
{
	return m_internal->getCandidatePageCount();
}

void Pinyin::getCandidateByPage(unsigned int page, std::vector<std::string>& candidates) const
{
	m_internal->getCandidateByPage(page, candidates);
}

unsigned int Pinyin::getCandidateCount() const
{
	return m_internal->getCandidateCount();
}

void Pinyin::getCandidate(unsigned int index, unsigned int count, std::vector<std::string>& candidates)
{
	m_internal->getCandidate(index, count, candidates);
}

void Pinyin::getCandidateInfo(unsigned int page, unsigned int index, CandidateInfo & info) const
{
	m_internal->getCandidateInfo(page, index, info);
}

void Pinyin::getCandidateInfo(unsigned int index, CandidateInfo & info) const
{
	m_internal->getCandidateInfo(index, info);
}

bool Pinyin::learn(const std::string & pinyin, const std::string & cizu, bool &alreadyExist)
{
	return m_internal->learn(pinyin, cizu, alreadyExist);
}

bool Pinyin::forget(const std::string & pinyin, const std::string & cizu)
{
	return m_internal->forget(pinyin, cizu);
}

bool Pinyin::promote(const std::string & pinyin, const std::string & cizu)
{
	return m_internal->promote(pinyin, cizu);
}
