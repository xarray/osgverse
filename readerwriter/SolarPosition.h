#ifndef MANA_READERWRITER_SOLAR_POSITION_H
#define MANA_READERWRITER_SOLAR_POSITION_H

#include <cmath>

namespace osgVerse
{
    // Subsolar point: solar declination and longitude of the sun's ground track (radians)
    struct SubsolarPoint { double declRad; double lonRad; };

    // Compute the subsolar point from UTC year/month/day + UTC hours (0-24, fractional).
    // NOAA SPA approximation, accuracy ~±0.5°, header-only and dependency-free.
    inline SubsolarPoint computeSubsolarPoint(int year, int month, int day, double utcHours)
    {
        static const int cum[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
        if (month < 1) month = 1; if (month > 12) month = 12;
        if (day < 1) day = 1; if (day > 31) day = 31;

        int doy = cum[month - 1] + day;
        // Leap-year correction: add one day from March onwards (Feb 29 shifts the day count)
        const bool leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
        if (leap && month > 2) doy += 1;

        const double PI = 3.14159265358979323846;
        double gamma = (2.0 * PI / 365.0) * ((double)(doy - 1) + (utcHours - 12.0) / 24.0);
        double decl = 0.006918 - 0.399912 * cos(gamma) + 0.070257 * sin(gamma)
                    - 0.006758 * cos(2 * gamma) + 0.000907 * sin(2 * gamma)
                    - 0.002697 * cos(3 * gamma) + 0.00148 * sin(3 * gamma);
        double eqtimeMin = 229.18 * (0.000075 + 0.001868 * cos(gamma) - 0.032077 * sin(gamma)
                    - 0.014615 * cos(2 * gamma) - 0.040849 * sin(2 * gamma));
        double subsolarLonDeg = -15.0 * (utcHours - 12.0) - eqtimeMin / 4.0;
        while (subsolarLonDeg > 180.0) subsolarLonDeg -= 360.0;
        while (subsolarLonDeg < -180.0) subsolarLonDeg += 360.0;
        SubsolarPoint s; s.declRad = decl; s.lonRad = subsolarLonDeg * PI / 180.0;
        return s;
    }
}

#endif
