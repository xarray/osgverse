#ifndef MANA_AI_MCPSERVER_HPP
#define MANA_AI_MCPSERVER_HPP

#include <osg/ref_ptr>
#include <osg/Referenced>
#include <picojson.h>
#include <vector>
#include <map>
#include <functional>

namespace osgVerse
{

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
        typedef std::function<void(const std::string&)> SessionCleanupHandler;

        McpServer(const std::string& host, int port);
        std::vector<osg::ref_ptr<McpTool>> getTools() const;

        void setAuthenticationHandler(AuthenticationHandler handler);
        void registerSessionCleanup(const std::string& key, SessionCleanupHandler handler);

        void registerMethod(const std::string& method, MethodHandler handler);
        void registerTool(const std::string& tool, MethodHandler handler);
        void registerNotification(const std::string& notify, NotificationHandler handler);
        void registerResource(const std::string& path, McpResource* resource);

    protected:
        virtual ~McpServer();
        osg::ref_ptr<osg::Referenced> _core;
    };

}

#endif
