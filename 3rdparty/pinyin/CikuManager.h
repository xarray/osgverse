/*************************************************
*	CikuManager词库管理
*		类型分为系统词库,学习词库和用户词库,
*		其中系统词库和学习词库在引擎初始化时加载
*
**************************************************/
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <tuple>

namespace ime{ namespace pinyin{

class Ciku;
class CikuManager final
{
public:
	CikuManager() = delete;
	~CikuManager() = delete;

	static void configure(const std::string &systemCikuPath, const std::string &learnCikuPath);

	static bool init();
	static bool deinit();

	static bool addUserCiku(const std::string &path);

	static bool remoteUserCiku(const std::string &path);

	static void clearUserCiku();

	static void getAllUserCikuPaths(std::vector<std::string> &cikuPaths);

	static std::vector<std::shared_ptr<Ciku>> getCikus();

	static bool insertRecord(int size, const std::string &completedPinyinStr, const std::string &cizu, int priority, int weight);

	static bool deleteRecord(const std::string &completedPinyinStr, const std::string &cizu);

	static bool updateRecord(const std::string &srcCompletedPinyin, const std::string &srcCizu, int newSize, const std::string &newCompletedPinyinStr, const std::string &newCizu, int newPriority, int newWeight);

private:
	static std::string								m_systemCikuPath;
	static std::string								m_learnCikuPath;
	static std::vector<std::shared_ptr<Ciku>>		m_cikus;
};

}}
