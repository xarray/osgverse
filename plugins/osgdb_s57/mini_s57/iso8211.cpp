// ============================================================================
// osgS57/src/iso8211.cpp
//
// Minimal ISO/IEC 8211 reader for IHO S-57 ENC data sets.
//
// Byte-level layout notes (offsets are relative to the start of a record):
//   Leader (24 bytes):
//      0..4    record length (ASCII, 5 digits; 0 => Annex C large record)
//      5       interchange level ('1'|'2'|'3')
//      6       leader identifier ('L' DDR, 'D' data record, 'R' reuse header)
//      10..11  field control length
//      12..16  base address of the data/field area (5 digits)
//      17..19  extended character set indicator
//      20      size in bytes of the directory "field length" field
//      21      size in bytes of the directory "field position" field
//      23      size in bytes of the directory "tag" field
//   Directory (starts at byte 24): one entry per field:
//      tag(sizeFieldTag) + length(sizeFieldLength) + position(sizeFieldPos)
//      terminated by a field terminator byte 0x1E.
//   Field area: starts at leader[12..16].
//
// The first record of an S-57 data set is a DDR.  Every DDR field data item
// carries:
//      [0] data structure code   [1] data type code
//      [fieldControlLength] field name  ... 0x1F  subfield list ... 0x1F
//      format controls ... 0x1E
// Format control codes, e.g. "(b11,b14,2b11,3A,2A(8),R(4),...)":
//   b<subtype><width> : binary. subtype 1 unsigned, 2 signed; width in bytes
//   B<..>             : binary with the opposite byte order of 'b'
//   A/I/R/S/C         : character data; with "(n)" fixed length n, otherwise
//                       variable, terminated by 0x1F (unit) / 0x1E (field).
// A leading repeat count ("2b11") duplicates the format item that follows.
// ============================================================================
#include "iso8211.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <sstream>

