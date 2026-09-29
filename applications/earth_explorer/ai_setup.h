#ifndef EARTH_AI_SETUP_H
#define EARTH_AI_SETUP_H
// AI 助手的地球侧装配:内置工具注册 + 默认系统提示词 + 工作区上下文快照。
// 工具 execute 全部在主线程(AIChatCore::drainMainThread)执行,可安全碰场景图。
#include "ai_chat.h"
#include "LayerRegistry.h"
#include "DataSource.h"

namespace osgVerse { class EarthManipulator; }

namespace earthai
{
    // 注册内置工具(fly_to / set_layer / get_view_state / get_data_summary)。
    // manipulator 为 NULL 时 fly_to 不注册(其它工具不受影响)。
    void registerEarthAITools(ToolRegistry& registry, LayerRegistry* layers,
                              earthds::DataSourceManager* sources,
                              osgVerse::EarthManipulator* manipulator);

    // 默认系统提示词(配置 ai.system_prompt 可覆盖)
    std::string defaultEarthSystemPrompt();

    // 每条用户指令前自动采集的工作区快照(JSON):相机位置 + 图层/数据源状态。
    // 在主线程读(submit 时调用一次)。
    std::string buildEarthContextJson(osgVerse::EarthManipulator* manipulator,
                                      LayerRegistry* layers,
                                      earthds::DataSourceManager* sources);
}
#endif
