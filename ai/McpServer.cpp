#include <OpenThreads/Thread>
#include <osg/Notify>
#include <osg/io_utils>
#include <random>

#include <libhv/all/EventLoop.h>
#include <libhv/all/server/HttpService.h>
#include <libhv/all/server/HttpServer.h>
#include "McpServer.h"
using namespace osgVerse;

static std::string generateSessionID()
{
    std::random_device rd; std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::stringstream ss; ss << std::hex;

    // UUID format: 8-4-4-4-12 hexadecimal digits
    for (int i = 0; i < 8; ++i) ss << dis(gen); ss << "-";
    for (int i = 0; i < 4; ++i) ss << dis(gen); ss << "-";
    for (int i = 0; i < 4; ++i) ss << dis(gen); ss << "-";
    for (int i = 0; i < 4; ++i) ss << dis(gen); ss << "-";
    for (int i = 0; i < 12; ++i) ss << dis(gen); return ss.str();
}

static picojson::value simpleJson(const std::string& idName, int id, const std::string& k0, const std::string& v0,
                                  const std::string& k1 = "", const std::string& v1 = "")
{
    picojson::object result; result[idName] = picojson::value((double)id);
    result["jsonrpc"] = picojson::value("2.0"); if (!k1.empty()) result[k1] = picojson::value(v1);
    if (!k0.empty()) result[k0] = picojson::value(v0); return picojson::value(result);
}

class JsonRpcServer : public osg::Referenced, public OpenThreads::Thread
{
protected:
    struct McpRequest
    {
        picojson::value params; int id;
        std::string version, method;
    };

    virtual ~JsonRpcServer()
    { if (threadPool != NULL) delete threadPool; }

    static int preprocessor(HttpRequest* req, HttpResponse* resp)
    {
        req->ParseBody();
        resp->content_type = APPLICATION_JSON;
        return HTTP_STATUS_UNFINISHED;
    }

    static int postprocessor(HttpRequest* req, HttpResponse* resp)
    {
        //OSG_NOTICE << resp->Dump(true, true) << std::endl;
        return resp->status_code;
    }

    static int responseStatus(const HttpContextPtr& ctx, int code = 200, const char* message = NULL, bool plain = false)
    {
        HttpResponse* resp = ctx->response.get();
        if (message == NULL) message = http_status_str((enum http_status)code);
        resp->Set("code", code); resp->Set("message", message);
        resp->content_type = plain ? TEXT_PLAIN : APPLICATION_JSON;
        ctx->send(); return code;
    }

    int handleJsonRpc(const HttpContextPtr& ctx)
    {
        std::string sessionID = ctx->request->GetParam("session_id");
        if (ctx->request->Method() == "OPTIONS")
            return responseStatus(ctx, 204, "", true);
        OSG_NOTICE << "[JsonRpcServer] POST: " << ctx->body() << std::endl;

        picojson::value root;
        std::string err = picojson::parse(root, ctx->body());
        if (!err.empty())
        {
            OSG_NOTICE << "[JsonRpcServer] Failed to parse '" << ctx->body() << "': " << err << std::endl;
            return responseStatus(ctx, 400, "Invalid JSON");
        }

        McpRequest request;
        if (root.contains("jsonrpc")) request.version = root.get("jsonrpc").get<std::string>();
        if (root.contains("method")) request.method = root.get("method").get<std::string>();
        if (root.contains("id")) request.id = (int)root.get("id").get<double>();
        if (root.contains("params")) request.params = root.get("params");

        JsonRpcServer* rpc = this;
        if (request.method.empty())
            return responseStatus(ctx, 400, "Method not found");
        else if (request.method == "ping")
            return responseStatus(ctx, 202, "Accepted", true);
        else if (sessionID.empty())
            return responseStatus(ctx, 400, "Session not found");
        else if (!root.contains("id"))  // it is a notification
        {
            rpc->threadPool->enqueue([rpc, request, sessionID]()
                { rpc->handleRequest(sessionID, request, true); });
            return responseStatus(ctx, 202, "Accepted", true);
        }
        else
        {
            rpc->threadPool->enqueue([rpc, request, sessionID]()
                {
                    picojson::value result = rpc->handleRequest(sessionID, request);
                    rpc->createEventMessage(sessionID, result.serialize(false), "message");
                });
            return responseStatus(ctx, 202, "Accepted", true);
        }
    }

