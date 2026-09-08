#include <osg/io_utils>
#include <osg/ValueObject>
#include <osg/TriangleIndexFunctor>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgUtil/Tessellator>

#include <pipeline/Drawer2D.h>
#include <readerwriter/FeatureDefinition.h>
#include "mini_s57/iso8211.h"
#include "mini_s57/s57chart.h"
#include "mini_s57/s57catalog.h"

namespace osgS57
{
    // ---------------------------------------------------------------------------
    // S-57 pointers are stored as 40-bit binary "NAME" values:
    //     byte 0  : RCNM of the referenced record (110/120 node, 130 edge, 140 face)
    //     bytes1-4: RCID, little endian
    // ---------------------------------------------------------------------------
    struct SRef
    {
        int kind = 0;           // RCNM code of the referenced record
        long long rcid = 0;     // RCID inside that record class
        int topi = 0;           // VRPT topology indicator (1=begin,2=end ...)
        bool valid() const { return kind >= 100 && rcid >= 0; }
    };

    // One spatial (vector) record.
    struct VecRec
    {
        int kind = 0;
        long long rcid = 0;
        std::vector<S57::ChartPoint> pts;   // decoded coordinates (degrees)
        std::vector<SRef> links;       // VRPT pointer list
    };

    // One feature record.
    struct FeatureRec
    {
        long long rcid = 0;
        int objl = 0, prim = 0;
        std::vector<S57::AttributeInstance> attrs;
        std::vector<SRef> refs;        // FSPT pointer list
    };

    // Decode a 40-bit / wider binary NAME subfield into kind + RCID.
    bool decodeName(const I8211::Value& v, SRef& out)
    {
        out = SRef();
        if (!v.valid || v.type != I8211::SubfieldType::BinaryString) return false;
        static auto nib = [](unsigned char c) -> unsigned char {
            if (c >= '0' && c <= '9') return static_cast<unsigned char>(c - '0');
            if (c >= 'A' && c <= 'F') return static_cast<unsigned char>(c - 'A' + 10);
            if (c >= 'a' && c <= 'f') return static_cast<unsigned char>(c - 'a' + 10);
            return 0;
        };

        int bytes = static_cast<int>(v.hex.size()) / 2;
        if (bytes < 5) return false;   // need at least RCNM(1) + RCID(4)
        auto byteAt = [&](int i) -> int {
            unsigned char hi = nib(static_cast<unsigned char>(v.hex[2 * i]));
            unsigned char lo = nib(static_cast<unsigned char>(v.hex[2 * i + 1]));
            return (hi << 4) | lo;
        };

        out.kind = byteAt(0); out.rcid = 0;
        for (int i = 1; i < 5; ++i)
            out.rcid |= static_cast<long long>(byteAt(i)) << (8 * (i - 1));
        return out.valid();
    }

    // ---------------------------------------------------------------------------
    // local tangent-plane projection: metres around the cell centre
    // ---------------------------------------------------------------------------
    struct Projection
    {
        double lon0 = 0.0, lat0 = 0.0;
        double kx = 111194.93, ky = 111194.93;   // metres per degree

        void set(double lonMin, double latMin, double lonMax, double latMax)
        {
            lon0 = (lonMin + lonMax) * 0.5;
            lat0 = (latMin + latMax) * 0.5;
            double mPerDeg = 111194.9266445587;  // 6371000 * pi/180
            kx = mPerDeg * std::cos(lat0 * 3.141592653589793 / 180.0);
            ky = mPerDeg;
        }

        osg::Vec3 toXY(const S57::ChartPoint& p) const
        {
            return osg::Vec3(static_cast<float>((p.lon - lon0) * kx),
                             static_cast<float>((p.lat - lat0) * ky), 0.0f);
        }
    };

    // ---------------------------------------------------------------------------
    // chain-node rebuild: order a set of edges through their shared nodes
    // ---------------------------------------------------------------------------
    struct Chain { std::vector<osg::Vec3> pts; bool closed = false; };