namespace osgS57 {
namespace I8211 {

namespace {
constexpr int kLeaderSize = 24;
constexpr char kFieldTerminator = 0x1E;
constexpr char kUnitTerminator = 0x1F;

// ASCII digits -> integer, ignoring leading whitespace and stopping at the
// first non-digit (mirrors atoi() semantics used by classic implementations).
int scanInt(const char* p, int n) {
    int i = 0;
    while (i < n && isspace(static_cast<unsigned char>(p[i]))) ++i;
    int val = 0;
    bool any = false;
    for (; i < n; ++i) {
        if (p[i] < '0' || p[i] > '9') break;
        val = val * 10 + (p[i] - '0');
        any = true;
    }
    return any ? val : 0;
}

// Fetch a delimited chunk of text; returns the characters before the first
// delimiter, and the number of source bytes consumed (including the
// delimiter when one was found).
std::string fetchVariable(const uint8_t* p, std::size_t size, int& consumed) {
    std::size_t i = 0;
    while (i + 1 < size && p[i] != kUnitTerminator && p[i] != kFieldTerminator)
        ++i;
    std::string out(reinterpret_cast<const char*>(p), i);
    consumed = static_cast<int>(i);
    if (i < size && (p[i] == kUnitTerminator || p[i] == kFieldTerminator))
        consumed = static_cast<int>(i) + 1;
    return out;
}

std::string trimRight(std::string s) {
    s.erase(std::find_if_not(s.rbegin(), s.rend(),
                             [](unsigned char c) { return c == ' ' || c == 0; })
                .base(),
            s.end());
    return s;
}

std::string lower(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

}  // namespace

// ---------------------------------------------------------------------------
// Value::display
// ---------------------------------------------------------------------------
std::string Value::display() const {
    switch (type) {
        case SubfieldType::Integer: {
            std::ostringstream os;
            os << i;
            return os.str();
        }
        case SubfieldType::Real: {
            std::ostringstream os;
            os << r;
            return os.str();
        }
        case SubfieldType::Text:
            return s;
        case SubfieldType::BinaryString:
            return hex;
        default:
            return s;
    }
}

// ---------------------------------------------------------------------------
// FieldDefn lookup helpers
// ---------------------------------------------------------------------------
const SubfieldDefn* FieldDefn::findSubfield(const std::string& name) const {
    for (const auto& sf : subfields) {
        if (lower(sf.name) == lower(name)) return &sf;
    }
    return nullptr;
}

const FieldDef* RecordDef::find(const std::string& tag, int instance) const {
    for (const auto& f : fields) {
        if (f.defn && lower(f.defn->tag) == lower(tag)) {
            if (instance == 0) return &f;
            --instance;
        }
    }
    return nullptr;
}

const FieldDefn* RecordDef::fieldDefn(const std::string& tag, int instance) const {
    const FieldDef* f = find(tag, instance);
    return f ? f->defn : nullptr;
}

// ---------------------------------------------------------------------------
// Iso8211Reader
// ---------------------------------------------------------------------------
const FieldDefn* Iso8211Reader::findFieldDefn(const std::string& tag) const {
    for (const auto& d : m_fieldDefns) {
        if (lower(d.tag) == lower(tag)) return &d;
    }
    return nullptr;
}

ParseResult Iso8211Reader::open(const std::string& path) {
    m_open = false;
    m_fieldDefns.clear();
    m_records.clear();
    m_error.clear();

    FILE* fp = nullptr;
#if defined(_MSC_VER) && _MSC_VER >= 1400
    fopen_s(&fp, path.c_str(), "rb");
#else
    fp = fopen(path.c_str(), "rb");
#endif
    if (!fp) {
        m_error = "Unable to open file: " + path;
        return {false, m_error};
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        m_error = "fseek failed: " + path;
        return {false, m_error};
    }
    long sz = ftell(fp);
    if (sz <= 0) {
        fclose(fp);
        m_error = "Empty or unseekable file: " + path;
        return {false, m_error};
    }
    rewind(fp);
    m_file.resize(static_cast<std::size_t>(sz));
    if (fread(m_file.data(), 1, m_file.size(), fp) != m_file.size()) {
        fclose(fp);
        m_error = "Short read: " + path;
        return {false, m_error};
    }
    fclose(fp);
    return parseLoaded();
}

ParseResult Iso8211Reader::openMemory(const std::vector<uint8_t>& data) {
    m_open = false;
    m_fieldDefns.clear();
    m_records.clear();
    m_error.clear();
    if (data.empty()) {
        m_error = "Empty buffer.";
        return {false, m_error};
    }
    m_file = data;
    return parseLoaded();
}

ParseResult Iso8211Reader::parseLoaded() {
    const uint8_t* base = m_file.data();

    // ---- first record must be a DDR ---------------------------------------
    if (m_file.size() < kLeaderSize) {
        m_error = "File is too small to contain an ISO8211 leader.";
        return {false, m_error};
    }
    std::size_t pos = 0;
    if (!parseDdr(m_file, pos, base)) {
        return {false, m_error};
    }

    // ---- remaining records ------------------------------------------------
    while (pos + kLeaderSize <= m_file.size()) {
        // padding runs of 0x00 / 0x20 / '^' may be appended to pad file size
        bool allPad = true;
        for (std::size_t i = pos; i < m_file.size(); ++i) {
            uint8_t c = m_file[i];
            if (c != 0x00 && c != 0x20 && c != '^') {
                allPad = false;
                break;
            }
        }
        if (allPad) break;

        if (m_file[pos] == '^' || m_file[pos] == 0x00) break;  // padding

        RecordDef rec;
        if (!parseRecord(m_file, pos, base, rec)) {
            if (!m_error.empty()) break;
            break;
        }
        if (rec.fields.empty() && pos >= m_file.size()) break;
        m_records.push_back(std::move(rec));
    }

    m_open = true;
    return {true, {}};
}

// ---------------------------------------------------------------------------
bool Iso8211Reader::parseDdr(const std::vector<uint8_t>& buf,
                             std::size_t& pos, const uint8_t* /*base*/) {
    const uint8_t* p = buf.data() + pos;
    if (buf.size() - pos < kLeaderSize) {
        m_error = "Short DDR record.";
        return false;
    }
    const char* leader = reinterpret_cast<const char*>(p);

    int recLength = scanInt(leader, 5);
    if (recLength < kLeaderSize || leader[6] != 'L') {
        m_error = "First record is not a valid ISO8211 DDR record.";
        return false;
    }
    int fieldControlLength = scanInt(leader + 10, 2);
    int fieldAreaStart = scanInt(leader + 12, 5);
    int sizeFieldLength = scanInt(leader + 20, 1);
    int sizeFieldPos = scanInt(leader + 21, 1);
    int sizeFieldTag = scanInt(leader + 23, 1);

    if (fieldControlLength <= 0 || fieldAreaStart < kLeaderSize ||
        fieldAreaStart >= recLength || sizeFieldLength <= 0 ||
        sizeFieldPos <= 0 || sizeFieldTag <= 0) {
        m_error = "Corrupt ISO8211 DDR leader.";
        return false;
    }
    m_ddrFieldControlLength = fieldControlLength;

    int entryWidth = sizeFieldLength + sizeFieldPos + sizeFieldTag;

    // count directory entries
    int count = 0;
    for (int i = kLeaderSize; i + entryWidth <= recLength; i += entryWidth) {
        if (p[i] == kFieldTerminator) break;
        ++count;
    }

    // build one FieldDefn per entry
    for (int i = 0; i < count; ++i) {
        int entry = kLeaderSize + i * entryWidth;
        const char* e = reinterpret_cast<const char*>(p) + entry;
        std::string tag(e, static_cast<std::size_t>(sizeFieldTag));
        int fLen = scanInt(e + sizeFieldTag, sizeFieldLength);
        int fPos = scanInt(e + sizeFieldTag + sizeFieldLength, sizeFieldPos);

        if (fieldAreaStart + fPos < 0 || fieldAreaStart + fPos + fLen > recLength) {
            m_error = "DDR field '" + tag + "' points outside the record.";
            return false;
        }
        FieldDefn defn;
        defn.tag = tag;
        if (!parseFieldDefnArea(p + fieldAreaStart + fPos, fLen, defn)) {
            m_error = "Failed to parse DDR field '" + tag + "'.";
            return false;
        }
        m_fieldDefns.push_back(std::move(defn));
    }

    pos += recLength;
    return true;
}

// ---------------------------------------------------------------------------
bool Iso8211Reader::parseRecord(const std::vector<uint8_t>& buf,
                                std::size_t& pos, const uint8_t* /*base*/,
                                RecordDef& out) {
    const uint8_t* p = buf.data() + pos;
    const char* leader = reinterpret_cast<const char*>(p);

    int recLength = scanInt(leader, 5);
    char leaderIden = leader[6];
    if (recLength == 0) {
        // ISO8211 Annex C "large record" variant: zero length record whose
        // fields are individually sized.  Not required by the S-64 data sets
        // handled in stage-1; report and stop.
        m_error =
            "ISO8211 Annex-C large record (zero length) not supported yet.";
        return false;
    }
    if (recLength <= kLeaderSize) return false;
    if (pos + static_cast<std::size_t>(recLength) > buf.size()) {
        m_error = "Data record extends beyond end of file.";
        return false;
    }
    if (leaderIden == 'R') {
        // reuse header: same structure as the preceding record (rare in S57)
        m_error = "ISO8211 reuse-header record not supported yet.";
        return false;
    }

    int fieldAreaStart = scanInt(leader + 12, 5);
    int sizeFieldLength = scanInt(leader + 20, 1);
    int sizeFieldPos = scanInt(leader + 21, 1);
    int sizeFieldTag = scanInt(leader + 23, 1);

    if (sizeFieldLength <= 0 || sizeFieldPos <= 0 || sizeFieldTag <= 0 ||
        fieldAreaStart < kLeaderSize || fieldAreaStart > recLength) {
        m_error = "Corrupt ISO8211 data record leader.";
        return false;
    }
    int entryWidth = sizeFieldLength + sizeFieldPos + sizeFieldTag;

    int count = 0;
    for (int i = kLeaderSize; i + entryWidth <= recLength; i += entryWidth) {
        if (p[i] == kFieldTerminator) break;
        ++count;
    }

    out.fields.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        int entry = kLeaderSize + i * entryWidth;
        const char* e = reinterpret_cast<const char*>(p) + entry;
        std::string tag(e, static_cast<std::size_t>(sizeFieldTag));
        int fLen = scanInt(e + sizeFieldTag, sizeFieldLength);
        int fPos = scanInt(e + sizeFieldTag + sizeFieldLength, sizeFieldPos);

        if (fieldAreaStart + fPos < 0 ||
            fieldAreaStart + fPos + fLen > recLength) {
            m_error = "Field '" + tag + "' points outside the record.";
            return false;
        }
        const FieldDefn* defn = findFieldDefn(tag);
        if (!defn) {
            // Tolerate undeclared tags (producer extensions).  The record is
            // kept, but the field is skipped by higher level decoders.
            continue;
        }
        FieldDef f;
        f.defn = defn;
        f.data = p + fieldAreaStart + fPos;
        f.size = static_cast<std::size_t>(fLen);
        out.fields.push_back(f);
    }

