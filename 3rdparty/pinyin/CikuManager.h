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

	//配置system-db和learn-db
	static void configure(const std::string &systemCikuPath, const std::string &learnCikuPath);

	//初始化，加载system-db和learn-db
	static bool init();
	static bool deinit();

	//添加一个用户词库
	static bool addUserCiku(const std::string &path);

	//删除一个用户词库
	static bool remoteUserCiku(const std::string &path);

	//删除所有的用户词库
	static void clearUserCiku();

	//获取所有的用户词库路径
	static void getAllUserCikuPaths(std::vector<std::string> &cikuPaths);

	//获取所有词库
	static std::vector<std::shared_ptr<Ciku>> getCikus();

	//插入一个词条
	//长度、pinyin、cizu、优先级、权重
	static bool insertRecord(int size, const std::string &completedPinyinStr, const std::string &cizu, int priority, int weight);

	//删除一个词条
	//completedPinyinStr：形如 "wo'ai'ni"，必须是完整的
	//cizu：形如："我爱你"
	static bool deleteRecord(const std::string &completedPinyinStr, const std::string &cizu);

	//更新
	static bool updateRecord(const std::string &srcCompletedPinyin, const std::string &srcCizu, int newSize, const std::string &newCompletedPinyinStr, const std::string &newCizu, int newPriority, int newWeight);

private:
	//所有的词库，前两位为system词库和learn词库
	static std::string								m_systemCikuPath;
	static std::string								m_learnCikuPath;
	static std::vector<std::shared_ptr<Ciku>>		m_cikus;
};

}}