#ifndef EARTH_LAYER_REGISTRY_H
#define EARTH_LAYER_REGISTRY_H

#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <vector>
#include "marker_style.h"

// 统一图层注册表(移植自朋友版 LayerManager,剥离 project 序列化/预设/透明度)。
// UI(ImGui 绘制回调,可能在 draw 线程)只写 setEnabled;真正的场景图变更
// (apply 回调)经由 pending 队列,由主线程的 drainPending() 逐条执行——
// 避免 draw 线程直接改场景图。
struct OverlayLayer
{
    enum Type { RasterTile, PointFeed };
    std::string id, displayName, group, subtitle;
    Type type = PointFeed;
    bool enabled = false;
    bool needsKey = false;        // 缺 key 时 UI 灰显
    earthmark::MarkerShape shape = earthmark::MarkerShape::Circle;
    osg::Vec4 iconColor = osg::Vec4(0.7f, 0.7f, 0.72f, 1.0f);
    // 0=未抓取/idle,1=成功且有要素,2=失败(out 填错误文案),3=成功但 0 要素
    std::function<int(std::string&)> fetchStatus;
    // enabled 变化时在主线程调用(挂/摘场景子图、启停线程等)
    std::function<void(const OverlayLayer&)> apply;
};

class LayerRegistry
{
public:
    OverlayLayer& add(const OverlayLayer& l)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _layers.push_back(l);
        return _layers.back();
    }

    // UI 线程调用:只排队,不直接改场景
    void setEnabled(const std::string& id, bool on)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        OverlayLayer* layer = findUnlocked(id);
        if (!layer) return;
        layer->enabled = on;
        PendingCommand command;
        command.id = id; command.enabled = on;
        _pending.push_back(command);
    }

    // 主线程每帧调用:应用 pending 命令(执行 apply 回调)
    size_t drainPending()
    {
        size_t count = 0;
        for (;;)
        {
            PendingCommand command;
            {
                std::lock_guard<std::mutex> lock(_mutex);
                if (_pending.empty()) break;
                command = _pending.front(); _pending.pop_front();
            }
            OverlayLayer layerCopy; std::function<void(const OverlayLayer&)> apply;
            {
                std::lock_guard<std::mutex> lock(_mutex);
                OverlayLayer* layer = findUnlocked(command.id);
                if (!layer) continue;
                layer->enabled = command.enabled;
                layerCopy = *layer; apply = layer->apply;
            }
            if (apply) apply(layerCopy);
            ++count;
        }
        return count;
    }

    // UI 绘制线程调用:整份快照拷贝(层数量小,拷贝廉价)
    std::vector<OverlayLayer> layersSnapshot() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _layers;
    }

private:
    struct PendingCommand { std::string id; bool enabled = false; };
    OverlayLayer* findUnlocked(const std::string& id)
    {
        for (size_t i = 0; i < _layers.size(); ++i)
            if (_layers[i].id == id) return &_layers[i];
        return NULL;
    }
    mutable std::mutex _mutex;
    std::deque<PendingCommand> _pending;
    std::vector<OverlayLayer> _layers;
};

#endif