    static osg::Vec3 nodeXY(const std::vector<VecRec>& recs, const Projection& pr, int nodeIdx)
    {
        if (nodeIdx >= 0 && nodeIdx < static_cast<int>(recs.size()) &&
            !recs[nodeIdx].pts.empty()) return pr.toXY(recs[nodeIdx].pts[0]);
        return osg::Vec3();
    }

    // Append one edge's vertices to a chain, oriented startNode -> endNode.
    static void appendEdge(const std::vector<VecRec>& recs,
                           const Projection& pr, int eIdx, int fromNode, int toNode,
                           std::vector<osg::Vec3>& out)
    {
        const VecRec& e = recs[eIdx];
        if (e.pts.empty())
        {
            // straight edge: geometry comes from its two nodes
            bool pushed = false;
            if (fromNode >= 0)
            {
                osg::Vec3 p = nodeXY(recs, pr, fromNode);
                if (out.empty() || out.back() != p) out.push_back(p);
                pushed = true;
            }
            if (toNode >= 0 && toNode != fromNode)
            {
                osg::Vec3 p = nodeXY(recs, pr, toNode);
                if (out.empty() || out.back() != p) out.push_back(p);
                pushed = true;
            }
            if (!pushed && !e.pts.empty()) out.push_back(pr.toXY(e.pts[0]));
            return;
        }

        bool forward = true;
        if (fromNode >= 0)
        {
            osg::Vec3 nf = nodeXY(recs, pr, fromNode);
            const S57::ChartPoint& f0 = e.pts.front();
            const S57::ChartPoint& f1 = e.pts.back();
            osg::Vec3 p0 = pr.toXY(f0), p1 = pr.toXY(f1);
            auto d2 = [](const osg::Vec3& a, const osg::Vec3& b) { return (a - b).length2(); };
            forward = d2(p0, nf) <= d2(p1, nf);
        }

        if (forward)
        {
            for (const S57::ChartPoint& p : e.pts)
            {
                osg::Vec3 v = pr.toXY(p);
                if (out.empty() || out.back() != v) out.push_back(v);
            }
        }
        else
        {
            for (auto it = e.pts.rbegin(); it != e.pts.rend(); ++it)
            {
                osg::Vec3 v = pr.toXY(*it);
                if (out.empty() || out.back() != v) out.push_back(v);
            }
        }
    }

    static std::vector<Chain> buildChains(const std::vector<VecRec>& recs,
                                          const Projection& pr, const std::vector<int>& edgeIdx)
    {
        std::vector<Chain> result;

        // node index -> edges incident at that node
        std::map<int, std::vector<int>> adj;
        std::vector<int> nodeA(edgeIdx.size(), -1), nodeB(edgeIdx.size(), -1);
        for (size_t k = 0; k < edgeIdx.size(); ++k)
        {
            const VecRec& e = recs[edgeIdx[k]]; int a = -1, b = -1;
            for (const SRef& r : e.links)
            {
                if (r.kind != 110 && r.kind != 120) continue;
                // find the vector record index of that node
                int idx = -1;
                for (size_t j = 0; j < recs.size(); ++j)
                    if (recs[j].kind == r.kind && recs[j].rcid == r.rcid) { idx = static_cast<int>(j); break; }
                if (idx < 0) continue;
                if (r.topi == 1) a = idx;
                else if (r.topi == 2) b = idx;
                else if (a < 0) a = idx;
                else if (b < 0) b = idx;
            }
            nodeA[k] = a; nodeB[k] = b;
            if (a >= 0) adj[a].push_back(static_cast<int>(k));
            if (b >= 0 && b != a) adj[b].push_back(static_cast<int>(k));
        }

        std::vector<bool> used(edgeIdx.size(), false);
        for (size_t s = 0; s < edgeIdx.size(); ++s)
        {
            if (used[s]) continue;
            int e0 = static_cast<int>(s);
            used[e0] = true;
            Chain chain;
            int startNode = nodeA[e0];
            if (startNode < 0) startNode = nodeB[e0];

            // prefer to begin at a degree-1 node (open polyline end)
            if (nodeA[e0] >= 0 && adj[nodeA[e0]].size() == 1 && adj[nodeB[e0]].size() != 1)
                startNode = nodeA[e0];
            else if (nodeB[e0] >= 0 && adj[nodeB[e0]].size() == 1)
                startNode = nodeB[e0];

            int curNode = startNode, curEdge = e0;
            int guard = 0, limit = static_cast<int>(edgeIdx.size()) * 2 + 8;
            while (guard++ < limit)
            {
                int a = nodeA[curEdge], b = nodeB[curEdge], other = -1;
                if (a == curNode) other = b;
                else if (b == curNode) other = a;
                appendEdge(recs, pr, edgeIdx[curEdge], curNode, other, chain.pts);
                curNode = other;
                if (other >= 0 && other == startNode)
                {
                    chain.closed = true;
                    break;
                }

                int nxt = -1;
                if (curNode >= 0)
                {
                    auto it = adj.find(curNode);
                    if (it != adj.end())
                        for (int cand : it->second)
                            if (!used[cand]) { nxt = cand; break; }
                }
                if (nxt < 0) break;   // open end of the chain reached
                used[nxt] = true; curEdge = nxt;
            }

            if (!chain.pts.empty())
            {
                // drop the duplicated closing vertex that the topology may imply
                if (chain.pts.size() > 2 && chain.pts.front() == chain.pts.back())
                    chain.pts.pop_back();
                result.push_back(chain);
            }
        }
        return result;
    }

