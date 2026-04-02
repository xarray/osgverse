#include <OpenThreads/Thread>
#include <osg/Notify>
#include <osg/io_utils>
#include <random>
#include <set>

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
        std::string version, method, progressToken;
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

        if (authentication)
        {
            std::string authHeader = ctx->request->GetHeader("Authorization");
            std::string apiKey = ctx->request->GetParam("api_key");
            if (!authHeader.empty())
            {
                if (authHeader.substr(0, 7) == "Bearer ") apiKey = authHeader.substr(7);
                else apiKey = authHeader;
            }

            if (!authentication(apiKey, sessionID))
            {
                OSG_NOTICE << "[JsonRpcServer] Authentication failed" << std::endl;
                return responseStatus(ctx, 401, "Unauthorized");
            }
        }

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
        if (request.params.contains("_meta"))
        {
            picojson::value meta = request.params.get("_meta");
            if (meta.contains("progressToken"))
                request.progressToken = meta.get("progressToken").get<std::string>();
        }

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
            else if (req.method == "notifications/resources/updated")
            {
                OSG_NOTICE << "[JsonRpcServer] Resource updated notification received" << std::endl;
            }
            else if (req.method == "notifications/cancelled")
            {
                std::string reqId = "unknown";
                if (req.params.contains("requestId"))
                    reqId = req.params.get("requestId").get<std::string>(); cancelledRequests.insert(reqId);
                OSG_NOTICE << "[JsonRpcServer] Request cancelled: " << reqId << std::endl;
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
            std::string reqIdStr = std::to_string(req.id);
            if (cancelledRequests.find(reqIdStr) != cancelledRequests.end())
            {
                cancelledRequests.erase(reqIdStr);
                result["error"] = simpleJson("code", InvalidRequest, "message", "Request was cancelled");
                return picojson::value(result);
            }

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

        picojson::object resAnnotations, promptAnnotations;
        resAnnotations["subscribe"] = picojson::value(true);
        resAnnotations["listChanged"] = picojson::value(true);
        promptAnnotations["listChanged"] = picojson::value(true);

        picojson::object promptsProps, resourcesProp, toolsProp;
        picojson::object capabilities, serverInfo;
        resourcesProp["annotations"] = picojson::value(resAnnotations);
        if (!prompts.empty()) capabilities["prompts"] = picojson::value(promptAnnotations);
        capabilities["resources"] = picojson::value(resourcesProp);
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

    bool readResourceContent(const std::string& uri, picojson::array& contents,
                             const std::string& sessionID)
    {
        // Find static resource
        auto it = resources.find(uri);
        if (it != resources.end())
        {
            picojson::value content = it->second->read();
            contents.push_back(content); return true;
        }

        // Find template resource
        for (auto& tmpl : resourceTemplates)
        {
            std::map<std::string, std::string> params;
            if (!tmpl.second->match(uri, params)) continue;
            if (!tmpl.second->handler) continue;

            picojson::value content = tmpl.second->handler(params, sessionID);
            if (content.is<picojson::object>())
            {
                picojson::object& obj = content.get<picojson::object>();
                if (obj.find("uri") == obj.end()) obj["uri"] = picojson::value(uri);
            }
            contents.push_back(content); return true;
        }
        return false;
    }

    bool resourceExists(const std::string& uri) const
    {
        if (resources.find(uri) != resources.end()) return true;
        for (auto& tmpl : resourceTemplates)
        {
            std::map<std::string, std::string> params;
            if (tmpl.second->match(uri, params)) return true;
        }
        return false;
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
        EventAndData ed;
        {
            std::lock_guard<std::mutex> lock(msgMutex);
            std::queue<EventAndData>& origin = sseMessages[sessionID];
            if (origin.empty()) return; ed = origin.front(); origin.pop();
        }

        ctx->writer->SSEvent(ed.first, ed.second.c_str());
        if (ed.second != "heartbeat")
            OSG_NOTICE << "[JsonRpcServer] SSE-" << ed.second << ": " << ed.first << std::endl;
    }

    void sendNotification(const std::string& sessionID, const picojson::value& notification)
    { createEventMessage(sessionID, notification.serialize(false), "message"); }

    void broadcastResourceUpdated(const std::string& uri)
    {
        std::vector<std::string> tempResourceSubscriptions;
        {
            std::lock_guard<std::mutex> lock(sseMutex);
            for (std::map<std::string, std::set<std::string>>::iterator it = resourceSubscriptions.begin();
                 it != resourceSubscriptions.end(); ++it)
            {
                if (it->second.find(uri) != it->second.end())
                    tempResourceSubscriptions.push_back(it->first);
            }
        }

        picojson::object params, notify; params["uri"] = picojson::value(uri);
        notify["jsonrpc"] = picojson::value("2.0");
        notify["method"] = picojson::value("notifications/resources/updated");
        notify["params"] = picojson::value(params);
        for (auto& sessionID : tempResourceSubscriptions)
            createEventMessage(sessionID, picojson::value(notify).serialize(false), "message");
    }

    void broadcastResourceListChanged()
    {
        std::map<std::string, bool> tempSseInitialized;
        {
            std::lock_guard<std::mutex> lock(sseMutex);
            for (auto& it : sseInitialized)
            { if (it.second) tempSseInitialized[it.first] = it.second; }
        }

        picojson::object notify;
        notify["jsonrpc"] = picojson::value("2.0");
        notify["method"] = picojson::value("notifications/resources/list_changed");
        for (auto& session : tempSseInitialized)
            createEventMessage(session.first, picojson::value(notify).serialize(false), "message");
    }

    void broadcastPromptListChanged()
    {
        std::map<std::string, bool> tempSseInitialized;
        {
            std::lock_guard<std::mutex> lock(sseMutex);
            for (auto& it : sseInitialized)
            { if (it.second) tempSseInitialized[it.first] = it.second; }
        }

        picojson::object notify;
        notify["jsonrpc"] = picojson::value("2.0");
        notify["method"] = picojson::value("notifications/prompts/list_changed");
        for (auto& session : tempSseInitialized)
            createEventMessage(session.first, picojson::value(notify).serialize(false), "message");
    }

    void removeSessionMember(const std::string& sessionID)
    {
        auto cit = clientMap.find(sessionID);
        auto tit2 = sseMessages.find(sessionID);
        auto tit3 = sseInitialized.find(sessionID);
        auto subIt = resourceSubscriptions.find(sessionID);
        if (cit != clientMap.end()) clientMap.erase(cit);
        if (tit2 != sseMessages.end()) sseMessages.erase(tit2);
        if (tit3 != sseInitialized.end()) sseInitialized.erase(tit3);
        if (subIt != resourceSubscriptions.end()) resourceSubscriptions.erase(subIt);
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
        if (toRelease) { toRelease->join(); toRelease.release(); }
    }

    std::string encodeCursor(int offset)
    { return "cursor_" + std::to_string(offset); }

    int decodeCursor(const std::string& cursor)
    { if (cursor.substr(0, 7) == "cursor_") return std::stoi(cursor.substr(7)); return 0; }

    typedef std::pair<std::string, std::string> EventAndData;
    std::map<std::string, McpClientInfo> clientMap;
    std::map<std::string, std::unique_ptr<std::thread>> sseThreads;
    std::map<std::string, std::queue<EventAndData>> sseMessages;
    std::map<std::string, bool> sseInitialized;

    typedef std::pair<osg::ref_ptr<McpTool>, McpServer::MethodHandler> ToolData;
    typedef std::pair<osg::ref_ptr<McpPrompt>, McpServer::PromptHandler> PromptData;

    std::map<std::string, McpServer::MethodHandler> methods;
    std::map<std::string, McpServer::NotificationHandler> notifications;
    std::map<std::string, McpServer::SessionCleanupHandler> sessionCleanups;
    std::map<std::string, osg::ref_ptr<McpResource>> resources;
    std::map<std::string, osg::ref_ptr<McpResourceTemplate>> resourceTemplates;
    std::map<std::string, std::set<std::string>> resourceSubscriptions;
    std::map<std::string, PromptData> prompts;
    std::map<std::string, ToolData> tools;
    std::set<std::string> cancelledRequests;
    McpServer::AuthenticationHandler authentication;

    ThreadPool* threadPool;
    hv::HttpServer server;
    hv::HttpService service;
    std::string sseEndpoint, msgEndpoint;
    std::mutex clientMutex, sseMutex, msgMutex;
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

            std::string cursor = params.contains("cursor") ? params.get("cursor").get<std::string>() : "";
            return paginateResults(toolsData, cursor, 20, "tools");
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

void McpServer::registerRootsHandler(MethodHandler handler)
{
    JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
    std::lock_guard<std::mutex> lock(server->sseMutex);
    server->methods["roots/list"] = handler;
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
            std::string name = params.get("uri").get<std::string>();

            picojson::array contents; picojson::object result;
            if (server->readResourceContent(name, contents, id))
            {
                result["contents"] = picojson::value(contents);
                return picojson::value(result);
            }
            return simpleJson("code", InvalidParams, "message", "Resource " + name + " not found");
        };
    if (server->methods.find("resources/list") == server->methods.end())
        server->methods["resources/list"] = [this](const picojson::value& params, const std::string& id)
        {
            JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
            picojson::array allResources;
            for (auto& res : server->resources) allResources.push_back(res.second->metadata());
            for (auto& tmpl : server->resourceTemplates) allResources.push_back(tmpl.second->metadata());

            std::string cursor = params.contains("cursor") ? params.get("cursor").get<std::string>() : "";
            return paginateResults(allResources, cursor, 20, "resources");
        };
    if (server->methods.find("resources/subscribe") == server->methods.end())
        server->methods["resources/subscribe"] = [this](const picojson::value& params, const std::string& id)
        {
            JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
            if (!params.contains("uri")) return simpleJson("code", InvalidParams, "message", "Missing 'uri' parameter");
            std::string name = params.get("uri").get<std::string>();

            if (!server->resourceExists(name))
                return simpleJson("code", InvalidParams, "message", "Resource " + name + " not found");
            else
            {
                std::lock_guard<std::mutex> lock(server->sseMutex);
                server->resourceSubscriptions[id].insert(name);
            }
            picojson::object result; return picojson::value(result);
        };
    if (server->methods.find("resources/unsubscribe") == server->methods.end())
        server->methods["resources/unsubscribe"] = [this](const picojson::value& params, const std::string& id)
        {
            JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
            if (!params.contains("uri")) return simpleJson("code", InvalidParams, "message", "Missing 'uri' parameter");
            std::string name = params.get("uri").get<std::string>();
            {
                std::lock_guard<std::mutex> lock(server->sseMutex);
                auto it = server->resourceSubscriptions.find(id);
                if (it != server->resourceSubscriptions.end()) it->second.erase(name);
            }
            picojson::object result; return picojson::value(result);
        };
}

