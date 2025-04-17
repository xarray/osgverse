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

    static int handleJsonRpc(const HttpContextPtr& ctx)
    {
        std::string sessionID = ctx->request->GetParam("session_id");
        if (ctx->request->Method() == "OPTIONS")
            return responseStatus(ctx, 204, "", true);

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

        JsonRpcServer* rpc = (JsonRpcServer*)ctx->userdata;
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
                    std::stringstream ss; picojson::value result = rpc->handleRequest(sessionID, request);
                    ss << "event: message\r\ndata: " << result.serialize(false) << "\r\n\r\n";
                    rpc->createEventMessage(sessionID, ss.str());
                });
            return responseStatus(ctx, 202, "Accepted", true);
        }
    }

    static int handleSSE(const HttpContextPtr& ctx)
    {
        JsonRpcServer* rpc = (JsonRpcServer*)ctx->userdata;
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
                    std::stringstream ss;
                    ss << "event: endpoint\r\ndata: " << rpc->msgEndpoint
                       << "?session_id=" << sessionID << "\r\n\r\n";
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    rpc->createEventMessage(sessionID, ss.str());

                    int heartbeatCount = 0;
                    while (rpc->running && ctx->writer->isConnected())
                    {
                        std::this_thread::sleep_for(std::chrono::seconds(5) + std::chrono::milliseconds(rand() % 500));
                        if (!rpc->running || !ctx->writer->isConnected()) break;

                        std::stringstream heartbeat;
                        heartbeat << "event: heartbeat\r\ndata: " << heartbeatCount++ << "\r\n\r\n";
                        rpc->createEventMessage(sessionID, heartbeat.str());
                    }
                    rpc->closeSession(sessionID);
                });
            {
                std::lock_guard<std::mutex> lock(rpc->sseMutex);
                rpc->sseThreads[sessionID] = std::move(thread);
            }

            hv::setInterval(200, [rpc, ctx, sessionID](hv::TimerID timerID)
                {
                    if (ctx->writer->isConnected())
                        rpc->sendEventMessage(sessionID, ctx);
                    else
                        hv::killTimer(timerID);
                });
            return HTTP_STATUS_UNFINISHED;
        }
        return responseStatus(ctx, 202, "Accepted", true);
    }

    picojson::value handleRequest(const std::string& sessionID,
                                  const McpRequest& req, bool asNotify = false)
    {
        picojson::value result;
        if (asNotify)
        {
            if (req.method == "notifications/initialized")
            {
                std::lock_guard<std::mutex> lock(sseMutex);
                sseInitialized[sessionID] = true;
            }
            return result;
        }

        McpServer::MethodHandler handler = NULL;
        bool initialized = false;
        if (req.method == "initialize")
            result = handInitialization(sessionID, req);
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

        if (!initialized)
            result = simpleJson("id", req.id, "error", "Session not initialized");
        else if (handler == NULL)
            result = simpleJson("id", req.id, "error", "Method " + req.method + " not found");
        else
            result = handler(req.params, sessionID);
        return result;
    }

    picojson::value handInitialization(const std::string& sessionID, const McpRequest& req)
    {
        // TODO

        picojson::object infoData;
        //{"protocolVersion", MCP_VERSION}
        //{"capabilities", capabilities}
        //{"serverInfo", server_info}

        picojson::object result = simpleJson("id", req.id, "", "").get<picojson::object>();
        result["result"] = picojson::value(infoData); return picojson::value(result);
    }