    pos += recLength;
    return true;
}

// ---------------------------------------------------------------------------
// Parse the contents of a DDR field into a FieldDefn.
// ---------------------------------------------------------------------------
bool Iso8211Reader::parseFieldDefnArea(const uint8_t* p, std::size_t size,
                                       FieldDefn& out) const {
    if (size < 2) return false;
    char structCode = static_cast<char>(p[0]);
    char typeCode = static_cast<char>(p[1]);
    // p[0]='0'/' ' elementary; '1' vector; '2' array; '3' concatenated
    out.dataStructCode =
        (structCode == '0' || structCode == ' ') ? 0 : (structCode - '0');
    out.dataTypeCode = typeCode - '0';
    if (out.dataTypeCode < 0 || out.dataTypeCode > 9) out.dataTypeCode = 0;

    std::size_t off = static_cast<std::size_t>(m_ddrFieldControlLength);
    if (off > size) off = size;

    int consumed = 0;
    out.fieldName = fetchVariable(p + off, size - off, consumed);
    off += static_cast<std::size_t>(consumed);

    if (out.dataStructCode != 0) {
        out.arrayDescr = fetchVariable(p + off, size - off, consumed);
        off += static_cast<std::size_t>(consumed);

        out.formatControls = fetchVariable(p + off, size - off, consumed);
        off += static_cast<std::size_t>(consumed);

        if (out.formatControls.empty() && !out.arrayDescr.empty())
            return false;

        buildSubfields(out);
        if (!applyFormats(out)) return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Convert the DDR "subfield list" ("*YCOO!XCOO") into SubfieldDefn names.
// ---------------------------------------------------------------------------
void Iso8211Reader::buildSubfields(FieldDefn& d) const {
    std::string list = d.arrayDescr;
    // '*' marks a repeating subfield group
    std::size_t star = list.find('*');
    while (star != std::string::npos) {
        d.repeating = true;
        list.erase(0, star + 1);
        star = list.find('*');
    }
    d.subfields.clear();
    std::size_t start = 0;
    while (start <= list.size()) {
        std::size_t end = list.find('!', start);
        if (end == std::string::npos) end = list.size();
        std::string name = list.substr(start, end - start);
        name = trimRight(name);
        // strip an optional trailing '&' (older producers used '&' as a
        // name separator too)
        if (!name.empty() && name.back() == '&') name.pop_back();
        if (!name.empty()) {
            SubfieldDefn sf;
            sf.name = name;
            d.subfields.push_back(sf);
        }
        if (end == list.size()) break;
        start = end + 1;
    }
}

// ---------------------------------------------------------------------------
// Expand "(b11,b14,2b11,3A,2A(8),R(4),b11,2A,b11,b12,A)" into one format
// item per subfield, then assign each to d.subfields[i].
// ---------------------------------------------------------------------------
bool Iso8211Reader::applyFormats(FieldDefn& d) const {
    std::string c = d.formatControls;
    // strip surrounding parentheses
    if (c.size() >= 2 && c.front() == '(' && c.back() == ')') {
        c = c.substr(1, c.size() - 2);
    }

    // ---- expand repeat prefixes -------------------------------------------
    std::vector<std::string> items;
    std::size_t i = 0;
    while (i < c.size()) {
        // read a comma-separated segment honouring nested parentheses
        std::size_t j = i;
        int depth = 0;
        for (; j < c.size(); ++j) {
            char ch = c[j];
            if (ch == '(') ++depth;
            else if (ch == ')' && depth > 0) --depth;
            else if (ch == ',' && depth == 0) break;
        }
        std::string seg = c.substr(i, j - i);
        i = (j < c.size()) ? j + 1 : j;

        // leading decimal count duplicates the remainder of the segment
        std::size_t k = 0;
        while (k < seg.size() && seg[k] >= '0' && seg[k] <= '9') ++k;
        int repeat = 1;
        if (k > 0) {
            repeat = scanInt(seg.c_str(), static_cast<int>(k));
        }
        std::string body = seg.substr(k);
        if (body.empty()) continue;
        for (int r = 0; r < repeat; ++r) items.push_back(body);
    }

    // ---- assign one format string per subfield ----------------------------
    if (items.size() < d.subfields.size()) {
        // tolerate producers that group trailing subfields into the last
        // format: replicate the final format item to fill the gap
        while (items.size() < d.subfields.size() && !items.empty())
            items.push_back(items.back());
    }

    std::size_t n = std::min(d.subfields.size(), items.size());
    for (std::size_t k = 0; k < n; ++k) {
        SubfieldDefn& sf = d.subfields[k];
        sf.format = items[k];
        std::string f = items[k];
        char code = f.empty() ? 0 : f[0];
        bool hasParen = f.size() > 1 && f[1] == '(';
        int width = 0;
        if (hasParen) width = scanInt(f.c_str() + 2, static_cast<int>(f.size() - 2));

        switch (code) {
            case 'A':
            case 'C':
                sf.type = SubfieldType::Text;
                if (hasParen) { sf.width = width; sf.variable = false; }
                break;
            case 'I':
            case 'S':
                sf.type = SubfieldType::Integer;
                if (hasParen) { sf.width = width; sf.variable = false; }
                break;
            case 'R':
                sf.type = SubfieldType::Real;
                if (hasParen) { sf.width = width; sf.variable = false; }
                break;
            case 'b':
            case 'B': {
                sf.type = SubfieldType::Integer;
                sf.variable = false;
                sf.bigEndian = (code == 'B');  // 'B' is byte-swapped 'b'
                if (hasParen) {
                    // width expressed in bits, e.g. "b(32)"
                    sf.width = width / 8;
                    sf.binary = BinaryFormat::SignedInt;
                    if (sf.width >= 5) sf.type = SubfieldType::BinaryString;
                } else {
                    int subtype = (f.size() > 1 && f[1] >= '0' && f[1] <= '5')
                                      ? (f[1] - '0')
                                      : 1;
                    sf.binary = static_cast<BinaryFormat>(subtype);
                    sf.width = f.size() > 2 ? scanInt(f.c_str() + 2,
                                                      static_cast<int>(f.size() - 2))
                                            : 0;
                    switch (sf.binary) {
                        case BinaryFormat::UnsignedInt:
                        case BinaryFormat::SignedInt:
                            sf.type = SubfieldType::Integer;
                            break;
                        case BinaryFormat::FPReal:
                        case BinaryFormat::FloatReal:
                            sf.type = SubfieldType::Real;
                            break;
                        case BinaryFormat::FloatComplex:
                        default:
                            sf.type = SubfieldType::BinaryString;
                            break;
                    }
                    if (sf.width >= 5) sf.type = SubfieldType::BinaryString;
                }
                break;
            }
            default:
                sf.type = SubfieldType::Unknown;
                break;
        }
        if (sf.type == SubfieldType::Unknown) return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Compute the length in bytes of one subfield value and how many bytes it
// consumes in the stream (variable fields consume their terminator too).
// ---------------------------------------------------------------------------
int Iso8211Reader::subfieldLength(const SubfieldDefn& sf, const uint8_t* p,
                                  std::size_t remain,
                                  std::size_t& consumed) const {
    int len = 0;
    if (!sf.variable) {
        len = std::min<int>(sf.width, static_cast<int>(remain));
        consumed = static_cast<std::size_t>(len);
    } else {
        std::size_t i = 0;
        while (i < remain && p[i] != kUnitTerminator &&
               p[i] != kFieldTerminator)
            ++i;
        len = static_cast<int>(i);
        consumed = i + 1;  // includes terminator (next field will start there)
    }
    return len;
}

// ---------------------------------------------------------------------------
Value Iso8211Reader::readValue(const SubfieldDefn& sf, const uint8_t* p,
                               std::size_t remain) const {
    Value v;
    v.type = sf.type;
    std::size_t consumed = 0;
    int len = subfieldLength(sf, p, remain, consumed);

    switch (sf.type) {
        case SubfieldType::Integer: {
            v.valid = true;
            if (sf.variable) {
                std::string s(reinterpret_cast<const char*>(p),
                              static_cast<std::size_t>(len));
                v.i = atoll(s.c_str());
                v.s = s;
            } else {
                if (sf.binary != BinaryFormat::None) {
                    // binary integer (b/B).  ISO8211 'b' data is stored in
                    // the producer's native order; on little-endian machines
                    // it is little-endian and 'B' (uppercase) marks the
                    // opposite order.  We always build the value with
                    // little-endian byte order after optionally reversing.
                    uint8_t b[8] = {0};
                    int n = std::min<int>(len, 8);
                    for (int k = 0; k < n; ++k)
                        b[k] = p[k];
                    if (sf.bigEndian) std::reverse(b, b + n);
                    long long u = 0;
                    for (int k = 0; k < n; ++k) u |= static_cast<long long>(b[k]) << (8 * k);
                    if (sf.binary == BinaryFormat::SignedInt && n < 8) {
                        int bits = n * 8;
                        long long sign = 1LL << (bits - 1);
                        long long m = (1LL << bits) - 1;
                        if (u & sign) u = -(~u & m) - 1;
                    }
                    v.i = u;
                    std::ostringstream os;
                    os << u;
                    v.s = os.str();
                } else {
                    std::string s(reinterpret_cast<const char*>(p),
                                  static_cast<std::size_t>(len));
                    v.i = atoll(s.c_str());
                    v.s = s;
                }
            }
            break;
        }
        case SubfieldType::Real: {
            v.valid = true;
            if (sf.binary == BinaryFormat::FloatReal && !sf.variable) {
                if (len == 4) {
                    uint8_t b[4];
                    for (int k = 0; k < 4; ++k) b[k] = p[k];
                    if (sf.bigEndian) std::reverse(b, b + 4);
                    float f;
                    std::memcpy(&f, b, 4);
                    v.r = f;
                } else if (len >= 8) {
                    uint8_t b[8];
                    for (int k = 0; k < 8; ++k) b[k] = p[k];
                    if (sf.bigEndian) std::reverse(b, b + 8);
                    double dd;
                    std::memcpy(&dd, b, 8);
                    v.r = dd;
                }
                std::ostringstream os;
                os << v.r;
                v.s = os.str();
            } else {
                std::string s(reinterpret_cast<const char*>(p),
                              static_cast<std::size_t>(len));
                v.r = atof(s.c_str());
                std::ostringstream os;
                os << v.r;
                v.s = os.str();
            }
            break;
        }
        case SubfieldType::Text: {
            v.valid = true;
            std::string s(reinterpret_cast<const char*>(p),
                          static_cast<std::size_t>(len));
            s = trimRight(std::move(s));
            v.s = s;
            break;
        }
        case SubfieldType::BinaryString: {
            v.valid = true;
            static const char* hexc = "0123456789ABCDEF";
            for (int k = 0; k < len; ++k) {
                v.hex.push_back(hexc[p[k] >> 4]);
                v.hex.push_back(hexc[p[k] & 0xF]);
            }
            break;
        }
        default:
            break;
    }
    (void)consumed;
    return v;
}

// ---------------------------------------------------------------------------
// How many instances of the subfield group does a field contain?
// ---------------------------------------------------------------------------
int Iso8211Reader::repeatCount(const FieldDef& f) const {
    if (!f.defn || !f.defn->repeating || f.defn->subfields.empty()) return 1;
    // sum widths when every subfield is fixed width
    bool allFixed = true;
    int total = 0;
    for (const auto& sf : f.defn->subfields) {
        if (sf.variable) {
            allFixed = false;
            break;
        }
        total += sf.width;
    }
    if (allFixed && total > 0) return static_cast<int>(f.size / total);
    // variable width: walk the instances
    const uint8_t* p = f.data;
    std::size_t remain = f.size;
    int count = 0;
    while (remain > 0) {
        // scan one instance
        const uint8_t* q = p;
        std::size_t r = remain;
        for (const auto& sf : f.defn->subfields) {
            std::size_t consumed = 0;
            subfieldLength(sf, q, r, consumed);
            q += consumed;
            r -= (consumed <= r) ? consumed : r;
        }
        if (q == p) break;  // no progress -> malformed
        p = q;
        remain = r;
        ++count;
    }
    return count;
}

// ---------------------------------------------------------------------------
// Decode one field's subfields into printable rows (all instances).
// ---------------------------------------------------------------------------
std::vector<Iso8211Reader::Row> Iso8211Reader::decodeRecord(
    const RecordDef& rec) const {
    std::vector<Row> out;
    int fi = 0;
    for (const auto& f : rec.fields) {
        // elementary fields (no subfields) carry a raw byte payload: print it
        if (!f.defn || f.defn->subfields.empty()) {
            Row row;
            row.fieldIndex = fi;
            row.tag = f.defn ? f.defn->tag : "?";
            row.subfield = "*data*";
            row.repeat = 0;
            Value v;
            v.type = SubfieldType::Text;
            v.valid = true;
            std::string s(reinterpret_cast<const char*>(f.data), f.size);
            bool printable = true;
            for (std::size_t k = 0; k < f.size; ++k) {
                unsigned char c = f.data[k];
                if (c != '\t' && c != '\r' && c != '\n' &&
                    (c < 0x20 || c > 0x7e)) {
                    printable = false;
                    break;
                }
            }
            if (printable) {
                v.s = trimRight(std::move(s));
            } else {
                v.type = SubfieldType::BinaryString;
                static const char* hexc = "0123456789ABCDEF";
                for (std::size_t k = 0; k < f.size; ++k) {
                    v.hex.push_back(hexc[f.data[k] >> 4]);
                    v.hex.push_back(hexc[f.data[k] & 0xF]);
                }
            }
            row.value = std::move(v);
            out.push_back(std::move(row));
            ++fi;
            continue;
        }
        int reps = repeatCount(f);
        const uint8_t* p = f.data;
        std::size_t remain = f.size;
        for (int rep = 0; rep < reps && remain > 0; ++rep) {
            for (const auto& sf : f.defn->subfields) {
                std::size_t consumed = 0;
                Value v = readValue(sf, p, remain);
                subfieldLength(sf, p, remain, consumed);
                p += consumed;
                if (remain >= consumed) remain -= consumed;
                else remain = 0;

                Row row;
                row.fieldIndex = fi;
                row.tag = f.defn->tag;
                row.repeat = rep;
                row.subfield = sf.name;
                row.value = std::move(v);
                out.push_back(std::move(row));
            }
        }
        ++fi;
    }
    return out;
}

// ---------------------------------------------------------------------------
// Direct accessors (index the field by ordinal; index the subfield by name
// and the occurrence within the repeated group).
// ---------------------------------------------------------------------------
static const I8211::FieldDef* locate(const Iso8211Reader& rdr,
                                     const I8211::RecordDef& rec,
                                     const std::string& field, int iField) {
    int k = 0;
    for (const auto& f : rec.fields) {
        if (f.defn && lower(f.defn->tag) == lower(field)) {
            if (k == iField) return &f;
            ++k;
        }
    }
    (void)rdr;
    return nullptr;
}

I8211::Value Iso8211Reader::valueSubfield(const RecordDef& rec,
                                          const std::string& field,
                                          int iField,
                                          const std::string& subfield,
                                          int iInstance) const {
    Value out;
    const FieldDef* f = locate(*this, rec, field, iField);
    if (!f || !f->defn) return out;
    const SubfieldDefn* sf = f->defn->findSubfield(subfield);
    if (!sf) return out;

    const uint8_t* p = f->data;
    std::size_t remain = f->size;
    int rep = 0;
    while (remain > 0 && rep <= iInstance) {
        for (const auto& s2 : f->defn->subfields) {
            if (&s2 == sf && rep == iInstance) {
                out = readValue(*sf, p, remain);
                return out;
            }
            std::size_t consumed = 0;
            subfieldLength(s2, p, remain, consumed);
            p += consumed;
            if (remain >= consumed) remain -= consumed;
            else remain = 0;
        }
        ++rep;
    }
    return out;
}

long long Iso8211Reader::intSubfield(const RecordDef& rec,
                                     const std::string& field, int iField,
                                     const std::string& subfield,
                                     int iInstance, bool* ok) const {
    Value v = valueSubfield(rec, field, iField, subfield, iInstance);
    if (ok) *ok = v.valid && (v.type == SubfieldType::Integer ||
                              v.type == SubfieldType::Text);
    return v.i;
}

double Iso8211Reader::doubleSubfield(const RecordDef& rec,
                                     const std::string& field, int iField,
                                     const std::string& subfield,
                                     int iInstance, bool* ok) const {
    Value v = valueSubfield(rec, field, iField, subfield, iInstance);
    if (ok) *ok = v.valid && (v.type == SubfieldType::Real ||
                              v.type == SubfieldType::Integer);
    return v.r;
}

std::string Iso8211Reader::stringSubfield(const RecordDef& rec,
                                          const std::string& field, int iField,
                                          const std::string& subfield,
                                          int iInstance, bool* ok) const {
    Value v = valueSubfield(rec, field, iField, subfield, iInstance);
    if (ok) *ok = v.valid && v.type == SubfieldType::Text;
    return v.s;
}

}  // namespace I8211
}  // namespace osgS57
