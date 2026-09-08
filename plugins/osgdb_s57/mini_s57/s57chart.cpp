// ============================================================================
// osgS57/src/s57chart.cpp
// ============================================================================
#include "s57chart.h"

#include <cctype>

namespace osgS57 {
namespace S57 {

namespace {
long long optInt(const I8211::Iso8211Reader& rdr, const I8211::RecordDef& rec,
                 const char* tag, const char* sub, bool& ok) {
    return rdr.intSubfield(rec, tag, 0, sub, 0, &ok);
}
std::string optStr(const I8211::Iso8211Reader& rdr,
                   const I8211::RecordDef& rec, const char* tag,
                   const char* sub, bool& ok) {
    return rdr.stringSubfield(rec, tag, 0, sub, 0, &ok);
}
}  // namespace

DatasetParameters readDspm(const I8211::Iso8211Reader& rdr,
                           const I8211::RecordDef& rec) {
    DatasetParameters p;
    bool ok = false;
    optInt(rdr, rec, "DSPM", "RCNM", ok);
    if (!ok) return p;  // no DSPM field

    p.present = true;
    p.rcnm = rdr.intSubfield(rec, "DSPM", 0, "RCNM", 0, &ok);
    p.rcid = rdr.intSubfield(rec, "DSPM", 0, "RCID", 0, &ok);
    p.hdat = rdr.intSubfield(rec, "DSPM", 0, "HDAT", 0, &ok);
    p.vdat = rdr.intSubfield(rec, "DSPM", 0, "VDAT", 0, &ok);
    p.sdat = rdr.intSubfield(rec, "DSPM", 0, "SDAT", 0, &ok);
    p.cscl = rdr.intSubfield(rec, "DSPM", 0, "CSCL", 0, &ok);
    p.duni = rdr.intSubfield(rec, "DSPM", 0, "DUNI", 0, &ok);
    p.huni = rdr.intSubfield(rec, "DSPM", 0, "HUNI", 0, &ok);
    p.puni = rdr.intSubfield(rec, "DSPM", 0, "PUNI", 0, &ok);
    p.coun = rdr.intSubfield(rec, "DSPM", 0, "COUN", 0, &ok);
    p.comf = rdr.intSubfield(rec, "DSPM", 0, "COMF", 0, &ok);
    p.somf = rdr.intSubfield(rec, "DSPM", 0, "SOMF", 0, &ok);
    p.comt = rdr.stringSubfield(rec, "DSPM", 0, "COMT", 0, &ok);
    return p;
}

std::vector<ChartPoint> spatialCoordinates(const I8211::Iso8211Reader& rdr,
                                           const I8211::RecordDef& rec,
                                           const DatasetParameters& dspm) {
    std::vector<ChartPoint> pts;
    double sx = dspm.positionScale();
    double sd = dspm.soundingScale();

    for (const char* tag : {"SG3D", "SG2D"}) {
        const I8211::FieldDefn* defn = rdr.fieldDefns().empty() ? nullptr
                                   : rdr.findFieldDefn(tag);
        if (!defn) continue;
        const I8211::FieldDef* fld = rec.find(tag);
        if (!fld) continue;

        // is the group actually laid out as YCOO/XCOO(/VE3D)?
        bool hasX = defn->findSubfield("XCOO") != nullptr;
        bool hasY = defn->findSubfield("YCOO") != nullptr;
        bool hasZ = defn->findSubfield("VE3D") != nullptr;
        if (!hasX || !hasY) continue;

        int reps = rdr.repeatCount(*fld);
        for (int k = 0; k < reps; ++k) {
            ChartPoint pt;
            bool ok = false;
            long long x = rdr.intSubfield(rec, tag, 0, "XCOO", k, &ok);
            if (!ok) continue;
            long long y = rdr.intSubfield(rec, tag, 0, "YCOO", k, &ok);
            if (!ok) continue;
            pt.lon = static_cast<double>(x) / sx;
            pt.lat = static_cast<double>(y) / sx;
            if (hasZ) {
                pt.depth = static_cast<double>(
                               rdr.intSubfield(rec, tag, 0, "VE3D", k, &ok)) /
                           sd;
                pt.hasDepth = ok;
            }
            pt.valid = true;
            pts.push_back(pt);
        }
        if (!pts.empty()) break;  // only one coordinate field per record
    }
    return pts;
}

std::vector<AttributeInstance> readAttributes(
    const I8211::Iso8211Reader& rdr, const I8211::RecordDef& rec) {
    std::vector<AttributeInstance> out;
    for (const char* tag : {"ATTF", "NATF"}) {
        const I8211::FieldDefn* defn = rdr.findFieldDefn(tag);
        if (!defn) continue;
        const I8211::FieldDef* fld = rec.find(tag);
        if (!fld || !defn->findSubfield("ATTL") || !defn->findSubfield("ATVL"))
            continue;
        int reps = rdr.repeatCount(*fld);
        for (int k = 0; k < reps; ++k) {
            bool ok = false;
            long long code = rdr.intSubfield(rec, tag, 0, "ATTL", k, &ok);
            std::string val = rdr.stringSubfield(rec, tag, 0, "ATVL", k, &ok);
            AttributeInstance a;
            a.code = code;
            a.value = val;
            out.push_back(a);
        }
    }
    return out;
}

int recordRcnm(const I8211::Iso8211Reader& rdr,
               const I8211::RecordDef& rec) {
    // features and spatial vectors carry RCNM in FRID/VRID
    for (const char* tag : {"FRID", "VRID"}) {
        const I8211::FieldDefn* defn = rdr.findFieldDefn(tag);
        if (!defn) continue;
        const I8211::FieldDef* fld = rec.find(tag);
        if (!fld || !defn->findSubfield("RCNM")) continue;
        bool ok = false;
        int v = static_cast<int>(
            rdr.intSubfield(rec, tag, 0, "RCNM", 0, &ok));
        if (ok) return v;
    }
    // general exchange records (DSID/DSPM ...) identify themselves by tag
    if (rec.find("DSID")) return RCNM_DSID;
    if (rec.find("DSPM")) return RCNM_DSPM;
    if (rec.find("CATD")) return RCNM_CATD;
    return 0;
}

}  // namespace S57
}  // namespace osgS57
