#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/BindImageTexture>
#include <osg/DispatchCompute>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>

#include <modeling/Math.h>
#include <readerwriter/DatabasePager.h>
#include <pipeline/IncrementalCompiler.h>
#include <VerseCommon.h>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

static const char* resizeSrc = {
    "#version 430 core \n"
    "layout (local_size_x = 16, local_size_y = 16) in; \n"
    "layout (binding = 0) uniform sampler2D sourceTexture; \n"
    "layout (binding = 1, rgba8) writeonly uniform image2D targetTexture; \n"
    "void main() { \n"
    "    ivec2 targetCoord = ivec2(gl_GlobalInvocationID.xy); \n"
    "    vec2 sourceCoord = vec2(targetCoord) * 2.0 / vec2(textureSize(sourceTexture, 0)); \n"
    "    vec4 color = texture(sourceTexture, sourceCoord); \n"
    "    imageStore(targetTexture, targetCoord, color.bbba); \n"
    "} \n"
};

class UserIncrementalCompileCallback : public osgVerse::IncrementalCompileCallback
{
public:
    UserIncrementalCompileCallback() : osgVerse::IncrementalCompileCallback()
    {
        osg::ref_ptr<osg::Program> resizeProgram = new osg::Program;
        resizeProgram->addShader(new osg::Shader(osg::Shader::COMPUTE, resizeSrc));

        _compute = new osg::DispatchCompute;
        _compute->setDataVariance(osg::Object::DYNAMIC);
        _compute->getOrCreateStateSet()->setAttributeAndModes(resizeProgram.get());
        _compute->getOrCreateStateSet()->addUniform(new osg::Uniform("sourceTexture", (int)0));
        _compute->getOrCreateStateSet()->addUniform(new osg::Uniform("targetTexture", (int)1));
    }

    virtual bool compile(ICO::CompileTextureOp* op, ICO::CompileInfo& compileInfo)
    {
#if false
        osg::State* state = compileInfo.getState();
        if (glCopyImageSubData == NULL) osg::setGLExtensionFuncPtr(
            glCopyImageSubData, "glCopyImageSubData", "glCopyImageSubDataEXT", "glCopyImageSubDataNV", true);

        osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
        if (op->_texture->getImage(0))
        {
            osg::Image* img = op->_texture->getImage(0);
            tex2D->setTextureSize(img->s() / 2, img->t() / 2);
        }
        else
            tex2D->setTextureSize(op->_texture->getTextureWidth() / 2, op->_texture->getTextureHeight() / 2);
        tex2D->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        tex2D->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        tex2D->setInternalFormat(GL_RGBA8); tex2D->setSourceFormat(GL_RGBA);
        tex2D->setSourceType(GL_UNSIGNED_BYTE);
        if (tex2D->getTextureWidth() == 0 || tex2D->getTextureHeight() == 0)
            return osgVerse::IncrementalCompileCallback::compile(op, compileInfo);

        osg::ref_ptr<osg::Texture::TextureObject> tobj =
            osg::Texture::generateTextureObject(tex2D.get(), state->getContextID(), GL_TEXTURE_2D);
        tex2D->setTextureObject(state->getContextID(), tobj.get());

        osg::ref_ptr<osg::BindImageTexture> binding = new osg::BindImageTexture(
            1, tex2D.get(), osg::BindImageTexture::WRITE_ONLY, GL_RGBA8);
        state->apply(_compute->getStateSet());
        state->applyTextureMode(0, op->_texture->getTextureTarget(), true);
        state->applyTextureAttribute(0, op->_texture.get());
        state->applyTextureMode(1, op->_texture->getTextureTarget(), true);
        state->applyTextureAttribute(1, tex2D.get());
        state->applyAttribute(binding.get());
        _compute->setComputeGroups(
            (tex2D->getTextureWidth() + 15) / 16, (tex2D->getTextureHeight() + 15) / 16, 1);
        _compute->draw(compileInfo); return true;
#else
        return osgVerse::IncrementalCompileCallback::compile(op, compileInfo);
#endif
    }

protected:
    osg::ref_ptr<osg::DispatchCompute> _compute;
};

class MyDatabasePager : public osgVerse::DatabasePager
{
    class SetBasedPagedLODList : public osgDB::DatabasePager::PagedLODList
    {
    public:
        typedef std::set<osg::observer_ptr<osg::PagedLOD>> PagedLODs;
        PagedLODs _pagedLODs;
    };

    class ExpirePagedLODsVisitor : public osg::NodeVisitor
    {
    public:
        ExpirePagedLODsVisitor() : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}
        META_NodeVisitor("osgDB", "ExpirePagedLODsVisitor")

