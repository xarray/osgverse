#include PREPENDED_HEADER
#include <osg/io_utils>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Group>
#include <osg/PolygonMode>
#include <osg/Timer>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgText/Text>

#include <pipeline/Global.h>
#include <pipeline/Pipeline.h>
#include <pipeline/ShaderLibrary.h>
#include <pipeline/Utilities.h>
#include <readerwriter/Utilities.h>
#include <modeling/Utilities.h>
#include <cfloat>
#include <cstdio>
#include <iostream>
#include <map>
#include <sstream>

#include <meshoptimizer/meshoptimizer.h>
#define CLUSTERLOD_IMPLEMENTATION
#include <meshoptimizer/clusterlod.h>

#ifdef OSG_LIBRARY_STATIC
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
#endif
USE_GRAPICSWINDOW_IMPLEMENTATION(SDL)
USE_GRAPICSWINDOW_IMPLEMENTATION(GLFW)

// Nanite-like cluster LOD visualization, based on meshoptimizer's clusterlod + the
// reference nanite demo: https://github.com/zeux/meshoptimizer/tree/master/demo
//
// Goals of this step:
// 1. Bake a cluster DAG from any OSG model and dump per-LOD statistics;
// 2. Visualize one valid DAG cut with per-cluster colors ("patched model" look);
// 3. Visualize cluster bounds colored by DAG level to verify the hierarchy.
//
// For reference, see the original Nanite paper: Brian Karis. Nanite: A Deep Dive. 2021

// Interleaved vertex layout, matching the reference demo so that attribute_protect_mask
// bits 3/4 can be used for UV seams. Do NOT reorder the members.
struct ClusterVertex
{
    float px, py, pz;
    float nx, ny, nz;
    float tx, ty;
};

struct ClusterInfo
{
    int refined;  // id of the (finer) group this cluster was simplified from, -1 for original geometry
    int group;  // id of the group owning this cluster
    unsigned int triangleCount;

    // cluster bounds; error is not monotonic across the DAG so it is only used for culling
    float center[3], radius;
    std::vector<unsigned int> indices;
};

struct GroupInfo
{
    // DAG level the group was generated at; level 0 is the original geometry
    int depth;

    // bounds of the simplified version of this group, error is FLT_MAX for terminal groups
    float center[3], radius, error;
};

class ClusterDAG : public osg::Referenced
{
public:
    std::vector<ClusterInfo> clusters;
    std::vector<GroupInfo> groups;

    /** Bake a cluster LOD DAG from the given vertex/index buffers */
    bool build(const std::vector<ClusterVertex>& vertices, const std::vector<unsigned int>& indices,
               size_t maxTriangles, bool verbose)
    {
        static const float attributeWeights[3] = {0.5f, 0.5f, 0.5f};
        if (vertices.empty() || indices.empty())
        { OSG_WARN << "[Nanite] Empty mesh, nothing to build" << std::endl; return false; }

        clodMesh mesh = {};
        mesh.indices = indices.data();
        mesh.index_count = indices.size();
        mesh.vertex_count = vertices.size();
        mesh.vertex_positions = &vertices[0].px;
        mesh.vertex_positions_stride = sizeof(ClusterVertex);
        mesh.vertex_attributes = &vertices[0].nx;
        mesh.vertex_attributes_stride = sizeof(ClusterVertex);
        mesh.attribute_weights = attributeWeights;
        mesh.attribute_count = 3;
        mesh.attribute_protect_mask = (1 << 3) | (1 << 4);  // protect UV seams (tx/ty)

        clodConfig config = clodDefaultConfig(maxTriangles);
        osg::Timer_t t0 = osg::Timer::instance()->tick();

        // clodBuild() emits groups in order of increasing depth, and the returned value of
        // the callback is recorded in the next level clusters as clodCluster::refined
        clodBuild(config, mesh, [&](clodGroup group, const clodCluster* clusterList, size_t clusterCount) -> int
        {
            int groupIndex = int(groups.size());
            GroupInfo gi;
            gi.depth = group.depth;
            gi.center[0] = group.simplified.center[0];
            gi.center[1] = group.simplified.center[1];
            gi.center[2] = group.simplified.center[2];
            gi.radius = group.simplified.radius;
            gi.error = group.simplified.error;
            groups.push_back(gi);

            for (size_t i = 0; i < clusterCount; ++i)
            {
                const clodCluster& c = clusterList[i];
                ClusterInfo ci;
                ci.refined = c.refined;
                ci.group = groupIndex;
                ci.triangleCount = unsigned(c.index_count / 3);
                ci.center[0] = c.bounds.center[0];
                ci.center[1] = c.bounds.center[1];
                ci.center[2] = c.bounds.center[2];
                ci.radius = c.bounds.radius;

                // NOTE: c.indices points into clusterlod's internal storage and is only valid
                // during this callback, so the data MUST be copied out right here
                ci.indices.assign(c.indices, c.indices + c.index_count);
                clusters.push_back(ci);
            }
            return groupIndex;
        });

        double time = osg::Timer::instance()->delta_s(t0, osg::Timer::instance()->tick());
        if (verbose) printStats(time); return !groups.empty();
    }

