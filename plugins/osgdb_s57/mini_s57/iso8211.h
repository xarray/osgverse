// ============================================================================
// osgS57/iso8211.h -- minimal, self contained ISO/IEC 8211 (DDF) reader.
//
// Only the subset of ISO 8211 that appears in IHO S-57 ENC data sets is
// needed by this project, but the implementation below follows the layout
// used by the S-57 test data sets and the well known open implementations
// (see the MIT licensed GDAL iso8211 module for the same format layout).
//
// Terminology used here:
//   DDR   -- Data Descriptive Record, the first record of the file.  It
//            carries one "field definition" per ISO 8211 field tag that may
//            appear in the data records that follow (name / subfield list /
//            format controls).
//   DR    -- Data Record, everything after the DDR.  Fields are tagged.
//
// A subfield format control is e.g. "b11" (binary unsigned int, 1 byte),
// "A" (variable char string), "A(8)" (fixed 8 char string), "R(4)" (binary
// IEEE float, 4 bytes), "b12" (2 byte int), "2A" (repeat prefix), ...
//
// The reader is intentionally dumb: it exposes the raw decoded subfield
// values; interpreting S-57 semantics lives one level up.
// ============================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace osgS57 {

namespace I8211 {

// Subfield data categories (mirror the common DDF types).
enum class SubfieldType {
    Integer,        // decoded as int64
    Real,           // decoded as double
    Text,           // character string
    BinaryString,   // opaque binary payload (hex printed)
    Unknown,
};

// Numeric binary encodings used by ISO8211 'b'/'B' formats.
enum class BinaryFormat {
    None = 0,
    UnsignedInt = 1,
    SignedInt = 2,
    FPReal = 3,       // (unused in S57)
    FloatReal = 4,    // IEEE float / double
    FloatComplex = 5, // (unused in S57)
};

// ---------------------------------------------------------------------------
// SubfieldDefn -- one column of a field (from the DDR "subfield list" + a
// format control item).
// ---------------------------------------------------------------------------
struct SubfieldDefn {
    std::string name;    // mnemonic, e.g. "RCNM", "COMF", "YCOO"
    std::string format;  // raw format string, e.g. "b11", "A", "A(8)", "R(4)"

    SubfieldType type = SubfieldType::Unknown;
    BinaryFormat binary = BinaryFormat::None;

    bool variable = true;  // true => scan for terminator; false => fixed width
    int width = 0;         // fixed byte width when !variable
    char delimiter = 31;   // 0x1F unit terminator used to end variable fields
    bool bigEndian = false; // byte order for binary payloads
};

// ---------------------------------------------------------------------------
// FieldDefn -- describes one tagged field.  Created while parsing the DDR.
// ---------------------------------------------------------------------------
struct FieldDefn {
    std::string tag;          // e.g. "DSID", "VRID", "SG3D"
    std::string fieldName;    // long descriptive name from the DDR
    std::string arrayDescr;   // e.g. "*YCOO!XCOO"
    std::string formatControls; // e.g. "(b11,b14,2b11,3A,...)"

    int dataStructCode = 0;   // 0=elementary 1=vector 2=array 3=concatenated
    int dataTypeCode = 0;

    bool repeating = false;   // field contains several instances of its subfields
    int fixedWidth = 0;       // total width in bytes when all subfields fixed

    std::vector<SubfieldDefn> subfields;

    bool hasSubfields() const { return !subfields.empty(); }
    const SubfieldDefn* findSubfield(const std::string& name) const;
};

// ---------------------------------------------------------------------------
// FieldDef -- raw field instance inside one data record.
// ---------------------------------------------------------------------------
struct FieldDef {
    const FieldDefn* defn = nullptr;
    const uint8_t* data = nullptr;  // points into the file image
    std::size_t size = 0;
};

// ---------------------------------------------------------------------------
// RecordDef -- one data record (a set of tagged fields).
// ---------------------------------------------------------------------------
struct RecordDef {
    std::vector<FieldDef> fields;