void McpServer::registerResourceTemplate(McpResourceTemplate* resourceTemplate)
{
    JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
    {
        std::lock_guard<std::mutex> lock(server->sseMutex);
        server->resourceTemplates[resourceTemplate->uriTemplate] = resourceTemplate;
    }
    server->broadcastResourceListChanged();
}

void McpServer::registerPrompt(McpPrompt* prompt, PromptHandler handler)
{
    JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
    {
        std::lock_guard<std::mutex> lock(server->sseMutex);
        server->prompts[prompt->name] = std::pair<osg::ref_ptr<McpPrompt>, PromptHandler>(prompt, handler);
    }

    if (server->methods.find("prompts/list") == server->methods.end())
        server->methods["prompts/list"] = [this](const picojson::value& params, const std::string& id)
        {
            JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
            picojson::array allPrompts;
            for (auto& p : server->prompts) allPrompts.push_back(p.second.first->metadata());

            std::string cursor = params.contains("cursor") ? params.get("cursor").get<std::string>() : "";
            return paginateResults(allPrompts, cursor, 20, "prompts");
        };
    if (server->methods.find("prompts/get") == server->methods.end())
        server->methods["prompts/get"] = [this](const picojson::value& params, const std::string& id)
        {
            JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
            if (!params.contains("name"))
                return simpleJson("code", InvalidParams, "message", "Missing 'name' parameter");

            std::string name = params.get("name").get<std::string>();
            auto it = server->prompts.find(name);
            if (it == server->prompts.end())
                return simpleJson("code", InvalidParams, "message", "Prompt " + name + " not found");

            picojson::value args = params.contains("arguments") ? params.get("arguments") : picojson::value();
            std::vector<McpPromptMessage> messages = it->second.second(args, id);
            picojson::array msgArray; for (auto& msg : messages) msgArray.push_back(msg.json());

            picojson::object result;
            result["description"] = picojson::value(it->second.first->description);
            result["messages"] = picojson::value(msgArray);
            return picojson::value(result);
        };
    server->broadcastPromptListChanged();
}

