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

    struct McpTool : public osg::Referenced
    {
        std::string name, description;
        picojson::value parameters;
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

        McpServer(const std::string& sse_endpoint = "/sse",
                  const std::string& msg_endpoint = "/jsonrpc");

        void start(const std::string& host, int port);
        void stop();

        void setAuthenticationHandler(AuthenticationHandler handler);
        void registerSessionCleanup(const std::string& key, SessionCleanupHandler handler);

        void registerMethod(const std::string& method, MethodHandler handler);
        void registerTool(const std::string& name, McpTool* tool, MethodHandler handler);
        void registerNotification(const std::string& notify, NotificationHandler handler);
        void registerResource(const std::string& path, McpResource* resource);

    protected:
        virtual ~McpServer();

        osg::ref_ptr<osg::Referenced> _core;
        std::string _sseRoute, _jsonRoute;
    };

}

#endif