    /** Screen-space error of a group, normalized to 0..1 (multiply by screen height for pixels).
        Same formula as the reference nanite demo: error / max(distance - radius, znear) * (proj * 0.5) */
    static float screenError(const GroupInfo& g, const osg::Vec3& eye, float proj, float znear)
    {
        osg::Vec3 d = osg::Vec3(g.center[0], g.center[1], g.center[2]) - eye;
        float dist = d.length() - g.radius;
        return g.error / std::max(dist, znear) * (proj * 0.5f);
    }

    /** Get a DAG cut at a given level. Terminal clusters of finer levels are always included,
        otherwise holes would appear because they have no coarser replacement at all */
    void collectCutByLevel(int level, std::vector<unsigned int>& out) const
    {
        out.clear();
        for (size_t i = 0; i < clusters.size(); ++i)
        {
            const GroupInfo& g = groups[clusters[i].group];
            if (g.depth == level || (g.depth < level && g.error == FLT_MAX))
                out.push_back(unsigned(i));
        }
    }

    /** Get a DAG cut from a viewpoint. A cluster should be rendered if:
        1. the group owning it is over the error threshold, and
        2. it is original geometry, or the group it was refined from is at/under the threshold */
    void collectCutByView(const osg::Vec3& eye, float proj, float znear, float threshold,
                          std::vector<unsigned int>& out) const
    {
        out.clear();
        for (size_t i = 0; i < clusters.size(); ++i)
        {
            const ClusterInfo& c = clusters[i];
            const GroupInfo& g = groups[c.group];
            bool refinedReady = (c.refined < 0) ||
                (screenError(groups[c.refined], eye, proj, znear) <= threshold);
            if (refinedReady && screenError(g, eye, proj, znear) > threshold)
                out.push_back(unsigned(i));
        }
    }

    void printStats(double seconds) const
    {
        struct LevelStats
        {
            LevelStats() : groups(0), clusters(0), triangles(0), terminal(0), radiusSum(0.0) {}
            size_t groups, clusters, triangles, terminal;
            double radiusSum;
        };
        std::map<int, LevelStats> stats;

        for (size_t i = 0; i < groups.size(); ++i)
        {
            LevelStats& s = stats[groups[i].depth];
            s.groups++;
            if (groups[i].error == FLT_MAX) s.terminal++;
        }
        for (size_t i = 0; i < clusters.size(); ++i)
        {
            LevelStats& s = stats[groups[clusters[i].group].depth];
            s.clusters++;
            s.triangles += clusters[i].triangleCount;
            s.radiusSum += clusters[i].radius;
        }

        std::printf("[Nanite] Cluster LOD baked in %.3fs: %d groups, %d clusters\n",
                    seconds, int(groups.size()), int(clusters.size()));
        std::printf("  %-4s %9s %11s %8s %9s %8s %11s\n",
                    "lod", "clusters", "triangles", "tri/cl", "groups", "stuck", "radius/cl");
        for (std::map<int, LevelStats>::const_iterator it = stats.begin(); it != stats.end(); ++it)
        {
            const LevelStats& s = it->second;
            double inv = 1.0 / double(s.clusters);
            std::printf("  %-4d %9d %11d %8.1f %9d %8d %11.3f (%.1f%% stuck)\n",
                        it->first, int(s.clusters), int(s.triangles), double(s.triangles) * inv,
                        int(s.groups), int(s.terminal), s.radiusSum * inv,
                        double(s.terminal) / double(s.groups) * 100.0);
        }
        std::fflush(stdout);
    }
};