    // ---------------------------------------------------------------------------
    // colour helpers (kept on the Feature as user values so that readNode can
    // reproduce the per-object-class colours)
    // ---------------------------------------------------------------------------
    static void setFeatureColor(osgVerse::Feature* f, const osg::Vec4& c)
    {
        f->setUserValue("_ColorR", (double)c.r());
        f->setUserValue("_ColorG", (double)c.g());
        f->setUserValue("_ColorB", (double)c.b());
        f->setUserValue("_ColorA", (double)c.a());
    }

    static osg::Vec4 featureColor(osgVerse::Feature* f,
        const osg::Vec4& def = osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f))
    {
        double r = def.r(), g = def.g(), b = def.b(), a = def.a();
        f->getUserValue("_ColorR", r); f->getUserValue("_ColorG", g);
        f->getUserValue("_ColorB", b); f->getUserValue("_ColorA", a);
        return osg::Vec4((float)r, (float)g, (float)b, (float)a);
    }

    // ---------------------------------------------------------------------------
    // convert all spatial/feature records into a FeatureCollection
    // ---------------------------------------------------------------------------
    struct ChartData
    {
        std::vector<VecRec> recs;
        std::vector<FeatureRec> features;
        std::map<std::pair<int, long long>, int> spatialIndex; // (kind,rcid)->rec

        int findSpatial(int kind, long long rcid) const
        {
            auto it = spatialIndex.find({ kind, rcid });
            if (it != spatialIndex.end()) return it->second;
            // tolerate producers that mix node classes
            if (kind == 110 || kind == 120)
                for (const VecRec& v : recs)
                    if (v.rcid == rcid && (v.kind == 110 || v.kind == 120))
                        return static_cast<int>(&v - &recs[0]);
            return -1;
        }
    };

    static osgVerse::FeatureCollection* buildChart(const I8211::Iso8211Reader& rdr)
    {
        std::size_t n = rdr.recordCount();
        if (n == 0) return NULL;

        // dataset parameters (coordinate scale)
        S57::DatasetParameters dspm;
        for (std::size_t i = 0; i < n; ++i)
        {
            const auto& rec = rdr.record(i);
            if (rec.find("DSPM")) dspm = S57::readDspm(rdr, rec);
        }

        ChartData chart;
        double lonMin = 1e9, lonMax = -1e9, latMin = 1e9, latMax = -1e9;
        bool haveGeo = false;

        // ---- pass 1: vector records -------------------------------------------
        for (std::size_t i = 0; i < n; ++i)
        {
            const auto& rec = rdr.record(i);
            if (!rec.find("VRID")) continue;

            bool ok = false;
            int kind = static_cast<int>(rdr.intSubfield(rec, "VRID", 0, "RCNM", 0, &ok));
            if (!ok || kind < 110 || kind > 140) continue;

            VecRec vr;
            vr.kind = kind;
            vr.rcid = rdr.intSubfield(rec, "VRID", 0, "RCID", 0, &ok);
            vr.pts = S57::spatialCoordinates(rdr, rec, dspm);

            // VRPT pointer list (nodes for edges, edges for faces)
            if (rec.find("VRPT"))
            {
                const I8211::FieldDef* f = rec.find("VRPT");
                int reps = rdr.repeatCount(*f);
                for (int k = 0; k < reps; ++k)
                {
                    SRef r;
                    I8211::Value name = rdr.valueSubfield(rec, "VRPT", 0, "NAME", k);
                    if (!decodeName(name, r)) continue;
                    r.topi = static_cast<int>(rdr.intSubfield(rec, "VRPT", 0, "TOPI", k, &ok));
                    vr.links.push_back(r);
                }
            }

            int idx = static_cast<int>(chart.recs.size());
            chart.recs.push_back(vr);
            chart.spatialIndex[{kind, vr.rcid}] = idx;
            for (const S57::ChartPoint& p : vr.pts)
            {
                lonMin = std::min(lonMin, p.lon); lonMax = std::max(lonMax, p.lon);
                latMin = std::min(latMin, p.lat); latMax = std::max(latMax, p.lat);
                haveGeo = true;
            }
        }

        // ---- pass 2: feature records ------------------------------------------
        for (std::size_t i = 0; i < n; ++i)
        {
            const auto& rec = rdr.record(i);
            if (!rec.find("FRID")) continue;

            FeatureRec fr;
            bool ok = false;
            fr.rcid = rdr.intSubfield(rec, "FRID", 0, "RCID", 0, &ok);
            fr.prim = static_cast<int>(rdr.intSubfield(rec, "FRID", 0, "PRIM", 0, &ok));
            fr.objl = static_cast<int>(rdr.intSubfield(rec, "FRID", 0, "OBJL", 0, &ok));
            fr.attrs = S57::readAttributes(rdr, rec);
            if (rec.find("FSPT"))
            {
                const I8211::FieldDef* f = rec.find("FSPT");
                int reps = rdr.repeatCount(*f);
                for (int k = 0; k < reps; ++k)
                {
                    SRef r;
                    I8211::Value name = rdr.valueSubfield(rec, "FSPT", 0, "NAME", k);
                    if (decodeName(name, r))
                    {
                        r.topi = static_cast<int>(rdr.intSubfield(rec, "FSPT", 0, "TOPI", k, &ok));
                        fr.refs.push_back(r);
                    }
                }
            }
            chart.features.push_back(fr);
        }
        if (chart.features.empty()) return NULL;

        // ---- projection centre ------------------------------------------------
        Projection pr;
        if (haveGeo) pr.set(lonMin, latMin, lonMax, latMax);

        osgVerse::FeatureCollection* collection = new osgVerse::FeatureCollection;
        for (const FeatureRec& fr : chart.features)
        {
            S57::ObjectClassInfo oi = S57::objectClass(fr.objl);
            std::string acr = oi.valid ? oi.acronym : "UNK" + std::to_string(fr.objl);
            std::string cls = oi.valid ? oi.name : "(unknown object class)";
            std::string fname = acr + "_" + std::to_string(fr.rcid);

            // simple deterministic colour from the object class code
            static const osg::Vec4 palette[] = {
                {0.70f, 0.85f, 0.95f, 1.0f}, {0.95f, 0.75f, 0.55f, 1.0f},
                {0.75f, 0.90f, 0.70f, 1.0f}, {0.90f, 0.80f, 0.70f, 1.0f},
                {0.75f, 0.75f, 0.95f, 1.0f}, {0.95f, 0.90f, 0.60f, 1.0f},
                {0.80f, 0.70f, 0.90f, 1.0f}, {0.70f, 0.90f, 0.90f, 1.0f}
            };
            osg::Vec4 color = palette[(unsigned int)(fr.objl) %
                                      (sizeof(palette) / sizeof(palette[0]))];

            // resolve the referenced spatial records
            std::vector<int> nodeRefs, edgeRefs, faceRefs;
            for (const SRef& r : fr.refs)
            {
                int idx = chart.findSpatial(r.kind, r.rcid);
                if (idx < 0) continue;
                if (r.kind == 110 || r.kind == 120) nodeRefs.push_back(idx);
                else if (r.kind == 130) edgeRefs.push_back(idx);
                else if (r.kind == 140) faceRefs.push_back(idx);
            }

            auto fillMeta = [&](osgVerse::Feature* ft)
            {
                ft->setName(fname);
                setFeatureColor(ft, color);
                ft->setUserValue("ObjectClass", cls);
                ft->setUserValue("ObjectAcronym", acr);
                ft->setUserValue("ObjectCode", (double)fr.objl);
                ft->setUserValue("RCID", (double)fr.rcid);
                ft->setUserValue("Primitive", (double)fr.prim);
                for (const S57::AttributeInstance& a : fr.attrs)
                {
                    S57::AttributeInfo ai = S57::attribute(static_cast<int>(a.code));
                    std::string key = ai.valid ? ai.acronym : "A" + std::to_string(a.code);
                    if (!key.empty()) ft->setUserValue(key, a.value);
                }
            };

            bool made = false;
            if (fr.prim == 1)   // ---------------- point ----------------
            {
                osgVerse::Feature* feature = new osgVerse::Feature;
                fillMeta(feature); feature->setType(GL_POINTS);

                for (int idx : nodeRefs)
                {
                    const VecRec& v = chart.recs[idx];
                    if (v.pts.empty()) continue;
                    feature->addPoint(pr.toXY(v.pts[0]));
                    made = true;
                }
                for (int idx : edgeRefs)  // fallback: edge-vertex point objects
                {
                    const VecRec& v = chart.recs[idx];
                    if (!v.pts.empty())
                        { feature->addPoint(pr.toXY(v.pts[0])); made = true; }
                }
                if (made) collection->push_back(feature);
            }
            else if (fr.prim == 2)  // ---------------- line ----------------
            {
                std::vector<int> ids = edgeRefs;
                if (ids.empty() && !faceRefs.empty())
                {
                    for (int fi : faceRefs)
                        for (const SRef& l : chart.recs[fi].links)
                            if (l.kind == 130)
                            {
                                int ei = chart.findSpatial(130, l.rcid);
                                if (ei >= 0) ids.push_back(ei);
                            }
                }

                std::vector<Chain> chains = buildChains(chart.recs, pr, ids);
                for (const Chain& c : chains)
                {
                    if (c.pts.size() < 2) continue;
                    osgVerse::Feature* cf = new osgVerse::Feature;   // one feature per chain
                    fillMeta(cf); cf->setType(c.closed ? GL_LINE_LOOP : GL_LINE_STRIP);

                    osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
                    va->insert(va->end(), c.pts.begin(), c.pts.end());
                    cf->addPoints(va.get()); collection->push_back(cf);
                }
            }
            else  // prim == 3 (area) or coverage
            {
                std::vector<int> ids = edgeRefs;
                if (ids.empty() && !faceRefs.empty())
                {
                    for (int fi : faceRefs)
                        for (const SRef& l : chart.recs[fi].links)
                            if (l.kind == 130)
                            {
                                int ei = chart.findSpatial(130, l.rcid);
                                if (ei >= 0) ids.push_back(ei);
                            }
                }

                std::vector<Chain> rings = buildChains(chart.recs, pr, ids);
                if (!rings.empty())
                {
#if false
                    osgVerse::Feature* feature = new osgVerse::Feature;
                    fillMeta(feature); feature->setType(GL_POLYGON);
                    for (const Chain& r : rings)
                    {
                        if (r.pts.size() < 3) continue;
                        // closed ring: tessellation works on an open contour list
                        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
                        va->insert(va->end(), r.pts.begin(), r.pts.end());
                        if (va->size() > 2 && (*va)[0] == (*va)[va->size() - 1]) va->pop_back();
                        if (va->size() >= 3) feature->addPoints(va.get());
                    }
                    made = !feature->getPointList().empty();
                    if (made) collection->push_back(feature);
#else
                    for (const Chain& r : rings)
                    {
                        if (r.pts.size() < 3) continue;
                        osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
                        va->insert(va->end(), r.pts.begin(), r.pts.end());
                        if (va->size() > 2 && (*va)[0] == (*va)[va->size() - 1]) va->pop_back();

                        osgVerse::Feature* feature = new osgVerse::Feature;
                        fillMeta(feature); feature->setType(GL_LINE_LOOP);
                        feature->addPoints(va.get()); collection->push_back(feature);
                    }
#endif
                }
            }
        }
        return collection;
    }
}  // namespace

