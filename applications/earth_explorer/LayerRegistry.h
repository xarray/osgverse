#ifndef EARTH_LAYER_REGISTRY_H
#define EARTH_LAYER_REGISTRY_H

#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <vector>
#include "marker_style.h"

// Unified layer registry (ported from the friend's LayerManager, stripped of project
// serialization/presets/opacity).
// The UI (ImGui draw callback, possibly on the draw thread) only writes setEnabled; the actual
// scene graph changes (apply callbacks) go through a pending queue, executed one by one by
// drainPending() on the main thread — avoiding direct scene graph edits from the draw thread.
struct OverlayLayer
{
    enum Type { RasterTile, PointFeed };
    std::string id, displayName, group, subtitle;
    Type type = PointFeed;
    bool enabled = false;
    bool needsKey = false;        // UI greys out when the key is missing
    earthmark::MarkerShape shape = earthmark::MarkerShape::Circle;
    osg::Vec4 iconColor = osg::Vec4(0.7f, 0.7f, 0.72f, 1.0f);
    // 0=not fetched/idle, 1=success with features, 2=failed (out gets the error text), 3=success but 0 features
    std::function<int(std::string&)> fetchStatus;
    // Called on the main thread when enabled changes (attach/detach scene subgraph, start/stop threads, etc.)
    std::function<void(const OverlayLayer&)> apply;
    // Feature-list hooks (PointFeed layers): the UI enumerates the loaded features
    // (title list) and can activate one to show its detail card. Empty = no features.
    std::function<size_t()> featureCount;
    std::function<std::string(size_t)> featureTitle;
    std::function<void(size_t)> featureActivate;
    // Feature location in degrees (for "double click to fly there"). false = out of range.
    std::function<bool(size_t, double&, double&)> featureLatLon;
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

    // Called by the UI thread: only enqueue, do not touch the scene directly
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

    // Called by the main thread every frame: apply pending commands (run apply callbacks)
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

    // Called by the UI draw thread: full snapshot copy (the layer count is small, so copying is cheap)
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
