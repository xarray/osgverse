#include <osg/io_utils>
#include <osg/TriangleIndexFunctor>
#include <osg/Texture2D>
#include <osg/Depth>
#include <osg/MatrixTransform>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <modeling/Math.h>
#include <animation/ParticleEngine.h>
#include <readerwriter/Utilities.h>
#include <pipeline/Utilities.h>
#include <iostream>
#include <sstream>

#ifdef OSG_LIBRARY_STATIC
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
#endif

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    if (arguments.read("--csv"))
    {
        std::string csvFile, outFile("result.particle");
        arguments.read("--in", csvFile); arguments.read("--out", outFile);
        if (csvFile.empty() || outFile.empty()) return 1;

        std::ifstream in(csvFile.c_str(), std::ios::in);
        std::ofstream out(outFile.c_str(), std::ios::out | std::ios::binary);
        osg::ref_ptr<osgVerse::ParticleCloud> pc = new osgVerse::ParticleCloud;
        pc->loadFromCsv(in, [](osgVerse::ParticleCloud& cloud, unsigned int id,
                               std::map<std::string, std::string>& values)
        {
#if true
            // x,y,z,amp,lat,lon
            double lat = atof(values["lat"].c_str()), lon = atof(values["lon"].c_str());
            double z = -atof(values["z"].c_str()), power = atof(values["amp"].c_str());
            if (!(id % 100000)) std::cout << "ID = " << id << ": Saving " << cloud.size() << " points\n";
            if (power == 0.0) return true;

            //static std::map<osg::Vec3i, int> s_hashMap;
            //osg::Vec3i key(int(lat * 10000000.0f), int(lon * 10000000.0f), int(z / 250));
            //if (s_hashMap.find(key) != s_hashMap.end()) return true;
            //else s_hashMap[key] = 1;
            static std::map<int, int> ampValues; ampValues[(int)(power / 5.0)]++;
            static osg::BoundingBoxd bb; bb.expandBy(osg::Vec3d(lat, lon, z));
            static double pMin = FLT_MAX, pMax = -FLT_MAX;
            if (power < pMin) pMin = power; if (power > pMax) pMax = power;
            if (!(id % 100000))
            {
                if (!(id % 10000000))
                {
                    for (std::map<int, int>::iterator itr = ampValues.begin(); itr != ampValues.end(); ++itr)
                        std::cout << "    " << (itr->first * 5) << ": Count = " << itr->second << "\n";
                }
                std::cout << bb._min << " -- " << bb._max << "; Power [" << pMin << ", " << pMax << "]\n";
            }
            cloud.add(osg::Vec3(lat * 10000.0, lon * 10000.0, z * 0.5), osg::Vec4(1.0f, 1.0f, 1.0f, 0.2f),
                      osg::Vec3(), osg::Vec4(power, 0.0f, 0.0f, 0.0f), 10000.0f);
#else
            // ...,latitude,longitude,depth,mag,...
            double lat = atof(values["latitude"].c_str()), lon = atof(values["longitude"].c_str());
            double z = -1000.0 * atof(values["depth"].c_str()), power = atof(values["mag"].c_str());

            osg::Vec3d pos = osgVerse::Coordinate::convertLLAtoECEF(
                osg::Vec3d(osg::inDegrees(lat), osg::inDegrees(lon), z));
            cloud.add(pos, osg::Vec4(1.0f, 1.0f, 1.0f, 0.2f), osg::Vec3(),
                      osg::Vec4(power, 0.0f, 0.0f, 0.0f), 10000.0f);
#endif
            return true;
        });
        pc->save(out); out.close(); return 0;
    }
    else if (arguments.read("--vdb"))
    {
        std::string inFile("in.particle"), vdbFile;
        arguments.read("--in", inFile); arguments.read("--out", vdbFile);
        if (inFile.empty() || vdbFile.empty()) return 1;

        std::ifstream in(inFile.c_str(), std::ios::in | std::ios::binary);
        osg::ref_ptr<osgVerse::ParticleCloud> pc = new osgVerse::ParticleCloud;
        pc->load(in); in.close(); std::cout << "Loaded " << pc->size() << " particles\n";

        float scale = 1.0f; arguments.read("--scale", scale);
        if (!osgDB::writeObjectFile(*pc, vdbFile)) std::cout << "Failed to save VDB cloud!\n";
        else std::cout << "Saved particle cloud as VDB!\n"; pc = NULL;

        osg::ref_ptr<osg::Image> image = osgDB::readImageFile(
            vdbFile, new osgDB::Options("DimensionScale=" + std::to_string(scale)));
        if (!image) { std::cout << "Failed to read VDB image!\n"; return 1; }
        std::cout << image->s() << 'x' << image->t() << "x" << image->r() << "\n";
        osgDB::writeImageFile(*image, vdbFile + "_img3d.vdb"); return 0;
    }

    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFiles(arguments);
    if (!scene) scene = osgDB::readNodeFile("cessna.osgt.1000,1000,1000.scale");
    if (!scene) { OSG_WARN << "Failed to load scene model " << (argc < 2) ? "" : argv[1]; }

    osg::Vec3d pos = osgVerse::Coordinate::convertLLAtoECEF(
        osg::Vec3d(osg::inDegrees(-39.996486), osg::inDegrees(174.214090), 0.0));
    osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
    mt->setMatrix(osg::Matrix::translate(pos)); mt->addChild(scene.get());

    osg::ref_ptr<osg::Geode> particleNode = new osg::Geode;
    std::string cloudFile; arguments.read("--cloud", cloudFile);
    bool useGeomShader = !arguments.read("--no-geometry-shader");
    {
        osg::ref_ptr<osgVerse::ParticleCloud> pointCloud = new osgVerse::ParticleCloud;
        if (cloudFile.empty())
        {
            for (int z = 0; z < 100; ++z) for (int y = 0; y < 100; ++y) for (int x = 0; x < 100; ++x)
            {
                if (x % 2) pointCloud->add(osg::Vec3(x * 2, y * 2, z * 2), osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
                else pointCloud->add(osg::Vec3(x * 2, y * 2, z * 2), osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
            }
            //std::ofstream out("../result.particle", std::ios::out | std::ios::binary); pointCloud->save(out);
        }
        else
        {
            std::ifstream in(cloudFile.c_str(), std::ios::in | std::ios::binary);
            pointCloud->load(in); in.close();
        }

        osg::ref_ptr<osg::Image> img = osgDB::readImageFile(BASE_DIR + "/textures/water_drop.png");
        osg::ref_ptr<osg::Shader> vs = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "particles.vert.glsl");
        osg::ref_ptr<osg::Shader> fs = osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "particles.frag.glsl");
        osg::ref_ptr<osg::Shader> gs = osgDB::readShaderFile(osg::Shader::GEOMETRY, SHADER_DIR + "particles.geom.glsl");
        osgVerse::ParticleSystemU3D::UpdateMethod method = useGeomShader ?
            osgVerse::ParticleSystemU3D::GPU_GEOMETRY : osgVerse::ParticleSystemU3D::CPU_VERTEX_ATTRIB;

        osg::ref_ptr<osgVerse::ParticleSystemU3D> cloud = new osgVerse::ParticleSystemU3D(method);
        cloud->setTexture(osgVerse::createTexture2D(img.get()));
        cloud->setParticleType(osgVerse::ParticleSystemU3D::PARTICLE_Billboard);
        cloud->setBlendingType(osgVerse::ParticleSystemU3D::BLEND_Additive);
        cloud->setPointCloud(pointCloud.get(), true);
        cloud->setGravityScale(0.0f); cloud->setAspectRatio(16.0 / 9.0);
        cloud->linkTo(particleNode.get(), true, vs.get(), fs.get(), gs.get());
    }

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(mt.get());
    root->addChild(particleNode.get());

    // Start the main loop
    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}