class ReaderWriterS57 : public osgDB::ReaderWriter
{
public:
    ReaderWriterS57()
    {
        supportsExtension("verse_s57", "osgVerse pseudo-loader");
        supportsExtension("000", "IHO S-57 ENC base data set (ISO8211)");
        supportsExtension("s57", "IHO S-57 ENC data set (alias)");
        supportsOption("IncludeFeatures", "Add FeatureCollection as UserData of the result Geometry/Image. Default: 0");
        supportsOption("ImageWidth", "Image resolution. Default: 512");
        supportsOption("ImageHeight", "Image resolution. Default: 512");
    }

    virtual const char* className() const
    {
        return "[osgVerse] S-57 electronic navigational chart reader";
    }

    virtual ReadResult readObject(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        if (fileName.empty()) return ReadResult::FILE_NOT_HANDLED;

        osg::ref_ptr<Options> lOptions = options ?
            static_cast<Options*>(options->clone(osg::CopyOp::SHALLOW_COPY)) : new Options;
        lOptions->setPluginStringData("STREAM_FILENAME", osgDB::getSimpleFileName(fileName));

        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_HANDLED;;
        return readObject(in, lOptions.get());
    }

    virtual ReadResult readNode(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        if (fileName.empty()) return ReadResult::FILE_NOT_HANDLED;

        osg::ref_ptr<Options> lOptions = options ?
            static_cast<Options*>(options->clone(osg::CopyOp::SHALLOW_COPY)) : new Options;
        lOptions->setPluginStringData("STREAM_FILENAME", osgDB::getSimpleFileName(fileName));

        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_HANDLED;;
        return readNode(in, lOptions.get());
    }