// Get a stable, well-spread color from a value in 0..1
static osg::Vec4 hsvColor(float h, float s = 0.72f, float v = 0.98f)
{
    h = h - floorf(h);
    float i = floorf(h * 6.0f), f = h * 6.0f - i;
    float p = v * (1.0f - s), q = v * (1.0f - s * f), t = v * (1.0f - s * (1.0f - f));
    switch (int(i) % 6)
    {
    case 0: return osg::Vec4(v, t, p, 1.0f);
    case 1: return osg::Vec4(q, v, p, 1.0f);
    case 2: return osg::Vec4(p, v, t, 1.0f);
    case 3: return osg::Vec4(p, q, v, 1.0f);
    case 4: return osg::Vec4(t, p, v, 1.0f);
    default: return osg::Vec4(v, p, q, 1.0f);
    }
}

static osg::Vec4 levelColor(int depth)
{ return hsvColor(0.618034f * float(depth) + 0.05f); }

static osg::Vec4 clusterColor(unsigned int id)
{
    unsigned int h = id * 2654435761u;  // golden ratio hash for stable per-cluster colors
    return hsvColor(float(h & 0xffffu) / 65535.0f);
}

// Render one DAG cut: all visible clusters are emitted as a single non-indexed geometry with
// per-vertex colors. Vertices are duplicated on purpose: clusters share the original vertex
// buffer, and sharing it here would blur colors across cluster borders
class CutGeometry : public osg::Referenced
{
public:
    CutGeometry(ClusterDAG* dag, const std::vector<ClusterVertex>& vertices)
    :   _dag(dag), _vertices(vertices), _clusters(0), _triangles(0), _colorByDepth(false)
    {
        _geometry = new osg::Geometry;
        _geometry->setUseDisplayList(false);
        _geometry->setUseVertexBufferObjects(true);
        _geometry->setVertexArray(new osg::Vec3Array);
        _geometry->setNormalArray(new osg::Vec3Array, osg::Array::BIND_PER_VERTEX);
        _geometry->setColorArray(new osg::Vec4Array, osg::Array::BIND_PER_VERTEX);
        _geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, 0));
    }

    void setColorByDepth(bool b) { _colorByDepth = b; }
    osg::Geometry* getGeometry() const { return _geometry.get(); }
    unsigned int getClusterCount() const { return _clusters; }
    unsigned int getTriangleCount() const { return _triangles; }

    /** Rebuild the geometry from the given cut; returns false if the cut did not change */
    bool update(const std::vector<unsigned int>& cut)
    {
        if (cut == _cut) return false; _cut = cut;
        osg::Vec3Array* va = static_cast<osg::Vec3Array*>(_geometry->getVertexArray());
        osg::Vec3Array* na = static_cast<osg::Vec3Array*>(_geometry->getNormalArray());
        osg::Vec4Array* ca = static_cast<osg::Vec4Array*>(_geometry->getColorArray());

        size_t total = 0, w = 0;
        for (size_t i = 0; i < cut.size(); ++i)
            total += size_t(_dag->clusters[cut[i]].triangleCount) * 3;
        va->resize(total); na->resize(total); ca->resize(total);

        for (size_t i = 0; i < cut.size(); ++i)
        {
            const ClusterInfo& c = _dag->clusters[cut[i]];
            const osg::Vec4& color = _colorByDepth ? levelColor(_dag->groups[c.group].depth)
                                                   : clusterColor(cut[i]);
            for (size_t j = 0; j < c.indices.size(); ++j)
            {
                const ClusterVertex& v = _vertices[c.indices[j]];
                (*va)[w] = osg::Vec3(v.px, v.py, v.pz);
                (*na)[w] = osg::Vec3(v.nx, v.ny, v.nz);
                (*ca)[w] = color; ++w;
            }
        }

        static_cast<osg::DrawArrays*>(_geometry->getPrimitiveSet(0))->setCount(int(total));
        _clusters = unsigned(cut.size()); _triangles = unsigned(total / 3);
        va->dirty(); na->dirty(); ca->dirty(); _geometry->dirtyBound(); return true;
    }

