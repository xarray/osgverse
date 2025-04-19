#ifndef MANA_AI_MCPSERVER_HPP
#define MANA_AI_MCPSERVER_HPP

#include <osg/ref_ptr>
#include <osg/Referenced>
#include <picojson.h>
#include <vector>
#include <map>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace osgVerse
{

#define MCP_VERSION "2024-11-05"
#define MCP_SERVER_NAME "osgverse_mcp"
#define MCP_SERVER_VERSION "1.0.0"

    enum ErrorCode
    {   // Standard JSON-RPC error codes
        ParseError = -32700,
        InvalidRequest = -32600,
        MethodNotFound = -32601,
        InvalidParams = -32602,
        InternalError = -32603
    };

    class ThreadPool
    {
    public:
        ThreadPool(size_t numThreads) : _stopped(false)
        {
            for (size_t i = 0; i < numThreads; ++i)
            {
                _threads.emplace_back([this]
                    {
                        while (true)
                        {
                            std::function<void()> task;
                            {
                                std::unique_lock<std::mutex> lock(_mutex);
                                _condition.wait(lock, [this] { return _stopped || !_tasks.empty(); });
                                if (_stopped && _tasks.empty()) return;
                                task = std::move(_tasks.front()); _tasks.pop();
                            }
                            task();
                        }
                    });
            }
        }

        ~ThreadPool()
        {
            { std::unique_lock<std::mutex> lock(_mutex); _stopped = true; } _condition.notify_all();
            for (size_t i = 0; i < _threads.size(); ++i) { _threads[i].join(); }
        }

        template <class F, class... Args> void enqueue(F&& func, Args &&... args)
        {
            { std::unique_lock<std::mutex> lock(_mutex); _tasks.emplace(std::bind(func, args...)); }
            _condition.notify_one();
        }

    private:
        std::vector<std::thread> _threads;
        std::queue<std::function<void()>> _tasks;
        std::condition_variable _condition;
        std::mutex _mutex; bool _stopped;
    };

    struct McpClientInfo : public osg::Referenced
    {
        std::string name, version;
        std::string protocolVersion;
    };

    struct McpTool : public osg::Referenced
    {
        enum PropertyType { StringType = 0x0001, NumberType = 0x0002, BoolType = 0x0004,
                            ArrayType = 0x0100, RequiredType = 0x1000 };
        typedef std::pair<std::string, int> PropertyData;

        std::string name, title, description;
        std::map<std::string, PropertyData> properties;
        bool readOnly, destructive, idempotent, openWorld;

        McpTool(const std::string& n, const std::string& d = "")
            : name(n), description(d), readOnly(false), destructive(false), idempotent(false), openWorld(false) {}
        void addProperty(const std::string& name, const std::string& description, int propertyType)
        { properties[name] = PropertyData(description, propertyType); }

        picojson::value json() const
        {
            picojson::object prop; picojson::array required;
            for (std::map<std::string, PropertyData>::const_iterator it = properties.begin();
                 it != properties.end(); it++)
            {
                picojson::object arg, item; int value = it->second.second;
                bool isArray = (value & ArrayType) > 0, isRequired = (value & RequiredType) > 0;
                switch (value & 0x00ff)
                {
                case BoolType:
                    arg["type"] = picojson::value("boolean"); arg["description"] = picojson::value(it->second.first);
                    if (isArray)
                    {
                        item["type"] = picojson::value("boolean"); arg["items"] = picojson::value(item);
                        arg["type"] = picojson::value("array");
                    } break;
                case NumberType:
                    arg["type"] = picojson::value("number"); arg["description"] = picojson::value(it->second.first);
                    if (isArray)
                    {
                        item["type"] = picojson::value("number"); arg["items"] = picojson::value(item);
                        arg["type"] = picojson::value("array");
                    } break;
                default:
                    arg["type"] = picojson::value("string"); arg["description"] = picojson::value(it->second.first);
                    if (isArray)
                    {
                        item["type"] = picojson::value("string"); arg["items"] = picojson::value(item);
                        arg["type"] = picojson::value("array");
                    } break;
                }
                prop[it->first] = picojson::value(arg);
                if (isRequired) required.push_back(picojson::value(it->first));
            }

            picojson::object inputSchema, extra; bool hasExtra = false;
            inputSchema["type"] = picojson::value("object");
            inputSchema["properties"] = picojson::value(prop);
            inputSchema["required"] = picojson::value(required);

            if (!title.empty()) { extra["title"] = picojson::value(title); hasExtra = true; }
            if (readOnly) { extra["readOnlyHint"] = picojson::value(readOnly); hasExtra = true; }
            if (destructive) { extra["destructiveHint"] = picojson::value(destructive); hasExtra = true; }
            if (idempotent) { extra["idempotentHint"] = picojson::value(idempotent); hasExtra = true; }
            if (openWorld) { extra["openWorldHint"] = picojson::value(openWorld); hasExtra = true; }

            picojson::object toolData; toolData["name"] = picojson::value(name);
            toolData["description"] = picojson::value(description);
            if (!properties.empty()) toolData["inputSchema"] = picojson::value(inputSchema);
            if (hasExtra) toolData["annotations"] = picojson::value(extra); return picojson::value(toolData);
        }
    };

    class McpResource : public osg::Referenced
    {
    public:
        virtual picojson::value metadata() const = 0;
        virtual picojson::value read() const = 0;
        virtual bool modified() const { return false; }
        virtual std::string uri() const { return ""; }
    };

    /** Model Context Protocol server implementation */
    class McpServer : public osg::Referenced
    {
    public:
        typedef std::function<picojson::value (const picojson::value&, const std::string&)> MethodHandler;
        typedef std::function<void (const picojson::value&, const std::string&)> NotificationHandler;
        typedef std::function<bool(const std::string&, const std::string&)> AuthenticationHandler;
        typedef std::function<void(const std::string&, const std::string&)> SessionCleanupHandler;

        static picojson::value methodResult(const std::string& data)
        {
            picojson::object result; result["type"] = picojson::value("text");
            result["text"] = picojson::value(data); return picojson::value(result);
        }

        McpServer(const std::string& sse_endpoint = "/sse",
                  const std::string& msg_endpoint = "/jsonrpc");

        void start(const std::string& host, int port);
        void stop();
        std::map<std::string, McpClientInfo> getClients() const;

        void setAuthenticationHandler(AuthenticationHandler handler);
        void registerSessionCleanup(const std::string& key, SessionCleanupHandler handler);

        void registerMethod(const std::string& method, MethodHandler handler);
        void registerTool(McpTool* tool, MethodHandler handler);
        void registerNotification(const std::string& notify, NotificationHandler handler);
        void registerResource(const std::string& path, McpResource* resource);

    protected:
        virtual ~McpServer();

        osg::ref_ptr<osg::Referenced> _core;
        std::string _sseRoute, _jsonRoute;
    };

}

#endif