void McpServer::registerSamplingHandler(SamplingHandler handler)
{
    JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
    std::lock_guard<std::mutex> lock(server->sseMutex);
    if (server->methods.find("sampling/createMessage") == server->methods.end())
        server->methods["sampling/createMessage"] = [this, handler](const picojson::value& params, const std::string& id)
        { return handler(params, id); };  // client should handle it...
}

void McpServer::notifyPromptListChanged()
{
    JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
    server->broadcastPromptListChanged();
}

void McpServer::notifyResourceUpdated(const std::string& uri)
{
    JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
    server->broadcastResourceUpdated(uri);
}

void McpServer::notifyResourceListChanged()
{
    JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
    server->broadcastResourceListChanged();
}

void McpServer::subscribeResource(const std::string& sessionID, const std::string& uri)
{
    JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
    {
        std::lock_guard<std::mutex> lock(server->sseMutex);
        server->resourceSubscriptions[sessionID].insert(uri);
    }
}

void McpServer::unsubscribeResource(const std::string& sessionID, const std::string& uri)
{
    JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
    {
        std::lock_guard<std::mutex> lock(server->sseMutex);
        auto it = server->resourceSubscriptions.find(sessionID);
        if (it != server->resourceSubscriptions.end()) it->second.erase(uri);
    }
}

