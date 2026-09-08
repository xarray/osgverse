// ============================================================================
// osgS57/s57chart.h -- S-57 (ENC) semantic helpers on top of the ISO8211
// reader.  Kept deliberately small for stage-1 (parse + report).  The model
// will grow into OSG geometry building blocks in later stages.
// ============================================================================
#pragma once

#include "iso8211.h"

#include <cstdint>
#include <string>
#include <vector>

namespace osgS57 {
namespace S57 {

// DSPM -- data set parameter values that drive geometry decoding.
struct DatasetParameters {
    bool present = false;

    long long rcnm = 0;
    long long rcid = 0;
    long long hdat = 0;  // horizontal datum
    long long vdat = 0;  // vertical datum
    long long sdat = 0;  // sounding datum
    long long cscl = 0;  // compilation scale
    long long duni = 0;  // units of depth (1=m 2=ft 3=fath)
    long long huni = 0;  // units of height (1=m 2=ft)
    long long puni = 0;  // units of position (1=deg 2=?)
    long long coun = 0;  // country / producing agency
    long long comf = 0;  // coordinate multiplication factor
    long long somf = 0;  // sounding multiplication factor
    std::string comt;

    // Convert a stored integer coordinate into degrees (lon/lat).
    double positionScale() const {
        // S-57 default COMF is 1,000,000 when the DSPM field is absent.
        return comf > 0 ? static_cast<double>(comf) : 1000000.0;
    }
    // Convert a stored depth integer into the sounding unit.
    double soundingScale() const {
        return somf > 0 ? static_cast<double>(somf) : 1.0;
    }
};

// Decode the DSPM field of a data-set parameter record.
DatasetParameters readDspm(const I8211::Iso8211Reader& rdr,
                           const I8211::RecordDef& rec);

// A decoded coordinate triple from an SG2D / SG3D field.
struct ChartPoint {
    double lon = 0.0;    // degrees
    double lat = 0.0;    // degrees
    double depth = 0.0;  // in sounding units, 0 when not a 3D point
    bool hasDepth = false;
    bool valid = false;
};

// Extract the repeated coordinate triples of one spatial record
// (fields SG2D / SG3D, whichever exists).
std::vector<ChartPoint> spatialCoordinates(
    const I8211::Iso8211Reader& rdr, const I8211::RecordDef& rec,
    const DatasetParameters& dspm);

// ATTF / NATF attribute instance { code, raw value }.
struct AttributeInstance {
    long long code = 0;
    std::string value;
};

// Attributes of a feature record (repeated ATTF + NATF fields).
std::vector<AttributeInstance> readAttributes(
    const I8211::Iso8211Reader& rdr, const I8211::RecordDef& rec);

// Classification helpers -----------------------------------------------------
// RCNM codes seen in the general exchange and spatial/feature records.
enum Rcnm {
    RCNM_DSID = 10,
    RCNM_CATD = 15,
    RCNM_DSPM = 20,
    RCNM_FEATURE = 100,
    RCNM_ISOLATED_NODE = 110,
    RCNM_CONNECTED_NODE = 120,
    RCNM_EDGE = 130,
    RCNM_FACE = 140,
};

// First RCNM field (in field tag order) of the record, if any.
int recordRcnm(const I8211::Iso8211Reader& rdr,
               const I8211::RecordDef& rec);

}  // namespace S57
}  // namespace osgS57