public:
    JsonRpcServer(const std::string& host, int port,
                  const std::string& sse_endpoint,
                  const std::string& msg_endpoint) : running(true)
    {
        memcpy(server.host, host.c_str(), osg::minimum((int)host.length(), 64));
        server.worker_processes = 0; server.worker_threads = 0;
        server.port = port; server.userdata = this;
        sseEndpoint = sse_endpoint; msgEndpoint = msg_endpoint;

        service.AllowCORS();
        service.preprocessor = JsonRpcServer::preprocessor;
        service.postprocessor = JsonRpcServer::postprocessor;
        service.POST(msg_endpoint.c_str(), handleJsonRpc);
        service.GET(sse_endpoint.c_str(), handleSSE);

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
        {
            std::lock_guard<std::mutex> lock(sseMutex);
            while (!sseThreads.empty()) { closeSession(sseThreads.begin()->first); }
            sseThreads.clear(); sseMessages.clear(); sseInitialized.clear();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        running = false; microSleep(20000);
        return OpenThreads::Thread::cancel();
    }

    void createEventMessage(const std::string& sessionID, const std::string& msg)
    {
        std::lock_guard<std::mutex> lock(msgMutex);
        sseMessages[sessionID].push(msg);
    }

    void sendEventMessage(const std::string& sessionID, const HttpContextPtr& ctx)
    {
        std::queue<std::string> messages;
        {
            std::lock_guard<std::mutex> lock(msgMutex);
            std::queue<std::string>& origin = sseMessages[sessionID];
            if (origin.empty()) return; messages = origin; origin.pop();
        }
        ctx->writer->WriteBody(messages.front());
    }

    void closeSession(const std::string& sessionID)
    {
        for (std::map<std::string, McpServer::SessionCleanupHandler>::iterator it = sessionCleanups.begin();
             it != sessionCleanups.end(); ++it) it->second(it->first, sessionID);

        std::unique_ptr<std::thread> toRelease;
        {
            std::lock_guard<std::mutex> lock(sseMutex);
            auto tit = sseThreads.find(sessionID);
            if (tit != sseThreads.end())
            {
                toRelease = std::move(tit->second);
                sseThreads.erase(tit);
            }

            auto tit2 = sseMessages.find(sessionID);
            auto tit3 = sseInitialized.find(sessionID);
            if (tit2 != sseMessages.end()) sseMessages.erase(tit2);
            if (tit3 != sseInitialized.end()) sseInitialized.erase(tit3);
        }
        if (toRelease) toRelease.release();
    }

    std::map<std::string, std::unique_ptr<std::thread>> sseThreads;
    std::map<std::string, std::queue<std::string>> sseMessages;
    std::map<std::string, bool> sseInitialized;

    std::map<std::string, McpServer::MethodHandler> methods;
    std::map<std::string, McpServer::NotificationHandler> notifications;
    std::map<std::string, McpServer::SessionCleanupHandler> sessionCleanups;
    std::map<std::string, osg::ref_ptr<McpResource>> resources;
    std::map<std::string, std::pair<osg::ref_ptr<McpTool>, McpServer::MethodHandler>> tools;
    McpServer::AuthenticationHandler authentication;

    ThreadPool* threadPool;
    hv::HttpServer server;
    hv::HttpService service;
    std::string sseEndpoint, msgEndpoint;
    std::mutex sseMutex, msgMutex;
    bool running;
};

McpServer::McpServer(const std::string& sse_endpoint, const std::string& msg_endpoint)
{ _sseRoute = sse_endpoint; _jsonRoute = msg_endpoint; }

McpServer::~McpServer()
{
    JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
    server->cancel();
}

void McpServer::start(const std::string& host, int port)
{
    JsonRpcServer* server = new JsonRpcServer(
        host, port, _sseRoute, _jsonRoute);
    server->start(); _core = server;
}

void McpServer::stop()
{
    JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
    server->cancel(); server->join(); _core = NULL;
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

void McpServer::registerTool(const std::string& name, McpTool* tool, MethodHandler handler)
{
    JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
    std::lock_guard<std::mutex> lock(server->sseMutex);
    server->tools[name] = std::pair<osg::ref_ptr<McpTool>, MethodHandler>(tool, handler);

    if (server->methods.find("tools/list") == server->methods.end())
        server->methods["tools/list"] = [&](const picojson::value& params, const std::string& id) -> picojson::value
        {
            // TODO: list tools
            return picojson::value();
        };
    if (server->methods.find("tools/call") == server->methods.end())
        server->methods["tools/call"] = [&](const picojson::value& params, const std::string& id) -> picojson::value
        {
            if (!params.contains("name")) return simpleJson("id", -1, "error", "Missing 'name' parameter");
            std::string name = params.get("name").get<std::string>(); auto it = server->tools.find(name);
            if (it == server->tools.end()) return simpleJson("id", -1, "error", "Tool " + name + " not found");

            picojson::object result;
            try
            {
                picojson::value args = params.contains("arguments") ? params.get("arguments") : picojson::value();
                result["content"] = it->second.second(args, id); result["isError"] = picojson::value(false);
            }
            catch (const std::exception& e)
            {
                result["content"] = picojson::value(e.what());
                result["isError"] = picojson::value(true);
            }
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
        server->methods["resources/read"] = [&](const picojson::value& params, const std::string& id) -> picojson::value
        {
            if (!params.contains("uri")) return simpleJson("id", -1, "error", "Missing 'uri' parameter");
            std::string name = params.get("uri").get<std::string>(); auto it = server->resources.find(name);
            if (it == server->resources.end()) return simpleJson("id", -1, "error", "Resource " + name + " not found");

            // TODO: read from resource
            return picojson::value();
        };
    if (server->methods.find("resources/list") == server->methods.end())
        server->methods["resources/list"] = [&](const picojson::value& params, const std::string& id) -> picojson::value
        {
            // TODO: list resources
            return picojson::value();
        };
    if (server->methods.find("resources/subscribe") == server->methods.end())
        server->methods["resources/subscribe"] = [&](const picojson::value& params, const std::string& id) -> picojson::value
        {
            // TODO: subscribe resources
            return picojson::value();
        };
}