bool McpServer::isResourceSubscribed(const std::string& sessionID, const std::string& uri) const
{
    const JsonRpcServer* server = static_cast<const JsonRpcServer*>(_core.get());
    {
        JsonRpcServer* nonconst = const_cast<JsonRpcServer*>(server);
        std::lock_guard<std::mutex> lock(nonconst->sseMutex);

        auto it = server->resourceSubscriptions.find(sessionID);
        if (it != server->resourceSubscriptions.end())
            return it->second.find(uri) != it->second.end();
    }
    return false;
}

picojson::value McpServer::paginateResults(const picojson::array& allItems, const std::string& cursor,
                                           int pageSize, const std::string& listType)
{
    JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
    int offset = server->decodeCursor(cursor), total = allItems.size();

    picojson::array pageItems; int end = std::min(offset + pageSize, total);
    for (int i = offset; i < end; ++i) pageItems.push_back(allItems[i]);

    picojson::object result; result[listType] = picojson::value(pageItems);
    if (end < total) result["nextCursor"] = picojson::value(server->encodeCursor(end));
    else result["nextCursor"] = picojson::value("");
    return picojson::value(result);
}

void McpServer::sendProgress(const std::string& sessionID, const std::string& token,
                             double progress, double total, const std::string& message)
{
    JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
    picojson::value notify = progressNotification(token, progress, total, message);
    server->sendNotification(sessionID, notify);
}

void McpServer::sendCancelled(const std::string& sessionID, const std::string& requestId,
                              const std::string& reason)
{
    JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
    picojson::value notify = cancelledNotification(requestId, reason);
    server->sendNotification(sessionID, notify);
}
