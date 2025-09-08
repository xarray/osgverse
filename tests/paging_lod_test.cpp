#include <osg/io_utils>
#include <osg/ValueObject>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/Archive>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <ghc/filesystem.hpp>
#include <nanoid/nanoid.h>

#include <VerseCommon.h>
#include <readerwriter/DatabasePager.h>
#include <readerwriter/OsgbTileOptimizer.h>
#include <readerwriter/DracoProcessor.h>

#ifdef OSG_LIBRARY_STATIC
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
USE_SERIALIZER_WRAPPER(DracoGeometry)
#endif

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

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

class GeomCompressor : public osg::NodeVisitor
{
public:
    virtual void apply(osg::PagedLOD& node)
    {
        //std::cout << "DB Path: " << node.getDatabasePath() << " => empty\n";
        node.setDatabasePath("");  traverse(node);
    }

    virtual void apply(osg::Geode& geode)
    {
        for (unsigned int i = 0; i < geode.getNumDrawables(); ++i)
        {
            osg::Geometry* geom = geode.getDrawable(i)->asGeometry();
            if (!geom) continue;

            osg::ref_ptr<osgVerse::DracoGeometry> geom2 = new osgVerse::DracoGeometry(*geom);
            geode.replaceDrawable(geom, geom2.get());
        }
        traverse(geode);
    }
};

class TextureChecker : public osg::NodeVisitor
{
public:
    virtual void apply(osg::PagedLOD& node)
    {
        if (node.getStateSet()) apply(&node, NULL, *node.getStateSet());
        node.setDatabasePath(""); traverse(node);
    }

    virtual void apply(osg::Geode& geode)
    {
        if (geode.getStateSet()) apply(&geode, NULL, *geode.getStateSet());
        for (unsigned int i = 0; i < geode.getNumDrawables(); ++i)
        {
            osg::Geometry* geom = geode.getDrawable(i)->asGeometry();
            if (geom && geom->getStateSet()) apply(&geode, geom, *geom->getStateSet());
        }
        traverse(geode);
    }

    void apply(osg::Node* n, osg::Drawable* d, osg::StateSet& ss)
    {
        osg::StateSet::TextureAttributeList& texAttrList = ss.getTextureAttributeList();
        for (size_t i = 0; i < texAttrList.size(); ++i)
        {
            osg::StateSet::AttributeList& attr = texAttrList[i];
            for (osg::StateSet::AttributeList::iterator itr = attr.begin();
                itr != attr.end(); ++itr)
            {
                osg::StateAttribute::Type t = itr->first.first;
                if (t == osg::StateAttribute::TEXTURE)
                    apply(n, d, static_cast<osg::Texture*>(itr->second.first.get()), i);
            }
        }
    }

    void apply(osg::Node* n, osg::Drawable* d, osg::Texture* tex, int u)
    {
        if (beBetter)
        {
            tex->setResizeNonPowerOfTwoHint(false);
            tex->setUnRefImageDataAfterApply(true);
        }
        else
        {
            for (unsigned int i = 0; i < tex->getNumImages(); ++i)
            {
                osg::Image* img = tex->getImage(i);
                if (img && img->valid())
                {
                    int s = osg::minimum(img->s() * 4, 8192);
                    int t = osg::minimum(img->t() * 4, 8192);
                    img->scaleImage(s - 20, t - 15, 1);
                }
            }
            tex->setResizeNonPowerOfTwoHint(true);
        }
    }

    TextureChecker(bool b) : beBetter(b) {}
    bool beBetter;
};

struct DataMergeCallback : public osgVerse::DatabasePager::DataMergeCallback
{
    virtual FilterResult filter(osg::PagedLOD* parent, const std::string& name, osg::Node*)
    {
        if (name.find(_prefixToScale) != std::string::npos)
        {
            bool scaled = false;
            if (parent->getUserValue("LodScaled", scaled))
            { if (scaled) return FilterResult::MERGE_NOW; }

            for (size_t i = 0; i < parent->getNumFileNames(); ++i)
            {
                float minV = parent->getMinRange(i), maxV = parent->getMaxRange(i);
                switch (parent->getRangeMode())
                {
                case osg::PagedLOD::PIXEL_SIZE_ON_SCREEN:
                    parent->setRange(i, minV * _lodScale, maxV * _lodScale); break;
                case osg::PagedLOD::DISTANCE_FROM_EYE_POINT:
                    parent->setRange(i, minV / _lodScale, maxV / _lodScale); break;
                }
            }
            parent->setUserValue("LodScaled", true);
            return FilterResult::DISCARDED;
        }
        return FilterResult::MERGE_NOW;
    }

