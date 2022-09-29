#pragma once
#include <string>
#include "Share.h"
#include "CandidateStubs.h"
#include "pinyin/Pinyin.h"
#include "PinyinDivider.h"

namespace ime{ namespace pinyin{

class Pinyin_Internal final
{
public:
	Pinyin_Internal();
	~Pinyin_Internal();
	Pinyin_Internal(const Pinyin_Internal &other) = delete;
	void operator =(const Pinyin_Internal &other) = delete;

	bool init(const std::string &systemCikuPath, const std::string &learnCikuPath);
	bool hasInit() const;
	bool deinit();

	void enableAICombineCandidate(bool enable);
	bool isEnableAICombineCandidate() const;
	void enableAssociateCandidate(bool enable);
	bool isEnableAssociateCandidate() const;

	bool addUserCiku(const std::string &path);
	bool remoteUserCiku(const std::string &path);
	void clearUserCiku();
	void getUserCikuPaths(std::vector<std::string> &paths) const;

	void setCandidatePageSize(unsigned int size);
	unsigned int getCandidatePageSize() const;

	bool search(const std::string &input);
	std::string getSearchingDividedPinyin() const;

	unsigned int getCandidatePageCount() const;
	void getCandidateByPage(unsigned int page, std::vector<std::string> &candidates) const;
	unsigned int getCandidateCount() const;
	void getCandidate(unsigned int index, unsigned int count, std::vector<std::string> &candidates) const;

	void getCandidateInfo(unsigned int page, unsigned int index, CandidateInfo &info) const;
	void getCandidateInfo(unsigned int index, CandidateInfo &info) const;

	bool learn(const std::string &pinyin, const std::string &cizu, bool &alreadyExist);
	bool forget(const std::string &pinyin, const std::string &cizu);
	bool promote(const std::string &pinyin, const std::string &cizu);

private:
	void checkInit() const;
	bool checkLegalPinyinString(const std::string &input);
	void getCandidate(unsigned int index, unsigned int count, std::vector<Candidate> &candidates) const;
	int getFirstLettersHighestWeight(const std::vector<std::string> &pinyins) const;
	void clear();

	bool							m_init;
	bool							m_AICombineCandidate;
	bool							m_associateCandidate;
	unsigned int					m_candidatePageSize;

	PinyinDivider					m_divider;
	std::string						m_dividedPinyin;
	CombineAssociateCandidate		m_combineAndAssociateCandidates;
	QueryCandidate					m_queryCandidates;
	SinglePinyinCandidate			m_singleCandidates;
};

}}