protected:
    osg::ref_ptr<osg::Geometry> _geometry;
    osg::observer_ptr<ClusterDAG> _dag;
    const std::vector<ClusterVertex>& _vertices;
    std::vector<unsigned int> _cut;
    unsigned int _clusters, _triangles;
    bool _colorByDepth;
};

// Render cluster bounds as wireframe circles, colored by DAG level
static osg::Geometry* createClusterBoundsGeometry(const ClusterDAG* dag, int maxLevel, int segments)
{
    osg::Vec3Array* va = new osg::Vec3Array;
    osg::Vec4Array* ca = new osg::Vec4Array;
    const float pi2 = 6.2831853f;
    for (size_t i = 0; i < dag->clusters.size(); ++i)
    {
        const ClusterInfo& c = dag->clusters[i];
        int depth = dag->groups[c.group].depth;
        if (maxLevel >= 0 && depth > maxLevel) continue;

        osg::Vec4 color = levelColor(depth);
        osg::Vec3 center(c.center[0], c.center[1], c.center[2]);
        for (int axis = 0; axis < 3; ++axis)
            for (int s = 0; s < segments; ++s)
            {
                float a0 = float(s) / float(segments) * pi2;
                float a1 = float(s + 1) / float(segments) * pi2;

                // build the 3 orthogonal circles of the cluster bounding sphere
                osg::Vec3 d0, d1;
                if (axis == 0)
                    { d0 = osg::Vec3(0.0f, cosf(a0), sinf(a0)); d1 = osg::Vec3(0.0f, cosf(a1), sinf(a1)); }
                else if (axis == 1)
                    { d0 = osg::Vec3(cosf(a0), 0.0f, sinf(a0)); d1 = osg::Vec3(cosf(a1), 0.0f, sinf(a1)); }
                else
                    { d0 = osg::Vec3(cosf(a0), sinf(a0), 0.0f); d1 = osg::Vec3(cosf(a1), sinf(a1), 0.0f); }

                va->push_back(center + d0 * c.radius);
                va->push_back(center + d1 * c.radius);
                ca->push_back(color); ca->push_back(color);
            }
    }

    // a constant normal array is needed because the shared shader reads osg_Normal
    osg::Vec3Array* na = new osg::Vec3Array;
    na->resize(va->size());
    for (size_t i = 0; i < na->size(); ++i) (*na)[i] = osg::Vec3(0.0f, 0.0f, 1.0f);

    osg::Geometry* geom = new osg::Geometry;
    geom->setUseDisplayList(false);
    geom->setUseVertexBufferObjects(true);
    geom->setVertexArray(va);
    geom->setNormalArray(na, osg::Array::BIND_PER_VERTEX);
    geom->setColorArray(ca, osg::Array::BIND_PER_VERTEX);
    geom->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, va->size()));
    return geom;
}

static const char* vertCode = {
    "VERSE_VS_OUT vec3 normalInEye;\n"
    "VERSE_VS_OUT vec4 vertexColor;\n"
    "void main() {\n"
    "    normalInEye = VERSE_MATRIX_N * osg_Normal;\n"
    "    vertexColor = osg_Color;\n"
    "    gl_Position = VERSE_MATRIX_MVP * osg_Vertex;\n"
    "}\n"
};

