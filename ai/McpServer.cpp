#include <hevent.h>
#include <hloop.h>
#include <hbase.h>
#include <hsocket.h>
#include <OpenThreads/Thread>
#include <osg/Notify>
#include "McpServer.h"
using namespace osgVerse;

class JsonRpcServer : public osg::Referenced, public OpenThreads::Thread
{
protected:
    virtual ~JsonRpcServer()
    {
        hloop_free(&loop);
    }

public:
    JsonRpcServer(const std::string& host, int port) : running(true)
    {
        memset(&settings, 0, sizeof(unpack_setting_t));
        settings.mode = UNPACK_BY_DELIMITER;
        settings.package_max_length = DEFAULT_PACKAGE_MAX_LENGTH;
        settings.delimiter[0] = '\0';
        settings.delimiter_bytes = 1;
        loop = hloop_new(0);
        hloop_set_userdata(loop, this);
        listener = hloop_create_tcp_server(loop, host.c_str(), port, JsonRpcServer::onAccept);
    }

    static void onAccept(hio_t* io)
    {
        OSG_NOTICE << "[McpServer] Server open." << std::endl;
        JsonRpcServer* js = (JsonRpcServer*)hloop_userdata(io->loop);

        hio_setcb_close(io, onClose);
        hio_setcb_read(io, onReceive);
        hio_set_unpack(io, &(js->settings));
        hio_read(io);
    }

    static void onClose(hio_t* io)
    {
        OSG_NOTICE << "[McpServer] Server closed. Error = " << hio_error(io) << std::endl;
    }

    static void onReceive(hio_t* io, void* readbuf, int readbytes)
    {
#if true
        char localaddrstr[SOCKADDR_STRLEN] = { 0 };
        char peeraddrstr[SOCKADDR_STRLEN] = { 0 };
        OSG_NOTICE << "[McpServer] Server connected: Local = "
                   << SOCKADDR_STR(hio_localaddr(io), localaddrstr) << ", Peer = "
                   << SOCKADDR_STR(hio_peeraddr(io), peeraddrstr) << std::endl;
#endif

        // JSON_Parse -> router -> JSON_Print -> hio_write
        std::string inputData((char*)readbuf, (char*)readbuf + readbytes);
        std::cout << "{{{" << inputData << "}}}" << std::endl;

        std::string responseData("{\"status\": -1}");
        hio_write(io, responseData.c_str(), responseData.length());
    }

    virtual void run()
    {
        while (running)
        {
            hloop_run(loop);
            microSleep(15000);
        }
    }

    virtual int cancel()
    {
        running = false; microSleep(20000);
        return OpenThreads::Thread::cancel();
    }

    hloop_t* loop;
    hio_t* listener;
    unpack_setting_t settings;
    bool running;
};

McpServer::McpServer(const std::string& host, int port)
{
    JsonRpcServer* server = new JsonRpcServer(host, port);
    server->start(); _core = server;
}

McpServer::~McpServer()
{
    JsonRpcServer* server = static_cast<JsonRpcServer*>(_core.get());
    server->cancel();
}