        virtual void apply(osg::PagedLOD& plod)
        {
            _childPagedLODs.insert(&plod);
            markRequestsExpired(&plod); traverse(plod);
        }

        bool removeExpiredChildrenAndFindPagedLODs(osg::PagedLOD* plod, double expiryTime,
                                                   unsigned int expiryFrame, osg::NodeList& removedChildren)
        {
            size_t sizeBefore = removedChildren.size();
            plod->removeExpiredChildren(expiryTime, expiryFrame, removedChildren);
            for (size_t i = sizeBefore; i < removedChildren.size(); ++i) removedChildren[i]->accept(*this);
            return sizeBefore != removedChildren.size();
        }

        typedef std::set<osg::ref_ptr<osg::PagedLOD> > PagedLODset;
        PagedLODset _childPagedLODs;

    private:
        void markRequestsExpired(osg::PagedLOD* plod)
        {
            unsigned numFiles = plod->getNumFileNames();
            for (unsigned i = 0; i < numFiles; ++i)
            {
                DatabasePager::DatabaseRequest* req = dynamic_cast<DatabasePager::DatabaseRequest*>(plod->getDatabaseRequest(i).get());
                if (req) req->_groupExpired = true;
            }
        }
    };

public:
    MyDatabasePager() : osgVerse::DatabasePager()
    {
        setTargetMaximumNumberOfPageLOD(50);
    }

    virtual void removeExpiredSubgraphs(const osg::FrameStamp& fs)
    {
        unsigned int numPagedLODs = _activePagedLODList->size();
        std::cout << "removeExpiredSubgraphs " << numPagedLODs << "...\n";
        //osgVerse::DatabasePager::removeExpiredSubgraphs(fs);

        double expiryTime = fs.getReferenceTime() - 0.1;
        unsigned int expiryFrame = fs.getFrameNumber() - 1;
        int numToPrune = numPagedLODs - _targetMaximumNumberOfPageLOD;
        if (numPagedLODs <= _targetMaximumNumberOfPageLOD) return;

        SetBasedPagedLODList* activeLODList = static_cast<SetBasedPagedLODList*>(_activePagedLODList.get());
        ObjectList childrenRemoved;
        if (numToPrune > 0) removeExpiredChildren(activeLODList,
            numToPrune, expiryTime, expiryFrame, childrenRemoved, false);

        numToPrune = _activePagedLODList->size() - _targetMaximumNumberOfPageLOD;
        if (numToPrune > 0) removeExpiredChildren(activeLODList,
            numToPrune, expiryTime, expiryFrame, childrenRemoved, true);

        if (!childrenRemoved.empty())
        {
            // pass the objects across to the database pager delete list
            if (_deleteRemovedSubgraphsInDatabaseThread)
            {
                OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_fileRequestQueue->_requestMutex);
                _fileRequestQueue->_childrenToDeleteList.splice(
                    _fileRequestQueue->_childrenToDeleteList.end(), childrenRemoved);
                _fileRequestQueue->updateBlock();
            }
            else
                childrenRemoved.clear();
        }
    }

    void removeExpiredChildren(
        SetBasedPagedLODList* activePagedLODList,
        int numberChildrenToRemove, double expiryTime, unsigned int expiryFrame,
        osgDB::DatabasePager::ObjectList& childrenRemoved, bool visitActive)
    {
        int leftToRemove = numberChildrenToRemove;
        for (SetBasedPagedLODList::PagedLODs::iterator itr = activePagedLODList->_pagedLODs.begin();
             itr != activePagedLODList->_pagedLODs.end() && leftToRemove > 0;)
        {
            osg::ref_ptr<osg::PagedLOD> plod;
            if (itr->lock(plod))
            {
                bool plodActive = expiryFrame < plod->getFrameNumberOfLastTraversal();
                if (visitActive == plodActive) // true if (visitActive && plodActive) OR (!visitActive &&!plodActive)
                {
                    ExpirePagedLODsVisitor expirePLV; osg::NodeList expiredChildren;
                    expirePLV.removeExpiredChildrenAndFindPagedLODs(
                        plod.get(), expiryTime, expiryFrame, expiredChildren);

                    // Clear any expired PagedLODs out of the set
                    for (ExpirePagedLODsVisitor::PagedLODset::iterator
                         citr = expirePLV._childPagedLODs.begin(), end = expirePLV._childPagedLODs.end(); citr != end; ++citr)
                    {
                        osg::observer_ptr<osg::PagedLOD> clod(*citr);
                        // This child PagedLOD cannot be equal to the
                        // PagedLOD pointed to by itr because it must be
                        // in itr's subgraph. Therefore erasing it doesn't
                        // invalidate itr.
                        if (activePagedLODList->_pagedLODs.erase(clod) > 0) leftToRemove--;
                    }
                    std::copy(expiredChildren.begin(), expiredChildren.end(), std::back_inserter(childrenRemoved));
                }

                // advance the iterator to the next element
                ++itr;
            }
            else
            {
                activePagedLODList->_pagedLODs.erase(itr++); leftToRemove--;
                OSG_NOTICE << "DatabasePager::removeExpiredSubgraphs() _inactivePagedLOD has been invalidated, but ignored" << std::endl;
            }
        }
    }
};

