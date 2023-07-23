/**********************************************
*	编写者：	Pan.瞳
*
*	编写日期：	2013-10-8
*
*	文件名称：	Pinyin.h
*
*	描述：		提供拼音输入法接口
*
*	说明：		要求所有字符串编码为utf8
*
************************************************/
#pragma once
#include <string>
#include <vector>

//#ifdef WIN32
//	#define IME_API	__declspec(dllexport)
//	#pragma warning(disable: 4251)
//#else
//	#define IME_API
//#endif
#define IME_API

namespace ime{ namespace pinyin{

struct IME_API CandidateInfo
{
	bool		canForget;		//是否可以被遗忘（可被遗忘的词只能是词库中的词组，不包括组合词以及单个汉字） //
	std::string pinyin;			//在词库中真正的完整拼音					：wo'zai //
	std::string cizu;			//候选词形如								：我在 //
	std::string devidedPinyin;	//候选词对应的分割后的拼音，包含分隔符形如  ：w'zai //
};

class Pinyin_Internal;
class IME_API Pinyin final
{
public:
	Pinyin();
	~Pinyin();
	Pinyin(const Pinyin &other) = delete;
	void operator =(const Pinyin &other) = delete;

	bool init(const std::string &systemCikuPath, const std::string &learnCikuPath);

	bool hasInit() const;

	bool deinit();

	// 智能组合词开关(默认开)。搜索"wo'zaizheli"，如果没有4长度的词，可能将"wo'zai'zhe"和"li"的结果进行组合成为候选词。 //
	void enableAICombineCandidate(bool enable);
	bool isEnableAICombineCandidate() const;

	// 联想开关（默认开）。搜索"zai'suo"， 如果对应的拼音无词条，继而会搜索"zai'suo*"，因此“在所难免”、“在所不辞”等可能会成为候选词）。 //
	void enableAssociateCandidate(bool enable);
	bool isEnableAssociateCandidate() const;

	bool addUserCiku(const std::string &path);
	bool remoteUserCiku(const std::string &path);
	void clearUserCiku();

	void getUserCikuPaths(std::vector<std::string> &paths) const;

	// 每页候选词的个数（默认为 5） //
	void setCandidatePageSize(unsigned int size);
	unsigned int getCandidatePageSize() const;

	/* 执行搜索
	input：由且仅由26小写字母和分隔符"'"组成且以字母开头的序列（分隔符可有可无且不可连续）。合法拼音如：wo'ai'ni；woaini；wo'aini等；
    非法拼音如：'ni'de、wo__ni+df'de、wo''de等
	返回值：true表示成功，false表示失败 */
	bool search(const std::string &input);

	// 获取正在搜索的拼音的分割模式，比如正在搜索woaini返回wo'ai'ni //
	std::string getSearchingDividedPinyin() const;

	// 获取搜索结果的候选词页数 //
	unsigned int getCandidatePageCount() const;

	void getCandidateByPage(unsigned int page, std::vector<std::string> &candidates) const;
	unsigned int getCandidateCount() const;
	void getCandidate(unsigned int index, unsigned int count, std::vector<std::string> &candidates);

	void getCandidateInfo(unsigned int page, unsigned int index, CandidateInfo &info) const;
	void getCandidateInfo(unsigned int index, CandidateInfo &info) const;

	/* 遗忘词条（调用后会清空所有的搜索结果）
	pinyin：完整拼音，形如：wo'de，ni'hao，不合法的拼音如：wod'e，wode，w(de，w'd等
	cizu：汉字词组，不需要和词组长度一致
	alreadyExist：词条是否已经存在
	返回值：成功返回true（如果词条已经存在也返回true），否则返回false。 */
	bool learn(const std::string &pinyin, const std::string &cizu, bool &alreadyExist);

	/* 遗忘词条（调用后会清空所有的搜索结果）
	pinyin：完整拼音，形如：wo'de，ni'hao，不合法的拼音如：wod'e，wode，w(de，w'd等
	cizu：词组
	返回值：成功返回true，否则返回false。（实际删除条目数为0也返回true) */
	bool forget(const std::string &pinyin, const std::string &cizu);

	/* 提升词频到最高（调用后会清空所有的搜索结果），比如提升"wo'de"的词频，该次将出现在w'd的首位
	pinyin：完整拼音，形如：wo'de，ni'hao，不合法的拼音如：wod'e，wode，w(de，w'd等
	cizu：词组
	返回值：成功返回true，否则返回false。（词频未提升也返回true) */
	bool promote(const std::string &pinyin, const std::string &cizu);

private:
	Pinyin_Internal		*m_internal;
};

}}
