// ============================================================================
// osgS57/s57catalog.h -- numeric-code -> mnemonic lookups.
//
// S-57 records store object classes and attributes as numeric codes (FRID
// "OBJL", ATTF "ATTL" ...).  These helpers resolve the codes to the standard
// mnemonics / long names from the IHO S-57 object catalogue.
// ============================================================================
#pragma once

#include <cstdint>
#include <string>

namespace osgS57 {
namespace S57 {

struct ObjectClassInfo {
    bool valid = false;
    int code = 0;
    std::string acronym;
    std::string name;
};

struct AttributeInfo {
    bool valid = false;
    int code = 0;
    std::string acronym;
    std::string name;
};

// Resolve an object class code (e.g. FRID.OBJL).
ObjectClassInfo objectClass(int code);

// Resolve an attribute code (e.g. the ATTL subfield of an ATTF instance).
AttributeInfo attribute(int code);

// Attribute "type" helper strings for the report (not a full enumeration
// catalogue -- values are left as the raw text in stage-1).
const char* primitiveName(int prim);   // FRID.PRIM: 1 point 2 line 3 area 4?
const char* recordName(int rcnm);      // RCNM code -> short record type name

}  // namespace S57
}  // namespace osgS57