    const FieldDef* find(const std::string& tag, int instance = 0) const;
    const FieldDefn* fieldDefn(const std::string& tag, int instance = 0) const;
};

// ---------------------------------------------------------------------------
// A decoded scalar.  value text is prepared according to the subfield type.
// ---------------------------------------------------------------------------
struct Value {
    bool valid = false;
    SubfieldType type = SubfieldType::Unknown;

    long long i = 0;    // Integer
    double r = 0.0;     // Real
    std::string s;      // Text or decimal rendering of numeric values
    std::string hex;    // BinaryString rendering

    // Decoded string used for reporting: text for text fields, otherwise the
    // numeric value.
    std::string display() const;
};

// Return code of the parsing functions.
struct ParseResult {
    bool ok = false;
    std::string message;
};

// ---------------------------------------------------------------------------
// Iso8211Reader -- parses a whole .000 / .001 / .003 style ISO8211 stream
// into DDR field definitions + an index of data records.
// ---------------------------------------------------------------------------
class Iso8211Reader {
public:
    Iso8211Reader() = default;
    explicit Iso8211Reader(const std::string& path) { open(path); }

    // Open and fully index the file.
    ParseResult open(const std::string& path);
    // Open from an in-memory image (used by ReaderWriter-style callers that
    // receive an std::istream instead of a file name).
    ParseResult openMemory(const std::vector<uint8_t>& data);
    bool isOpen() const { return m_open; }
    const std::string& error() const { return m_error; }

    // DDR information
    const std::vector<FieldDefn>& fieldDefns() const { return m_fieldDefns; }
    const FieldDefn* findFieldDefn(const std::string& tag) const;
    int ddrFieldControlLength() const { return m_ddrFieldControlLength; }

    // Data records
    std::size_t recordCount() const { return m_records.size(); }
    const RecordDef& record(std::size_t i) const { return m_records[i]; }

    // Convenience accessors that decode a subfield.
    // iField / iInstance index repeating instances of the field / subfield.
    long long intSubfield(const RecordDef& rec, const std::string& field,
                          int iField, const std::string& subfield,
                          int iInstance, bool* ok = nullptr) const;
    double doubleSubfield(const RecordDef& rec, const std::string& field,
                          int iField, const std::string& subfield,
                          int iInstance, bool* ok = nullptr) const;
    std::string stringSubfield(const RecordDef& rec, const std::string& field,
                               int iField, const std::string& subfield,
                               int iInstance, bool* ok = nullptr) const;
    // Decode one subfield occurrence and return the full typed value
    // (access to the raw bytes of opaque / binary-string subfields).
    Value valueSubfield(const RecordDef& rec, const std::string& field,
                        int iField, const std::string& subfield,
                        int iInstance) const;

    // Decode every field of a record into printable subfield rows.
    // Returns one row per subfield instance * subfield occurrence.
    struct Row {
        int fieldIndex;   // index of the field in the record
        std::string tag;  // field tag
        int repeat;       // instance ordinal within the repeated field
        std::string subfield;
        Value value;
    };
    std::vector<Row> decodeRecord(const RecordDef& rec) const;

    // Number of times the subfields repeat in a field (1 if not repeating).
    int repeatCount(const FieldDef& f) const;

private:
    ParseResult parseLoaded();
    bool parseDdr(const std::vector<uint8_t>& buf, std::size_t& pos,
                  const uint8_t* base);
    bool parseRecord(const std::vector<uint8_t>& buf, std::size_t& pos,
                     const uint8_t* base, RecordDef& out);
    bool parseFieldDefnArea(const uint8_t* p, std::size_t size,
                            FieldDefn& out) const;
    void buildSubfields(FieldDefn& d) const;
    bool applyFormats(FieldDefn& d) const;

    // decode helpers
    Value readValue(const SubfieldDefn& sf, const uint8_t* p,
                    std::size_t remain) const;
    int subfieldLength(const SubfieldDefn& sf, const uint8_t* p,
                       std::size_t remain, std::size_t& consumed) const;

    bool m_open = false;
    std::string m_error;
    std::vector<uint8_t> m_file;  // whole file image (keeps pointers valid)

    int m_ddrFieldControlLength = 9;
    std::vector<FieldDefn> m_fieldDefns;
    std::vector<RecordDef> m_records;
};

}  // namespace I8211
}  // namespace osgS57