    virtual ReadResult readImage(const std::string& path, const Options* options) const
    {
        std::string ext; std::string fileName = getRealFileName(path, ext);
        if (fileName.empty()) return ReadResult::FILE_NOT_HANDLED;

        osg::ref_ptr<Options> lOptions = options ?
            static_cast<Options*>(options->clone(osg::CopyOp::SHALLOW_COPY)) : new Options;
        lOptions->setPluginStringData("STREAM_FILENAME", osgDB::getSimpleFileName(fileName));

        std::ifstream in(fileName, std::ios::in | std::ios::binary);
        if (!in) return ReadResult::FILE_NOT_HANDLED;
        return readImage(in, lOptions.get());
    }

    virtual ReadResult readObject(std::istream& fin, const Options* options) const
    {
        std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(fin)),
                                    std::istreambuf_iterator<char>());
        if (buffer.empty()) return ReadResult::ERROR_IN_READING_FILE;

        osgS57::I8211::Iso8211Reader rdr;
        osgS57::I8211::ParseResult pr = rdr.openMemory(buffer);
        if (!pr.ok)
        {
            OSG_WARN << "[ReaderWriterS57] " << pr.message << std::endl;
            return ReadResult::ERROR_IN_READING_FILE;
        }

        osg::ref_ptr<osgVerse::FeatureCollection> collection = osgS57::buildChart(rdr);
        if (!collection) return ReadResult::ERROR_IN_READING_FILE;
        return collection.get();
    }

    virtual ReadResult readNode(std::istream& fin, const Options* options) const
    {
        std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(fin)),
                             std::istreambuf_iterator<char>());
        if (buffer.empty()) return ReadResult::ERROR_IN_READING_FILE;

        osgS57::I8211::Iso8211Reader rdr;
        osgS57::I8211::ParseResult pr = rdr.openMemory(buffer);
        if (!pr.ok)
        {
            OSG_WARN << "[ReaderWriterS57] " << pr.message << std::endl;
            return ReadResult::ERROR_IN_READING_FILE;
        }

        osg::ref_ptr<osgVerse::FeatureCollection> collection = osgS57::buildChart(rdr);
        if (!collection) return ReadResult::ERROR_IN_READING_FILE;

        osg::ref_ptr<osg::Geode> geode = new osg::Geode; osg::ref_ptr<osg::Geometry> geom;
        for (size_t i = 0; i < collection->features.size(); ++i)
        {
            if (!geom)
            {
                geom = new osg::Geometry; geode->addDrawable(geom.get());
                geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
            }

            osgVerse::Feature* feature = collection->features[i];
            osgVerse::addFeatureToGeometry(*feature, geom.get(), true);
            if (geom->getNumPrimitiveSets() > 1024) geom = NULL;
        }

        if (options)
        {
            int toInc = atoi(options->getPluginStringData("IncludeFeatures").c_str());
            if (toInc > 0) geode->setUserData(collection.get());
        }
        return geode.get();
    }

    virtual ReadResult readImage(std::istream& fin, const Options* options) const
    {
        std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(fin)),
                                    std::istreambuf_iterator<char>());
        if (buffer.empty()) return ReadResult::ERROR_IN_READING_FILE;

        osgS57::I8211::Iso8211Reader rdr;
        osgS57::I8211::ParseResult pr = rdr.openMemory(buffer);
        if (!pr.ok)
        {
            OSG_WARN << "[ReaderWriterS57] " << pr.message << std::endl;
            return ReadResult::ERROR_IN_READING_FILE;
        }

        osg::ref_ptr<osgVerse::FeatureCollection> fc = osgS57::buildChart(rdr);
        if (!fc) return ReadResult::ERROR_IN_READING_FILE;

        std::string wStr = options ? options->getPluginStringData("ImageWidth") : "512";
        std::string hStr = options ? options->getPluginStringData("ImageHeight") : "512";
        int w = atoi(wStr.c_str()), h = atoi(hStr.c_str()); if (w < 1) w = 512; if (h < 1) h = 512;

        osg::ref_ptr<osgVerse::Drawer2D> drawer = new osgVerse::Drawer2D;
        if (options)
        {
            int toInc = atoi(options->getPluginStringData("IncludeFeatures").c_str());
            if (toInc > 0) drawer->setUserData(fc.get());
        }
        drawer->allocateImage(w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE);
        drawer->setPixelBufferObject(new osg::PixelBufferObject(drawer.get()));
        drawer->start(false); drawer->fillBackground(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));

        const osg::BoundingBox& bb = fc->bound;
        osg::Vec2 off(-bb.xMin(), -bb.yMin()),
            sc((float)w / (bb.xMax() - bb.xMin()), (float)h / (bb.yMax() - bb.yMin()));
        osgVerse::DrawerStyleData fillStyle(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f), true);
        for (size_t i = 0; i < fc->features.size(); ++i)
        {
            osgVerse::Feature* feature = fc->features[i];
            osgVerse::drawFeatureToImage(*feature, drawer.get(), off, sc, &fillStyle);
        }
        drawer->finish(); return drawer.get();
    }

protected:
    std::string getRealFileName(const std::string& path, std::string& ext) const
    {
        std::string fileName(path); ext = osgDB::getLowerCaseFileExtension(path);
        if (!acceptsExtension(ext)) return "";

        bool usePseudo = (ext == "verse_s57");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(path);
            ext = osgDB::getFileExtension(fileName);
        }
        return fileName;
    }
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_s57, ReaderWriterS57)
