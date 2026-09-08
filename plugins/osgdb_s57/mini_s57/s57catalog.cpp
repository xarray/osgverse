// ============================================================================
// osgS57/src/s57catalog.cpp
// ============================================================================
#include "s57catalog.h"

// Generated table (numeric codes from the IHO S-57 object catalogue).
#include "s57catalog_tables.inc"

#include <cstring>

namespace osgS57 {
namespace S57 {

namespace {
template <typename T, int N>
const T* find(const T (&tab)[N], int code) {
    for (const auto& e : tab) {
        if (e.code == code) return &e;
    }
    return nullptr;
}
}  // namespace

ObjectClassInfo objectClass(int code) {
    ObjectClassInfo out;
    const auto* e = find(k_objclass_table, code);
    if (e) {
        out.valid = true;
        out.code = code;
        out.acronym = e->acronym;
        out.name = e->name;
    }
    return out;
}

AttributeInfo attribute(int code) {
    AttributeInfo out;
    const auto* e = find(k_attr_table, code);
    if (e) {
        out.valid = true;
        out.code = code;
        out.acronym = e->acronym;
        out.name = e->name;
    }
    return out;
}

const char* primitiveName(int prim) {
    switch (prim) {
        case 1: return "Point";
        case 2: return "Line";
        case 3: return "Area";
        case 4: return "Coverage";
        default: return "?";
    }
}

const char* recordName(int rcnm) {
    switch (rcnm) {
        case 10: return "DataSetGeneralInformation(DSID)";
        case 15: return "CatalogueDirectory(CATD)";
        case 20: return "DataSetParameter(DSPM)";
        case 25: return "DataSetGeoreference(D SGR)";
        case 30: return "DataSetAccuracy(D SAC)";
        case 35: return "DataSetHistory(D SHD)";
        case 40: return "DataDictionaryDefinition(DDDF)";
        case 45: return "DataDictionaryDefinitionDomain(DDDR)";
        case 100: return "FeatureRecord";
        case 110: return "VectorRecord-IsolatedNode";
        case 120: return "VectorRecord-ConnectedNode";
        case 130: return "VectorRecord-Edge";
        case 140: return "VectorRecord-Face";
        default: return "?";
    }
}

}  // namespace S57
}  // namespace osgS57