static const char* fragCode = {
    "uniform vec3 lightDir;\n"
    "uniform float shadeMix;\n"
    "VERSE_FS_IN vec3 normalInEye;\n"
    "VERSE_FS_IN vec4 vertexColor;\n"
    "VERSE_FS_OUT vec4 fragData;\n"
    "void main() {\n"
    "    // Two-sided headlight shading: without it a flat per-cluster color gives no depth cue\n"
    "    // at all, and LOD switches would look like chunks being cut away\n"
    "    float dTerm = abs(dot(normalize(normalInEye), normalize(lightDir)));\n"
    "    float shade = mix(1.0, 0.35 + 0.65 * dTerm, shadeMix);\n"
    "    fragData = vec4(vertexColor.rgb * shade, vertexColor.a);\n"
    "    VERSE_FS_FINAL(fragData);\n"
    "}\n"
};

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());

    std::string mode("cut"); arguments.read("--mode", mode);
    std::string colorMode("cluster"); arguments.read("--color", colorMode);
    int maxTriangles = 128; arguments.read("--max-tris", maxTriangles);
    int cutLevel = 0; arguments.read("--level", cutLevel);
    int maxLevel = -1; arguments.read("--max-level", maxLevel);
    int interval = 5; arguments.read("--interval", interval);
    int segments = 12; arguments.read("--segments", segments);
    int screenNo = 0; arguments.read("--screen", screenNo);
    float pixels = 2.0f; arguments.read("--error", pixels);
    bool wireframe = arguments.read("--wireframe");
    bool unlit = arguments.read("--unlit");
    bool verbose = arguments.read("--quiet") ? false : true;

    if (maxTriangles < 4) maxTriangles = 4;
    else if (maxTriangles > 256) maxTriangles = 256;

    // Load the model and collect it into a single indexed mesh
    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFiles(arguments);
    if (!scene) scene = osgDB::readNodeFile(BASE_DIR + "/models/Sponza/Sponza.gltf");
    if (!scene)
    {
        OSG_WARN << "[Nanite] Failed to load scene model. "
                 << "Please provide one as argument" << std::endl; return 1;
    }

    osgVerse::MeshCollector collector;
    collector.setWeldingVertices(true);
    collector.setUseGlobalVertices(true);
    scene->accept(collector);

    const std::vector<osg::Vec3>& positions = collector.getVertices();
    const std::vector<unsigned int>& triangles = collector.getTriangles();
    if (positions.empty() || triangles.empty())
    {
        OSG_WARN << "[Nanite] No triangle found in the model" << std::endl;
        return 1;
    }

    // Attributes are only usable if they match the vertex count, otherwise the model has
    // inconsistent attributes across geometries and we simply ignore them
    std::vector<osg::Vec4>& normals = collector.getAttributes(osgVerse::MeshCollector::NormalAttr);
    std::vector<osg::Vec4>& uvs = collector.getAttributes(osgVerse::MeshCollector::UvAttr);
    bool hasNormals = (normals.size() == positions.size());
    bool hasUvs = (uvs.size() == positions.size());

    std::vector<ClusterVertex> vertices(positions.size());
    for (size_t i = 0; i < positions.size(); ++i)
    {
        ClusterVertex& v = vertices[i];
        v.px = positions[i][0]; v.py = positions[i][1]; v.pz = positions[i][2];
        if (hasNormals) { v.nx = normals[i][0]; v.ny = normals[i][1]; v.nz = normals[i][2]; }
        else { v.nx = 0.0f; v.ny = 0.0f; v.nz = 1.0f; }
        if (hasUvs) { v.tx = uvs[i][0]; v.ty = uvs[i][1]; }
        else { v.tx = 0.0f; v.ty = 0.0f; }
    }

    std::cout << "[Nanite] Input mesh: " << vertices.size() << " vertices, "
              << triangles.size() / 3 << " triangles; normals=" << (hasNormals ? "yes" : "no")
              << ", texcoords=" << (hasUvs ? "yes" : "no") << std::endl;

    // Bake the cluster DAG
    osg::ref_ptr<ClusterDAG> dag = new ClusterDAG;
    if (!dag->build(vertices, triangles, (size_t)maxTriangles, verbose)) return 1;

    // Prepare visualization
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    osg::ref_ptr<CutGeometry> cutGeometry;
    osg::ref_ptr<osg::Geometry> boundsGeometry;
    bool viewCut = false;

    if (mode == "clusters")
    {
        boundsGeometry = createClusterBoundsGeometry(dag.get(), maxLevel, segments);
        geode->addDrawable(boundsGeometry.get());
    }
    else
    {
        cutGeometry = new CutGeometry(dag.get(), vertices);
        cutGeometry->setColorByDepth(colorMode == "depth");
        geode->addDrawable(cutGeometry->getGeometry());
        viewCut = (cutLevel < 0);
    }

    osg::ref_ptr<osg::Program> program = new osg::Program;
    program->addShader(new osg::Shader(osg::Shader::VERTEX, vertCode));
    program->addShader(new osg::Shader(osg::Shader::FRAGMENT, fragCode));

    // moduleFlags = 0: only add necessary definitions (VERSE_* and osg_Vertex/osg_Color aliases),
    // no predefined shader modules are needed for such a simple color shader
    osgVerse::ShaderLibrary::instance()->updateProgram(*program, NULL, 0);

    osg::StateSet* ss = geode->getOrCreateStateSet();
    ss->setAttribute(program.get());
    ss->addUniform(new osg::Uniform("lightDir", osg::Vec3(0.2f, 0.3f, 0.9f)));
    ss->addUniform(new osg::Uniform("shadeMix", unlit ? 0.0f : 1.0f));
    if (wireframe)
        ss->setAttributeAndModes(new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK,
                                                      osg::PolygonMode::LINE));
    else
        ss->setAttributeAndModes(new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK,
                                                      osg::PolygonMode::FILL));

    // On-screen information
    osg::ref_ptr<osg::Geode> textGeode = new osg::Geode;
    osg::ref_ptr<osgText::Text> text = new osgText::Text;
    text->setFont(MISC_DIR + "LXGWFasmartGothic.ttf");
    text->setCharacterSize(20.0f);
    text->setPosition(osg::Vec3(10.0f, 40.0f, 0.0f));
    text->setColor(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    textGeode->addDrawable(text.get());

    osg::ref_ptr<osg::Camera> hudCamera = osgVerse::createHUDCamera(NULL, 1920, 1080);
    hudCamera->addChild(textGeode.get());

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(geode.get());
    root->addChild(hudCamera.get());

    // Update the cut and the on-screen information
    osg::ref_ptr<osg::Camera> camera;
    int frameCount = 0;
    auto refresh = [&](bool force)
    {
        std::ostringstream info;
        if (mode == "clusters")
        {
            info << "Nanite cluster view: " << dag->groups.size() << " groups, "
                 << dag->clusters.size() << " clusters total";
            std::cout << info.str() << std::endl;
        }
        else if (viewCut)
        {
            osg::Matrix projM = camera->getProjectionMatrix();
            float proj = float(projM(1, 1));
            float znear = float(projM(3, 2) / (projM(2, 2) - 1.0));
            int height = (camera->getViewport() != NULL) ? int(camera->getViewport()->height()) : 1080;
            float threshold = pixels / float(height);

            std::vector<unsigned int> cut;
            dag->collectCutByView(camera->getViewMatrix().getTrans(), proj, znear, threshold, cut);
            bool changed = cutGeometry->update(cut);
            if (force || changed)
            {
                info << "Nanite DAG cut (view-driven, " << pixels << "px): "
                     << cutGeometry->getClusterCount() << " clusters, "
                     << cutGeometry->getTriangleCount() << " triangles / "
                     << triangles.size() / 3 << " total";
                if (force) std::cout << info.str() << std::endl;
            }
        }
        else
        {
            std::vector<unsigned int> cut;
            dag->collectCutByLevel(cutLevel, cut);
            bool changed = cutGeometry->update(cut);
            if (force || changed)
            {
                info << "Nanite DAG cut (lod " << cutLevel << "): "
                     << cutGeometry->getClusterCount() << " clusters, "
                     << cutGeometry->getTriangleCount() << " triangles / "
                     << triangles.size() / 3 << " total";
                std::cout << info.str() << std::endl;
            }
        }
        if (!info.str().empty()) text->setText(info.str());
    };

    // Create the viewer
    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(screenNo);
    viewer.setRealizeOperation(new osgVerse::RealizeOperation);
    viewer.realize();

    camera = viewer.getCamera();
    if (viewCut)
    {   // set a sensible initial viewpoint so that the view-driven cut is meaningful at startup
        osg::BoundingBoxd bound;
        for (size_t i = 0; i < positions.size(); ++i) bound.expandBy(positions[i]);
        osgVerse::alignCameraToBox(camera.get(), bound, 1280, 720);
    }
    refresh(true);

    while (!viewer.done())
    {
        viewer.frame();
        if (viewCut && (interval <= 1 || (frameCount % interval) == 0))
            refresh(frameCount == 0);
        frameCount++;
    }
    return 0;
}
