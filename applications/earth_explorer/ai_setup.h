#ifndef EARTH_AI_SETUP_H
#define EARTH_AI_SETUP_H
// Earth-side assembly of the AI assistant: built-in tool registration + default system prompt +
// workspace context snapshot.
// All tool execute callbacks run on the main thread (AIChatCore::drainMainThread), so they can
// safely touch the scene graph.
#include "ai_chat.h"
#include "LayerRegistry.h"
#include "DataSource.h"

namespace osgVerse { class EarthManipulator; }

namespace earthai
{
    // Register the built-in tools (fly_to / set_layer / get_view_state / get_data_summary).
    // fly_to is not registered when manipulator is NULL (other tools are unaffected).
    void registerEarthAITools(ToolRegistry& registry, LayerRegistry* layers,
                              earthds::DataSourceManager* sources,
                              osgVerse::EarthManipulator* manipulator);

    // Default system prompt (can be overridden by the ai.system_prompt config)
    std::string defaultEarthSystemPrompt();

    // Workspace snapshot (JSON) automatically collected before each user instruction: camera
    // position + layer/data source state.
    // Read on the main thread (called once during submit).
    std::string buildEarthContextJson(osgVerse::EarthManipulator* manipulator,
                                      LayerRegistry* layers,
                                      earthds::DataSourceManager* sources);
}
#endif