static std::string replace(std::string& src, const std::string& match, const std::string& v, bool& c)
{
    size_t levelPos = src.find(match); if (levelPos == std::string::npos) { c = false; return src; }
    src.replace(levelPos, match.length(), v); c = true; return src;
}

static std::string createCustomPath(const std::string& prefix, int x, int y, int z)
{
    std::string path = prefix; bool changed = false;
    //path = replace(path, "{x16}", std::to_string(x / 16), changed);
    //path = replace(path, "{y16}", std::to_string(y / 16), changed);
    //y = (int)pow((double)z, 2.0) - y - 1;
    path = replace(path, "{z}", std::to_string(z), changed);
    path = replace(path, "{x}", std::to_string(x), changed);
    path = replace(path, "{y}", std::to_string(y), changed); return path;
}

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osgVerse::updateOsgBinaryWrappers();

    osg::ref_ptr<osgDB::Options> earthOptions = new osgDB::Options(
        "URL=https://webst01.is.autonavi.com/appmaptile?style%3d6&x%3d{x}&y%3d{y}&z%3d{z} UseWebMercator=1 UseEarth3D=1");
        //"URL=https://mt1.google.com/vt/lyrs%3ds&x%3d{x}&y%3d{y}&z%3d{z} UseWebMercator=1 UseEarth3D=1");
        //"URL=http://p0.map.gtimg.com/sateTiles/{z}/{x16}/{y16}/{x}_{y}.jpg UseWebMercator=1 UseEarth3D=1");
    earthOptions->setPluginData("UrlPathFunction", (void*)createCustomPath);

    osg::ref_ptr<osg::Node> earth = osgDB::readNodeFile("0-0-0.verse_tms", earthOptions.get());
    osg::ref_ptr<osg::Node> tiles = osgDB::readNodeFiles(arguments, new osgDB::Options("DisabledPBR=1"));
    if (!earth || !tiles) return 1;

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    root->addChild(earth.get());
    
    osg::Vec3 dir = tiles->getBound().center(); dir.normalize();
    osg::ref_ptr<osg::MatrixTransform> tileMT = new osg::MatrixTransform;
    //tileMT->setMatrix(osg::Matrix::translate(-dir * 380.0f));
    tileMT->addChild(tiles.get());

    osg::ref_ptr<osg::EllipsoidModel> em = new osg::EllipsoidModel;
    osg::Vec3d ecef = tiles->getBound().center(), newEcef, lla;
    em->convertXYZToLatLongHeight(ecef[0], ecef[1], ecef[2], lla[0], lla[1], lla[2]);

    osg::Vec3d newLLA = osgVerse::Coordinate::convertWGS84toGCJ02(lla);
    std::cout << "WGS84: " << osg::RadiansToDegrees(lla[1]) << ", " << osg::RadiansToDegrees(lla[0]) << "; "
              << "GCJ02: " << osg::RadiansToDegrees(newLLA[1]) << ", " << osg::RadiansToDegrees(newLLA[0]) << std::endl;
    em->convertLatLongHeightToXYZ(newLLA[0], newLLA[1], newLLA[2], newEcef[0], newEcef[1], newEcef[2]);

    osg::ref_ptr<osg::MatrixTransform> tileOffset = new osg::MatrixTransform;
    tileOffset->setMatrix(osg::Matrix::translate(newEcef - ecef));
    tileOffset->addChild(tileMT.get());
    root->addChild(tileOffset.get());

    osg::BoundingSphere bs = tiles->getBound(); double r = bs.radius() * 10.0;
    osg::ref_ptr<osgGA::TrackballManipulator> trackball = new osgGA::TrackballManipulator;
    trackball->setHomePosition(bs.center() + osg::Z_AXIS * r, bs.center(), osg::Y_AXIS);

    osg::ref_ptr<osgVerse::IncrementalCompiler> incrementalCompiler = new osgVerse::IncrementalCompiler;
    incrementalCompiler->setCompileCallback(new UserIncrementalCompileCallback);

    osgViewer::Viewer viewer;
    viewer.getCamera()->setNearFarRatio(0.00001);
    viewer.setDatabasePager(new MyDatabasePager);
    //viewer.setIncrementalCompileOperation(incrementalCompiler.get());
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(trackball.get());
    viewer.setSceneData(root.get());
    return viewer.run();
}