    int handleSSE(const HttpContextPtr& ctx)
    {
        JsonRpcServer* rpc = this;
        ctx->response->Set("Content-Type", "text/event-stream");
        ctx->response->Set("Cache-Control", "no-cache");
        ctx->response->Set("Connection", "keep-alive");
        ctx->response->Set("Access-Control-Allow-Origin", "*");

        std::string sessionID = ctx->request->GetParam("session_id");
        if (sessionID.empty())
        {
            sessionID = generateSessionID();
            auto thread = std::make_unique<std::thread>([rpc, ctx, sessionID]()
                {
                    std::stringstream ss; ss << rpc->msgEndpoint << "?session_id=" << sessionID;
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    rpc->createEventMessage(sessionID, ss.str(), "endpoint");

                    int heartbeatCount = 0;
                    while (rpc->running && ctx->writer->isConnected())
                    {
                        std::this_thread::sleep_for(std::chrono::seconds(5) + std::chrono::milliseconds(rand() % 500));
                        if (!rpc->running || !ctx->writer->isConnected()) break;
                        rpc->createEventMessage(sessionID, std::to_string(heartbeatCount++), "heartbeat");
                    }
                    rpc->closeSession(sessionID);
                });
            {
                std::lock_guard<std::mutex> lock(rpc->sseMutex);
                rpc->sseThreads[sessionID] = std::move(thread);
            }

            hv::setInterval(200, [rpc, ctx, sessionID](hv::TimerID timerID)
                {
                    if (rpc->running && ctx->writer->isConnected())
                        rpc->sendEventMessage(sessionID, ctx);
                    else
                        hv::killTimer(timerID);
                });
            OSG_NOTICE << "[JsonRpcServer] SSE-new: " << sessionID << std::endl;
            return HTTP_STATUS_UNFINISHED;
        }
        return responseStatus(ctx, 202, "Accepted", true);
    }

    picojson::value handleRequest(const std::string& sessionID,
                                  const McpRequest& req, bool asNotify = false)
    {
        if (asNotify)
        {
            if (req.method == "notifications/initialized")
            {
                std::lock_guard<std::mutex> lock(sseMutex);
                sseInitialized[sessionID] = true;
            }
            else
                OSG_NOTICE << "[JsonRpcServer] notify(" << req.method << ")" << std::endl;
            return picojson::value();
        }

        McpServer::MethodHandler handler = NULL;
        bool initialized = false;
        if (req.method == "initialize")
            return handInitialization(sessionID, req);
        else
        {
            std::lock_guard<std::mutex> lock(sseMutex);
            initialized = sseInitialized[sessionID];
            if (initialized)
            {
                auto it = methods.find(req.method);
                if (it != methods.end()) handler = it->second;
            }
        }

        picojson::object result = simpleJson("id", req.id, "", "").get<picojson::object>();
        if (!initialized)
            result["error"] = simpleJson("code", InvalidRequest, "message", "Session not initialized");
        else if (handler == NULL)
            result["error"] = simpleJson("code", MethodNotFound, "message", "Method " + req.method + " not found");
        else
        {
            picojson::value methodResult = handler(req.params, sessionID);
            if (methodResult.contains("code")) result["error"] = picojson::value(methodResult);
            else result["result"] = picojson::value(methodResult);
        }
        return picojson::value(result);
    }

    picojson::value handInitialization(const std::string& sessionID, const McpRequest& req)
    {
        // Read client information
        McpClientInfo& info = clientMap[sessionID];
        info.protocolVersion = !req.params.contains("protocolVersion") ? "Unknown"
                             : req.params.get("protocolVersion").get<std::string>();
        if (req.params.contains("clientInfo"))
        {
            picojson::value clientInfo = req.params.get("clientInfo");
            info.name = !clientInfo.contains("name") ? "Unknown" : clientInfo.get("name").get<std::string>();
            info.version = !clientInfo.contains("version") ? "0.0" : clientInfo.get("version").get<std::string>();
        }

        picojson::object promptsProps, resourcesProp, toolsProp;  // TODO
        picojson::object capabilities, serverInfo;
        //capabilities["prompts"] = picojson::value();
        //capabilities["resources"] = picojson::value();
        capabilities["tools"] = picojson::value(toolsProp);
        serverInfo["name"] = picojson::value(MCP_SERVER_NAME);
        serverInfo["version"] = picojson::value(MCP_SERVER_VERSION);

        picojson::object infoData;
        infoData["protocolVersion"] = picojson::value(MCP_VERSION);
        infoData["capabilities"] = picojson::value(capabilities);
        infoData["serverInfo"] = picojson::value(serverInfo);

        picojson::object result = simpleJson("id", req.id, "", "").get<picojson::object>();
        result["result"] = picojson::value(infoData); return picojson::value(result);
    }

public:
    JsonRpcServer() : threadPool(NULL), running(true) {}