    DataMergeCallback(const std::string& p, float s) : _prefixToScale(p), _lodScale(s) {}
    std::string _prefixToScale; float _lodScale;
};

void processFile(const std::string& prefix, const std::string& dirName,
                 const std::string& dbBase, const std::string& fileName, bool savingToDB)
{
    std::string tileName = prefix + dirName + "/" + fileName;
    osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(tileName);
    if (node.valid())
    {
        std::string dbFileName = dbBase + dirName + "/" + fileName;
        if (!savingToDB) osgDB::makeDirectoryForFile(dbFileName);

#if false
        TextureChecker texChecker(true);
        texChecker.setTraversalMode(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN);
        node->accept(texChecker);
        osg::ref_ptr<osgDB::Options> options = new osgDB::Options("WriteImageHint=IncludeFile");
        osgDB::writeNodeFile(*node, dbFileName, options.get());
#else
        GeomCompressor rdp;
        rdp.setTraversalMode(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN);
        node->accept(rdp);

        osgVerse::TextureOptimizer opt(true, "optimize_tex_" + nanoid::generate(8));
        opt.setGeneratingMipmaps(true); node->accept(opt);

        osg::ref_ptr<osgDB::Options> options = new osgDB::Options("WriteImageHint=IncludeFile");
        options->setPluginStringData("UseBASISU", "1");
        osgDB::writeNodeFile(*node, dbFileName, options.get());
        opt.deleteSavedTextures();
#endif
        std::cout << "Re-saving " << fileName << "\n";
    }
}

