#include <osg/io_utils>
#include <osg/ValueObject>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
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

#include <readerwriter/DatabasePager.h>
#include <readerwriter/Utilities.h>
#include <readerwriter/OsgbTileOptimizer.h>
#include <readerwriter/DracoProcessor.h>
#ifdef OSG_LIBRARY_STATIC
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
USE_SERIALIZER_WRAPPER(DracoGeometry)
#endif

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

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

class RemoveDataPathVisitor : public osg::NodeVisitor
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
        RemoveDataPathVisitor rdp;
        rdp.setTraversalMode(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN);
        node->accept(rdp);

        osgVerse::TextureOptimizer opt(true);
        node->accept(opt);

        std::string dbFileName = dbBase + dirName + "/" + fileName;
        if (!savingToDB) osgDB::makeDirectoryForFile(dbFileName);

        osg::ref_ptr<osgDB::Options> options = new osgDB::Options("WriteImageHint=IncludeFile");
        options->setPluginStringData("UseBASISU", "1");
        osgDB::writeNodeFile(*node, dbFileName, options.get());
        opt.deleteSavedTextures();
        std::cout << "Re-saving " << fileName << "\n";
    }
}

int main(int argc, char** argv)
{
    osgVerse::fixOsgBinaryWrappers();
    if (argc > 3 && std::string(argv[1]) == "adj")
    {
        std::string srcDir = std::string(argv[2]), dstDir = std::string(argv[3]);
        osg::ref_ptr<osgVerse::TileOptimizer> opt = new osgVerse::TileOptimizer(dstDir);
        if (!opt->prepare(srcDir)) { printf("Can't prepare for tiles\n"); return 1; }
        opt->setUseThreads(10); opt->processAdjacency(2, 2); return 0;
    }
    else if (argc > 3 && std::string(argv[1]) == "top")
    {
        std::string srcDir = std::string(argv[2]), dstDir = std::string(argv[3]);
        osg::ref_ptr<osgVerse::TileOptimizer> opt = new osgVerse::TileOptimizer(dstDir);
        if (!opt->prepare(srcDir)) { printf("Can't prepare for tiles\n"); return 1; }
        opt->setUseThreads(10); opt->processGroundLevel(2, 2); return 0;
    }

#ifndef OSG_LIBRARY_STATIC
    osgDB::Registry::instance()->loadLibrary(
        osgDB::Registry::instance()->createLibraryNameForNodeKit("osgVerseReaderWriter"));
    osgDB::Registry::instance()->loadLibrary(
        osgDB::Registry::instance()->createLibraryNameForExtension("verse_leveldb"));
#endif
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
        osgDB::writeNodeFile(*root, outputFile);
    }
    else if (argc > 1)
    {
        osg::ArgumentParser arguments(&argc, argv);
        std::string prefix = ""; arguments.read("--prefix", prefix);
        float lodScale = 1.0f; arguments.read("--lod-scale", lodScale);
        //viewer.getCamera()->setLODScale(lodScale);
        root->addChild(osgDB::readNodeFiles(arguments));

        osgVerse::DatabasePager* dbPager = new osgVerse::DatabasePager;
        dbPager->setDataMergeCallback(new DataMergeCallback(prefix, lodScale));
        viewer.setDatabasePager(dbPager);
        viewer.setIncrementalCompileOperation(new osgUtil::IncrementalCompileOperation);
    }
    else
    {
        std::cout << "Usage: " << argv[0] << " 'adj/opt' <input_osgb_path> <output_path> <total_file>\n";
        std::cout << "      To save to database, set <output_path> to 'leveldb://factory.db/'";
        return 1;
    }

    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(0);
    return viewer.run();
}
