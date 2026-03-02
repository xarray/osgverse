#ifndef MANA_AI_MCPSERVER_HPP
#define MANA_AI_MCPSERVER_HPP

#include <osg/ref_ptr>
#include <osg/Referenced>
#include <picojson.h>
#include <vector>
#include <queue>
#include <map>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <regex>

namespace osgVerse
{

#define MCP_VERSION "2025-03-26"
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

    /** Tool */
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
            toolData["inputSchema"] = picojson::value(inputSchema);
            if (hasExtra) toolData["annotations"] = picojson::value(extra); return picojson::value(toolData);
        }
    };

    /** Static resource */
    struct McpResource : public osg::Referenced
    {
        enum MimeType
        {
            TextPlain = 0, TextHtml, TextMarkdown, ApplicationJson,
            ImageJPG, ImagePNG, BinaryData, CustomMime
        };
        std::string customMimeType;
        std::string name, description;
        bool subscribeEnabled;

        McpResource() : subscribeEnabled(false) {}
        virtual picojson::value read() const = 0;
        virtual bool modified() const { return false; }
        virtual std::string uri() const { return name; }
        virtual MimeType getMimeType() const { return TextPlain; }
        std::string getMimeTypeString() const { return mimeTypeString(getMimeType(), customMimeType); }

        virtual picojson::value metadata() const
        {
            picojson::object meta;
            meta["uri"] = picojson::value(uri());
            meta["name"] = picojson::value(name);
            meta["description"] = picojson::value(description);
            meta["mimeType"] = picojson::value(getMimeTypeString());
            if (subscribeEnabled) {
                meta["annotations"] = picojson::value(picojson::object());
                meta["annotations"].get<picojson::object>()["subscribe"] = picojson::value(true);
            }
            return picojson::value(meta);
        }

        static std::string mimeTypeString(MimeType t, const std::string& custom = "")
        {
            switch (t)
            {
            case TextHtml: return "text/html"; case TextMarkdown: return "text/markdown";
            case ImageJPG: return "image/jpeg"; case ImagePNG: return "image/png";
            case ApplicationJson: return "application/json";
            case BinaryData: return "application/octet-stream";
            case CustomMime: return custom;
            default: return "text/plain";
            }
        }
    };

    /** Template for dynamic resource objects */
    struct McpResourceTemplate : public osg::Referenced
    {
        typedef std::function<picojson::value (const std::map<std::string, std::string>&,
                                               const std::string&)> ResourceHandler;
        ResourceHandler handler;
        std::string uriTemplate, name, description;
        McpResource::MimeType mimeType;
        bool subscribeEnabled;
        McpResourceTemplate(const std::string& tmpl, const std::string& n, const std::string& d)
            : uriTemplate(tmpl), name(n), description(d), mimeType(McpResource::TextPlain), subscribeEnabled(false) {}

        bool match(const std::string& uri, std::map<std::string, std::string>& params) const
        {
            std::string pattern = uriTemplate; size_t pos = 0;
            std::vector<std::string> paramNames;
            while ((pos = pattern.find("{", pos)) != std::string::npos)
            {
                size_t end = pattern.find("}", pos);
                if (end == std::string::npos) return false;

                std::string paramName = pattern.substr(pos + 1, end - pos - 1);
                paramNames.push_back(paramName);
                pattern.replace(pos, end - pos + 1, "([^/]+)");
                pos += 4; // length of ([^/]+)
            }

            pos = 0;  // find and replace \*
            while ((pos = pattern.find(".", pos)) != std::string::npos)
            {
                if (!(pos == 0 || pattern[pos - 1] != '\\')) pos += 1;
                else { pattern.replace(pos, 1, "\\."); pos += 2; }
            }

            std::regex re("^" + pattern + "$"); std::smatch match;
            if (!std::regex_match(uri, match, re)) return false;
            for (size_t i = 1; i < match.size() && i - 1 < paramNames.size(); ++i)
                params[paramNames[i - 1]] = match[i].str();
            return true;
        }

        picojson::value metadata() const
        {
            picojson::object meta;
            meta["uriTemplate"] = picojson::value(uriTemplate);
            meta["name"] = picojson::value(name);
            meta["description"] = picojson::value(description);
            meta["mimeType"] = picojson::value(McpResource::mimeTypeString(mimeType));
            if (subscribeEnabled)
            {
                meta["annotations"] = picojson::value(picojson::object());
                meta["annotations"].get<picojson::object>()["subscribe"] = picojson::value(true);
            }
            return picojson::value(meta);
        }
    };

    /** Prompt definition */
    struct McpPrompt : public osg::Referenced
    {
        typedef std::pair<std::string, std::string> ArgumentData; // description, required
        std::map<std::string, ArgumentData> arguments;
        std::string name, description;
        McpPrompt(const std::string& n, const std::string& d) : name(n), description(d) {}

        void addArgument(const std::string& argName, const std::string& argDesc, bool required = false)
        { arguments[argName] = ArgumentData(argDesc, required ? "required" : "optional"); }

        picojson::value metadata() const
        {
            picojson::object prompt;
            prompt["name"] = picojson::value(name);
            prompt["description"] = picojson::value(description);
            if (!arguments.empty())
            {
                picojson::array args;
                for (auto& arg : arguments)
                {
                    picojson::object argObj;
                    argObj["name"] = picojson::value(arg.first);
                    argObj["description"] = picojson::value(arg.second.first);
                    argObj["required"] = picojson::value(arg.second.second == "required");
                    args.push_back(picojson::value(argObj));
                }
                prompt["arguments"] = picojson::value(args);
            }
            return picojson::value(prompt);
        }
    };

    /** Prompt messages used in prompt registering */
    struct McpPromptMessage
    {
        enum Role { User, Assistant } role;
        McpResource::MimeType contentType;
        std::string content, customMimeType;
        McpPromptMessage(Role r, const std::string& text, McpResource::MimeType mime = McpResource::TextPlain)
            : role(r), contentType(mime), content(text) {}

        picojson::value json() const
        {
            picojson::object msg, contentObj;
            msg["role"] = picojson::value(role == User ? "user" : "assistant");
            if (contentType == McpResource::TextPlain || contentType == McpResource::TextHtml ||
                contentType == McpResource::TextMarkdown || contentType == McpResource::ApplicationJson)
            {
                contentObj["type"] = picojson::value("text");
                contentObj["text"] = picojson::value(content);
            }
            else if (contentType == McpResource::ImageJPG || contentType == McpResource::ImagePNG)
            {
                contentObj["type"] = picojson::value("image");
                contentObj["data"] = picojson::value(content); // base64
                contentObj["mimeType"] = picojson::value(McpResource::mimeTypeString(contentType, customMimeType));
            }
            else if (contentType == McpResource::BinaryData || contentType == McpResource::CustomMime)
            {
                contentObj["type"] = picojson::value("resource");
                if (content.substr(0, 7) == "file://" ||
                    content.substr(0, 7) == "http://" || content.substr(0, 8) == "https://")
                {
                    contentObj["resource"] = picojson::value(picojson::object());
                    contentObj["resource"].get<picojson::object>()["uri"] = picojson::value(content);
                }
                else
                {
                    contentObj["resource"] = picojson::value(picojson::object());
                    contentObj["resource"].get<picojson::object>()["data"] = picojson::value(content);
                }
                contentObj["resource"].get<picojson::object>()["mimeType"] =
                    picojson::value(McpResource::mimeTypeString(contentType, customMimeType));
            }

            picojson::array contents; contents.push_back(picojson::value(contentObj));
            msg["content"] = picojson::value(contents); return picojson::value(msg);
        }
    };

    /** Model Context Protocol server implementation
        1. Authentication (optional):
        server->setAuthenticationHandler([](const std::string& apiKey, const std::string& sessionID) {
            std::cout << "[Auth] Session: " << sessionID << ", Key: " << apiKey.substr(0, 4) << "****" << std::endl;
            return apiKey == "my-secret-key";  // check "Bear my-secret-key" in your own way
        });

        2. Tool:
        class CreateFileTool : public McpTool {
        public:
            CreateFileTool() : McpTool("create_file", "Create a text file on local disk") {
                addProperty("path", "File path", StringType | RequiredType);
            }
        };
        server->registerTool(new CreateFileTool(), [](const picojson::value& params, const std::string& id) {
            std::string path = params.get("path").get<std::string>();
            ... (Acutally create and handle file)
            return McpServer::methodResult("Created: " + path + "\n(Anything else...)");
        });
        //!! CLINE: Please create a new file /home/wang/readme.txt

        3. Register resource
        McpResourceTemplate* fileTemplate = new McpResourceTemplate(
            "file:///{path}", "Local file", "Read file content from local disk");
        fileTemplate->handler = [](const std::map<std::string, std::string>& params, const std::string& id) -> picojson::value {
            std::string path = params.at("path"); std::stringstream buffer;
            ... (Load file content)
            return McpServer::resourceContent("file:///" + path, buffer.str(), "text/plain");
        };
        server->registerResourceTemplate(fileTemplate);  // dynamic file
        //!! CLINE: Please check my config @file:///etc/nginx/nginx.conf

        class FileResource : public McpResource {
            std::string _path, _uri;
        public:
            FileResource(const std::string& path, const std::string& uri) : _path(path), _uri(uri)
            { name = path; description = "Local file"; subscribeEnabled = true; }

            virtual picojson::value read() const {
                std::stringstream buffer;  ... (Load file content)
                return McpServer::resourceContent(_uri, buffer.str(), getMimeTypeString());
            }
            virtual std::string uri() const { return _uri; }
        };
        server->registerResource("file:///etc/hosts", new FileResource("/etc/hosts", "file:///etc/hosts"));  // static file
        //!! CLINE: Please check @file:///etc/hosts to see if it is available

        4. Prompt (with/without progress tracking)
        McpPrompt* analyzePrompt = new McpPrompt("analyze-project", "Analyze project structure");
        analyzePrompt->addArgument("project_path", "Project path", true);
        server->registerPrompt(analyzePrompt, [&server](const picojson::value& args, const std::string& id) -> std::vector<McpPromptMessage> {
            std::string path = args.get("project_path").get<std::string>();
            ProgressTracker tracker(&server, id, "analysis-token-" + id, 100.0);
            std::string result; tracker.update(0, "Start loading: " + path);
            for (int i = 0; i < 10; ++i)
            {
                ... (Actually load the project information to result)
                tracker.increment(10, "Progress: " + std::to_string((i + 1) * 10) + "%");
            }
            tracker.finish("Completed");

            std::vector<McpPromptMessage> messages;
            messages.push_back(McpPromptMessage(
                McpPromptMessage::User, "Current project information: " + result +
                                        "\nPlease analyze the project and see if it has any problems",
                McpResource::TextPlain));
            return messages;
        });
        //!! CLINE: /analyze-project (and input project_path)

        McpPrompt* codeReviewPrompt = new McpPrompt("code-review", "Source code reviewer");
        codeReviewPrompt->addArgument("language", "Code language", true);
        codeReviewPrompt->addArgument("code", "Source code to review", true);
        server->registerPrompt(codeReviewPrompt, codeReviewHandler);
        //!! CLINE: /code-review (and input language and code manually)

        5. CLINE configuation (settings.json):
        {
            "cline.mcpServers": {
                "osgverse-mcp": {
                    "url": "http://localhost:8080/sse", "timeout": 120,
                    "headers": { "Authorization": "Bearer my-secret-key" }
                }
            }
        }
    */
    class McpServer : public osg::Referenced
    {
    public:
        typedef std::function<picojson::value (const picojson::value&, const std::string&)> MethodHandler;
        typedef std::function<void (const picojson::value&, const std::string&)> NotificationHandler;
        typedef std::function<bool (const std::string&, const std::string&)> AuthenticationHandler;
        typedef std::function<void (const std::string&, const std::string&)> SessionCleanupHandler;
        typedef std::function<std::vector<McpPromptMessage> (const picojson::value&, const std::string&)> PromptHandler;
        typedef std::function<picojson::value (const picojson::value&, const std::string&)> SamplingHandler;
        typedef std::function<void (const std::string&, int, const picojson::value&)> ProgressCallback;

        static picojson::value methodResult(const std::string& data)
        {
            picojson::object result; result["type"] = picojson::value("text");
            result["text"] = picojson::value(data); return picojson::value(result);
        }

        static picojson::value resourceContent(const std::string& uri, const std::string& text,
                                               const std::string& mimeType = "text/plain",
                                               const picojson::object& meta = picojson::object())
        {
            picojson::object content;
            content["uri"] = picojson::value(uri);
            content["mimeType"] = picojson::value(mimeType);
            content["text"] = picojson::value(text);
            if (!meta.empty()) content["metadata"] = picojson::value(meta);
            return picojson::value(content);
        }

        static picojson::value resourceContentBinary(const std::string& uri, const std::string& base64Data,
                                                     const std::string& mimeType = "application/octet-stream",
                                                     const picojson::object& meta = picojson::object())
        {
            picojson::object content;
            content["uri"] = picojson::value(uri);
            content["mimeType"] = picojson::value(mimeType);
            content["blob"] = picojson::value(base64Data);
            if (!meta.empty()) content["metadata"] = picojson::value(meta);
            return picojson::value(content);
        }

        static picojson::value progressNotification(const std::string& token, double progress,
                                                    double total = 0.0, const std::string& message = "")
        {
            picojson::object params, notify;
            params["progressToken"] = picojson::value(token);
            params["progress"] = picojson::value(progress);
            if (total > 0) params["total"] = picojson::value(total);
            if (!message.empty()) params["message"] = picojson::value(message);

            notify["jsonrpc"] = picojson::value("2.0");
            notify["method"] = picojson::value("notifications/progress");
            notify["params"] = picojson::value(params);
            return picojson::value(notify);
        }

        static picojson::value cancelledNotification(const std::string& requestId, const std::string& reason = "")
        {
            picojson::object params, notify;
            params["requestId"] = picojson::value(requestId);
            if (!reason.empty()) params["reason"] = picojson::value(reason);

            notify["jsonrpc"] = picojson::value("2.0");
            notify["method"] = picojson::value("notifications/cancelled");
            notify["params"] = picojson::value(params);
            return picojson::value(notify);
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
        void registerRootsHandler(MethodHandler handler);
        void registerNotification(const std::string& notify, NotificationHandler handler);
        void registerResource(const std::string& path, McpResource* resource);
        void registerResourceTemplate(McpResourceTemplate* resourceTemplate);
        void registerPrompt(McpPrompt* prompt, PromptHandler handler);
        void registerSamplingHandler(SamplingHandler handler);

        void notifyPromptListChanged();
        void notifyResourceUpdated(const std::string& uri);
        void notifyResourceListChanged();

        picojson::value paginateResults(const picojson::array& allItems, const std::string& cursor,
                                        int pageSize, const std::string& listType);
        void sendProgress(const std::string& sessionID, const std::string& token, double progress,
                          double total = 0.0, const std::string& message = "");
        void sendCancelled(const std::string& sessionID, const std::string& requestId, const std::string& reason = "");

        void subscribeResource(const std::string& sessionID, const std::string& uri);
        void unsubscribeResource(const std::string& sessionID, const std::string& uri);
        bool isResourceSubscribed(const std::string& sessionID, const std::string& uri) const;

    protected:
        virtual ~McpServer();

        osg::ref_ptr<osg::Referenced> _core;
        std::string _sseRoute, _jsonRoute;
    };

    /** Track current progress while handling tools/resources */
    class ProgressTracker
    {
    public:
        ProgressTracker(McpServer* srv, const std::string& sid, const std::string& token, double total = 100.0)
            : _server(srv), _sessionID(sid), _progressToken(token), _totalSteps(total), _currentStep(0) {}

        void update(double step, const std::string& msg = "")
        { _currentStep = step; _server->sendProgress(_sessionID, _progressToken, _currentStep, _totalSteps, msg); }

        void increment(double delta = 1.0, const std::string& msg = "") { update(_currentStep + delta, msg); }
        void finish(const std::string& msg = "Completed") { update(_totalSteps, msg); }

    private:
        McpServer* _server;
        std::string _sessionID, _progressToken;
        double _totalSteps, _currentStep;
    };

}

#endif