int main(int argc, char** argv)
{
#ifndef OSG_LIBRARY_STATIC
    osgDB::Registry::instance()->loadLibrary(
        osgDB::Registry::instance()->createLibraryNameForNodeKit("osgVerseReaderWriter"));
    osgDB::Registry::instance()->loadLibrary(
        osgDB::Registry::instance()->createLibraryNameForExtension("verse_leveldb"));
#endif

    osg::ArgumentParser arguments(&argc, argv);
    std::string output; arguments.read("--output", output);
    bool withDraco = arguments.read("--enable-draco");
    bool withKtx = !arguments.read("--disable-ktx");
    bool gpuMerge = !arguments.read("--cpu-merge");

    std::string input("cow.osg"), archiveName;
    if (arguments.read("--archive", archiveName))
    {
        osg::ref_ptr<osgDB::Archive> archive = osgDB::openArchive(archiveName, osgDB::Archive::CREATE, 4096);
        arguments.read("--source", input);
        if (archive.valid())
        {
            osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(input);
            if (node.valid()) archive->writeNode(*node, input);
            archive->close();
        }
        else
            OSG_WARN << "Archive creating failed: " << archiveName << "\n";
        return 0;
    }

    osgVerse::fixOsgBinaryWrappers();
    if (argc > 3 && std::string(argv[1]) == "adj")
    {
        std::string srcDir = std::string(argv[2]), dstDir = std::string(argv[3]);
        osg::ref_ptr<osgVerse::TileOptimizer> opt = new osgVerse::TileOptimizer(dstDir);
        if (!opt->prepare(srcDir, "([+-]?\\d+)", withDraco, withKtx, gpuMerge)) return 1;
        opt->setUseThreads(10); opt->processAdjacency(2, 2); return 0;
    }
    else if (argc > 3 && std::string(argv[1]) == "top")
    {
        std::string srcDir = std::string(argv[2]), dstDir = std::string(argv[3]);
        osg::ref_ptr<osgVerse::TileOptimizer> opt = new osgVerse::TileOptimizer(dstDir);
        if (!opt->prepare(srcDir, "([+-]?\\d+)", withDraco, withKtx, gpuMerge)) return 1;
        opt->setUseThreads(10); opt->processGroundLevel(2, 2); return 0;
    }

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->setName("PlodGridRoot");

    // Benchmark of geometry & texture optimization:
    // Before: Data size: 419MB, CPU memory 185.7MB, GPU memory: 0.6GB
    // After : Data size: 154MB, CPU memory 150.1MB, GPU memory: 0.4GB
    osgViewer::Viewer viewer;
    if (argc > 4 && std::string(argv[1]) == "opt")
    {
        std::string prefix = std::string(argv[2]), dbBase = std::string(argv[3]);
        std::string outputFile = std::string(argv[4]);
        bool savingToDB = (dbBase.find("leveldb://") != dbBase.npos);

        ghc::filesystem::path dir(prefix);
        for (auto itr : ghc::filesystem::directory_iterator(dir))
        {
            const ghc::filesystem::path& file = itr.path();
            std::cout << file.filename().string() << "..........\n";
            if (itr.is_directory())
            {
                std::string dirName = file.filename().string();
                std::string rootTileName = prefix + dirName + "/" + dirName + ".osgb";
                if (dirName.empty() || dirName[0] == '.') continue;

                osgDB::DirectoryContents osgbFiles = osgDB::getDirectoryContents(prefix + dirName);
                ThreadPool pool(std::thread::hardware_concurrency()); // Create a thread pool
                for (size_t i = 0; i < osgbFiles.size(); ++i)
                {
                    std::string fileName = osgbFiles[i]; if (fileName.empty() || fileName[0] == '.') continue;
                    pool.enqueue([=]() { processFile(prefix, dirName, dbBase, fileName, savingToDB); });
                }

                osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(rootTileName);
                if (node.valid())
                {
                    osg::ref_ptr<osg::PagedLOD> plod = new osg::PagedLOD;
                    plod->setDatabasePath(dbBase);
                    plod->setCenterMode(osg::LOD::USER_DEFINED_CENTER);
                    plod->setCenter(node->getBound().center());
                    plod->setRadius(node->getBound().radius());
                    plod->addChild(new osg::Node);
                    plod->setFileName(1, dirName + "/" + dirName + ".osgb");
                    plod->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);
                    plod->setRange(0, 150000.0f, FLT_MAX);
                    plod->setRange(1, 0.0f, 150000.0f);
                    plod->setName(dirName);
                    root->addChild(plod.get());
                    std::cout << dirName << " added..." << std::endl;
                }
                else
                    std::cout << "[Error] no root tile: " << rootTileName << std::endl;
            }
        }
        osgDB::writeNodeFile(*root, outputFile); return 0;
    }
    else if (argc > 2 && std::string(argv[1]) == "opt_single")
    {
        osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(argv[2]);
        if (node.valid())
        {
            GeomCompressor rdp;
            rdp.setTraversalMode(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN);
            node->accept(rdp);

            osgVerse::TextureOptimizer opt(true, "optimize_tex_" + nanoid::generate(8));
            opt.setGeneratingMipmaps(true); node->accept(opt);

            osg::ref_ptr<osgDB::Options> options = new osgDB::Options("WriteImageHint=IncludeFile");
            options->setPluginStringData("UseBASISU", "1");
            osgDB::writeNodeFile(*node, std::string(argv[2]) + "_opt.osgb", options.get());
            opt.deleteSavedTextures(); return 0;
        }
    }
    else if (argc > 1)
    {
        std::string prefix = ""; arguments.read("--prefix", prefix);
        float lodScale = 1.0f; arguments.read("--lod-scale", lodScale);
        viewer.getCamera()->setLODScale(lodScale);
        root->addChild(osgDB::readNodeFiles(arguments));

        //osgVerse::DatabasePager* dbPager = new osgVerse::DatabasePager;
        //dbPager->setDataMergeCallback(new DataMergeCallback(prefix, lodScale));
        //viewer.setDatabasePager(dbPager);
        //viewer.setIncrementalCompileOperation(new osgUtil::IncrementalCompileOperation);
        viewer.getDatabasePager()->setDrawablePolicy(osgDB::DatabasePager::USE_VERTEX_BUFFER_OBJECTS);
        viewer.getDatabasePager()->setUnrefImageDataAfterApplyPolicy(true, true);
        viewer.getDatabasePager()->setTargetMaximumNumberOfPageLOD(1000);
    }
    else
    {
        std::cout << "Usage: " << argv[0] << " 'adj/top/opt' <input_osgb_path> <output_path> <total_file>\n";
        std::cout << "      To save to database, set <output_path> to 'leveldb://factory.db/'";
        return 1;
    }

    std::string pathfile;
    if (arguments.read("-p", pathfile))
    {
        osgGA::AnimationPathManipulator* apm = new osgGA::AnimationPathManipulator(pathfile);
        if (!apm->getAnimationPath()->empty()) viewer.setCameraManipulator(apm);
    }
    else
        viewer.setCameraManipulator(new osgGA::TrackballManipulator);

    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getStateSet()));
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(0);
    viewer.run();

    if (!output.empty()) osgDB::writeNodeFile(*root, output, new osgDB::Options("WriteImageHint=WriteOut"));
    return 0;
}
