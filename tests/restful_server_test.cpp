#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <osg/LightSource>
#include <osg/Texture2D>
#include <osg/ShapeDrawable>
#include <osg/MatrixTransform>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgUtil/CullVisitor>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <pipeline/SkyBox.h>
#include <pipeline/Pipeline.h>
#include <pipeline/Utilities.h>
#include <iostream>
#include <sstream>

#include <libhv/all/server/HttpService.h>
#include <libhv/all/server/HttpServer.h>
#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

class Handler
{
public:
    static int preprocessor(HttpRequest* req, HttpResponse* resp)
    {
        req->ParseBody();
        resp->content_type = APPLICATION_JSON;
        return HTTP_STATUS_UNFINISHED;
    }

    static int postprocessor(HttpRequest* req, HttpResponse* resp)
    {
        OSG_NOTICE << resp->Dump(true, true) << std::endl;
        return resp->status_code;
    }

    // curl -v http://127.0.0.1:2520/camera/matrix?type=view
    static int get_matrix(HttpRequest* req, HttpResponse* resp)
    {
        osg::Matrix m;
        for (auto& param : req->query_params)
        {
            if (param.first == "type")
            {
                if (param.second == "view")
                    m = viewer.getCamera()->getViewMatrix();
                else if (param.second == "projection")
                    m = viewer.getCamera()->getProjectionMatrix();
                else if (param.second == "window")
                {
                    if (viewer.getCamera()->getViewport())
                        m = viewer.getCamera()->getViewport()->computeWindowMatrix();
                }
            }
        }

        std::stringstream ss; ss << *m.ptr();
        for (int i = 1; i < 16; ++i) ss << " " << *(m.ptr() + i);
        resp->content_type = APPLICATION_JSON;
        resp->json["type"] = "dmat4";
        resp->json["value"] = ss.str();
        return 200;
    }

    // curl -v http://127.0.0.1:2520/scene/root -H "Content-Type:application/json"
    //      -d "{\"name\":\"cow\", \"file\":\"cow.osg\", \"position\":\"0 0 5\"}"
    static int add_child(const HttpContextPtr& ctx)
    {
        if (ctx->request->content_type != APPLICATION_JSON)
            return response_status(ctx, HTTP_STATUS_BAD_REQUEST);

        std::string name = ctx->get("name"), file = ctx->get("file");
        std::string position = ctx->get("position");
        std::string npath = ctx->param("npath"), result = "failed";

        osg::Node* parent = get_node_from_path(npath);
        if (parent && parent->asGroup())
        {
            osg::Node* child = osgDB::readNodeFile(file);
            if (child)
            {
                osg::MatrixTransform* mt = new osg::MatrixTransform;
                mt->addChild(child); mt->setName(name);
                parent->asGroup()->addChild(mt); result = "added";

                osgDB::StringList pValues;
                osgDB::split(position, pValues, ' ');
                if (pValues.size() == 3)
                    mt->setMatrix(osg::Matrix::translate(
                        atof(pValues[0].c_str()), atof(pValues[1].c_str()), atof(pValues[2].c_str())));
            }
        }

        HttpResponse* resp = ctx->response.get();
        resp->content_type = APPLICATION_JSON;
        resp->json["path"] = npath + "." + name;
        resp->json["result"] = result;
        return response_status(ctx, 200, "OK");
    }

    // curl -v -X DELETE http://127.0.0.1:2520/scene/root.cessna
    static int remove_child(const HttpContextPtr& ctx)
    {
        std::string npath = ctx->param("npath"), result = "deleted";
        osg::Node* node = get_node_from_path(npath);
        if (node && node->getNumParents() > 0)
            node->getParent(0)->removeChild(node);
        else
            result = "failed";

        HttpResponse* resp = ctx->response.get();
        resp->content_type = APPLICATION_JSON;
        resp->json["path"] = npath;
        resp->json["result"] = result;
        return response_status(ctx, 200, "OK");
    }

    static osg::ref_ptr<osg::Group> root;
    static osgViewer::Viewer viewer;

protected:
    static int response_status(const HttpContextPtr& ctx, int code = 200, const char* message = NULL)
    {
        HttpResponse* resp = ctx->response.get();
        if (message == NULL) message = http_status_str((enum http_status)code);
        resp->Set("code", code); resp->Set("message", message);
        ctx->send(); return code;
    }

    static osg::Node* get_node_from_path(const std::string& npath)
    {
        osgDB::StringList nodePath;
        osgDB::split(npath, nodePath, '.');

        osg::Node* current = root.get();
        for (size_t i = 1; i < nodePath.size(); ++i)
        {
            osg::Group* group = current->asGroup();
            if (!group) return NULL;

            for (size_t j = 0; j < group->getNumChildren(); ++j)
            {
                if (group->getChild(j)->getName() == nodePath[i])
                { current = group->getChild(j); break; }
            }
        }
        return current;
    }
};

osg::ref_ptr<osg::Group> Handler::root;
osgViewer::Viewer Handler::viewer;

int main(int argc, char** argv)
{
    hv::HttpServer server;
    server.worker_processes = 0;
    server.worker_threads = 0;
    server.port = 2520;

    hv::HttpService service;
    service.preprocessor = Handler::preprocessor;
    service.postprocessor = Handler::postprocessor;
    service.GET("/camera/matrix", Handler::get_matrix);
    service.POST("/scene/:npath", Handler::add_child);
    service.Delete("/scene/:npath", Handler::remove_child);
    server.registerHttpService(&service);
    server.start();

    // Scene root
    osg::Node* node = osgDB::readNodeFile("cessna.osg");
    if (node) node->setName("cessna");

    Handler::root = new osg::Group;
    Handler::root->addChild(node);
    Handler::root->setName("root");

    Handler::viewer;
    Handler::viewer.addEventHandler(new osgViewer::StatsHandler);
    Handler::viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    Handler::viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    Handler::viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    Handler::viewer.setSceneData(Handler::root.get());
    return Handler::viewer.run();
}