    void initialize(const std::string& host, int port,
                    const std::string& sse_endpoint, const std::string& msg_endpoint)
    {
        memcpy(server.host, host.c_str(), osg::minimum((int)host.length(), 64));
        server.worker_processes = 0; server.worker_threads = 0;
        server.port = port; server.userdata = this;
        sseEndpoint = sse_endpoint; msgEndpoint = msg_endpoint;

        service.AllowCORS();
        service.preprocessor = JsonRpcServer::preprocessor;
        service.postprocessor = JsonRpcServer::postprocessor;
        service.POST(msg_endpoint.c_str(), [this](const HttpContextPtr& ctx) { return handleJsonRpc(ctx); });
        service.GET(sse_endpoint.c_str(), [this](const HttpContextPtr& ctx) { return handleSSE(ctx); });

        threadPool = new ThreadPool(std::thread::hardware_concurrency());
        server.registerHttpService(&service);
    }

    virtual void run()
    {
        server.start();
        while (running) microSleep(15000);
        server.stop();
    }

    virtual int cancel()
    {
        if (!running) return 0;
        {
            std::lock_guard<std::mutex> lock(sseMutex);
            while (!sseThreads.empty()) { closeSession(sseThreads.begin()->first, false); }
            sseThreads.clear(); sseMessages.clear(); sseInitialized.clear();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        running = false; microSleep(20000);
        return OpenThreads::Thread::cancel();
    }

    void createEventMessage(const std::string& sessionID, const std::string& msg, const std::string& ev)
    {
        std::lock_guard<std::mutex> lock(msgMutex);
        sseMessages[sessionID].push(EventAndData(msg, ev));
    }

    void sendEventMessage(const std::string& sessionID, const HttpContextPtr& ctx)
    {
        std::queue<EventAndData> messages;
        {
            std::lock_guard<std::mutex> lock(msgMutex);
            std::queue<EventAndData>& origin = sseMessages[sessionID];
            if (origin.empty()) return; messages = origin; origin.pop();
        }

        EventAndData& ed = messages.front();
        ctx->writer->SSEvent(ed.first, ed.second.c_str());
        if (ed.second != "heartbeat")
            OSG_NOTICE << "[JsonRpcServer] SSE-" << ed.second << ": " << ed.first << std::endl;
    }

    void removeSessionMember(const std::string& sessionID)
    {
        auto cit = clientMap.find(sessionID);
        auto tit2 = sseMessages.find(sessionID);
        auto tit3 = sseInitialized.find(sessionID);
        if (cit != clientMap.end()) clientMap.erase(cit);
        if (tit2 != sseMessages.end()) sseMessages.erase(tit2);
        if (tit3 != sseInitialized.end()) sseInitialized.erase(tit3);
    }

    void closeSession(const std::string& sessionID, bool useMutex = true)
    {
        for (std::map<std::string, McpServer::SessionCleanupHandler>::iterator it = sessionCleanups.begin();
             it != sessionCleanups.end(); ++it) it->second(it->first, sessionID);

        std::unique_ptr<std::thread> toRelease;
        if (useMutex)
        {
            std::lock_guard<std::mutex> lock(sseMutex);
            auto tit = sseThreads.find(sessionID);
            if (tit != sseThreads.end())
            {
                toRelease = std::move(tit->second);
                sseThreads.erase(tit);
            }
            removeSessionMember(sessionID);
        }
        else
        {
            auto tit = sseThreads.find(sessionID);
            if (tit != sseThreads.end())
            {
                toRelease = std::move(tit->second);
                sseThreads.erase(tit);
            }
            removeSessionMember(sessionID);
        }
        if (toRelease) toRelease.release();
    }

    typedef std::pair<std::string, std::string> EventAndData;
    std::map<std::string, McpClientInfo> clientMap;
    std::map<std::string, std::unique_ptr<std::thread>> sseThreads;
    std::map<std::string, std::queue<EventAndData>> sseMessages;
    std::map<std::string, bool> sseInitialized;

    typedef std::pair<osg::ref_ptr<McpTool>, McpServer::MethodHandler> ToolData;
    std::map<std::string, McpServer::MethodHandler> methods;
    std::map<std::string, McpServer::NotificationHandler> notifications;
    std::map<std::string, McpServer::SessionCleanupHandler> sessionCleanups;
    std::map<std::string, osg::ref_ptr<McpResource>> resources;
    std::map<std::string, ToolData> tools;
    McpServer::AuthenticationHandler authentication;

    ThreadPool* threadPool;
    hv::HttpServer server;
    hv::HttpService service;
    std::string sseEndpoint, msgEndpoint;
    std::mutex sseMutex, msgMutex;
    bool running;
};

McpServer::McpServer(const std::string& sse_endpoint, const std::string& msg_endpoint)
{
    JsonRpcServer* server = new JsonRpcServer; _core = server;
    _sseRoute = sse_endpoint; _jsonRoute = msg_endpoint;
}

McpServer::~McpServer()
{
    JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
    server->cancel(); server->join(); _core = NULL;
}

void McpServer::start(const std::string& host, int port)
{
    JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
    server->initialize(host, port, _sseRoute, _jsonRoute); server->start();
}

void McpServer::stop()
{
    JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
    server->cancel(); server->join();
}

std::map<std::string, McpClientInfo> McpServer::getClients() const
{
    const JsonRpcServer* server = static_cast<const JsonRpcServer*>(_core.get());
    return server->clientMap;
}

void McpServer::setAuthenticationHandler(AuthenticationHandler handler)
{
    JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
    std::lock_guard<std::mutex> lock(server->sseMutex);
    server->authentication = handler;
}

void McpServer::registerSessionCleanup(const std::string& key, SessionCleanupHandler handler)
{
    JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
    std::lock_guard<std::mutex> lock(server->sseMutex);
    server->sessionCleanups[key] = handler;
}

void McpServer::registerMethod(const std::string& method, MethodHandler handler)
{
    JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
    std::lock_guard<std::mutex> lock(server->sseMutex);
    server->methods[method] = handler;
}

void McpServer::registerTool(McpTool* tool, MethodHandler handler)
{
    JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
    std::lock_guard<std::mutex> lock(server->sseMutex);
    server->tools[tool->name] = std::pair<osg::ref_ptr<McpTool>, MethodHandler>(tool, handler);

    if (server->methods.find("tools/list") == server->methods.end())
        server->methods["tools/list"] = [this](const picojson::value& params, const std::string& id)
        {
            JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get()); picojson::array toolsData;
            for (std::map<std::string, JsonRpcServer::ToolData>::iterator it = server->tools.begin();
                 it != server->tools.end(); ++it) toolsData.push_back(it->second.first->json());

            picojson::object toolsRoot; toolsRoot["tools"] = picojson::value(toolsData);
            toolsRoot["nextCursor"] = picojson::value("next-page-cursor"); return picojson::value(toolsRoot);
        };
    if (server->methods.find("tools/call") == server->methods.end())
        server->methods["tools/call"] = [this](const picojson::value& params, const std::string& id)
        {
            JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
            if (!params.contains("name"))
                return simpleJson("code", InvalidParams, "message", "Missing 'name' parameter");

            std::string name = params.get("name").get<std::string>(); auto it = server->tools.find(name);
            if (it == server->tools.end())
                return simpleJson("code", InvalidParams, "message", "Tool " + name + " not found");

            picojson::object result; picojson::array content;
            try
            {
                picojson::value args = params.contains("arguments") ? params.get("arguments") : picojson::value();
                content.push_back(it->second.second(args, id));  // { 'type': 'text', 'text': '...' }
                result["content"] = picojson::value(content);
            }
            catch (const std::exception& e)
            {
                picojson::object errorData; errorData["type"] = picojson::value("text");
                errorData["text"] = picojson::value(e.what()); content.push_back(picojson::value(errorData));
                result["content"] = picojson::value(content); result["isError"] = picojson::value(true);
            }
            OSG_NOTICE << "[JsonRpcServer] tools/call(" << name << "): "
                       << picojson::value(result).serialize(false) << std::endl;
            return picojson::value(result);
        };
}

void McpServer::registerNotification(const std::string& notify, NotificationHandler handler)
{
    JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
    std::lock_guard<std::mutex> lock(server->sseMutex);
    server->notifications[notify] = handler;
}

void McpServer::registerResource(const std::string& path, McpResource* resource)
{
    JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
    std::lock_guard<std::mutex> lock(server->sseMutex);
    server->resources[path] = resource;

    if (server->methods.find("resources/read") == server->methods.end())
        server->methods["resources/read"] = [this](const picojson::value& params, const std::string& id)
        {
            JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
            if (!params.contains("uri")) return simpleJson("code", InvalidParams, "message", "Missing 'uri' parameter");
            std::string name = params.get("uri").get<std::string>(); auto it = server->resources.find(name);
            if (it == server->resources.end())
                return simpleJson("code", InvalidParams, "message", "Resource " + name + " not found");

            // TODO: read from resource
            return picojson::value();
        };
    if (server->methods.find("resources/list") == server->methods.end())
        server->methods["resources/list"] = [this](const picojson::value& params, const std::string& id)
        {
            // TODO: list resources
            return picojson::value();
        };
    if (server->methods.find("resources/subscribe") == server->methods.end())
        server->methods["resources/subscribe"] = [this](const picojson::value& params, const std::string& id)
        {
            // TODO: subscribe resources
            return picojson::value();
        };
}
