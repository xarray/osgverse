/*****************************************************************************************
 *              MIT License                                                              *
 *                                                                                       *
 * Copyright (c) 2020 Gianmarco Cherchi, Marco Livesu, Riccardo Scateni e Marco Attene   *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION     *
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE        *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                *
 *                                                                                       *
 * Authors:                                                                              *
 *      Gianmarco Cherchi (g.cherchi@unica.it)                                           *
 *      https://people.unica.it/gianmarcocherchi/                                        *
 *                                                                                       *
 *      Marco Livesu (marco.livesu@ge.imati.cnr.it)                                      *
 *      http://pers.ge.imati.cnr.it/livesu/                                              *
 *                                                                                       *
 *      Riccardo Scateni (riccardo@unica.it)                                             *
 *      https://people.unica.it/riccardoscateni/                                         *
 *                                                                                       *
 *      Marco Attene (marco.attene@ge.imati.cnr.it)                                      *
 *      https://www.cnr.it/en/people/marco.attene/                                       *
 *                                                                                       *
 * ***************************************************************************************/

#ifndef DEBUG_H
#define DEBUG_H

#include <string>
#include <fstream>
#include <chrono>
#include <iomanip>

#include "indirect_predicates.h"
#include <cinolib/meshes/trimesh.h>

std::string to_string_prec( double d )
{
    std::ostringstream stm ;
    stm << std::setprecision(std::numeric_limits<double>::digits10) << d ;
    return stm.str() ;
}

inline std::string ts(const double &n)
{
    std::string s = std::to_string(n);
    std::replace(s.begin(), s.end(), '.', ',');
    return s;
}

inline void saveStatisticsOnFile(const std::string &file_in, const double &time, const double &time2)
{
    std::string filename = "log.csv";

    std::string header = "model; time; time mul\n";

    std::string data = file_in.substr(0, file_in.length() - 4); // model name without extension

    data = data + ";" + ts(time) + ";" + ts(time2) + "\n";

    std::ifstream ifs;
    bool exists;

    // we check if the file exist to insert the header
    ifs.open(filename);
    if(ifs)
    {
        exists = true;
        ifs.close();
    }
    else
        exists = false;

    // we inser the data
    std::ofstream ofs;
    ofs.open (filename, std::ofstream::out | std::ofstream::app);

    if(exists)
        ofs << data;
    else
    {
        ofs << header;
        ofs << data;
    }

    ofs.close();
}

inline static std::chrono::time_point<std::chrono::system_clock> startChrono()
{
    return std::chrono::system_clock::now();
}

inline double stopChrono(std::chrono::time_point<std::chrono::system_clock> &start)
{
    auto time = std::chrono::system_clock::now() - start;
    return std::chrono::duration <double, std::milli> (time).count() / 1000;
}



inline cinolib::vec3d genericPointToCinolib(const genericPoint &p)
{
    if(p.isExplicit3D())
    {
        explicitPoint3D ep = p.toExplicit3D();
        return cinolib::vec3d(ep.X(), ep.Y(), ep.Z());
    }
    else if(p.isLPI())
    {
        implicitPoint3D_LPI ip = p.toLPI();
        double x, y, z;
        ip.getApproxXYZCoordinates(x, y, z);
        return cinolib::vec3d(x, y, z);
    }
    else if(p.isTPI())
    {
        implicitPoint3D_TPI ip = p.toTPI();
        double x, y, z;
        ip.getApproxXYZCoordinates(x, y, z);
        return cinolib::vec3d(x, y, z);
    }
    return cinolib::vec3d(0,0,0); // warning killer
}


inline explicitPoint3D genericPointToExplicit(const genericPoint &p)
{
    if(p.isExplicit3D())
    {
        explicitPoint3D ep = p.toExplicit3D();
        return ep;
    }
    else if(p.isLPI())
    {
        implicitPoint3D_LPI ip = p.toLPI();
        double x, y, z;
        ip.getApproxXYZCoordinates(x, y, z);
        return explicitPoint3D(x, y, z);
    }
    else if(p.isTPI())
    {
        implicitPoint3D_TPI ip = p.toTPI();
        double x, y, z;
        ip.getApproxXYZCoordinates(x, y, z);
        return explicitPoint3D(x, y, z);
    }

    return explicitPoint3D(0,0,0); // warning killer
}


inline std::string genericPointToString(const genericPoint &p)
{
    std::setprecision(std::numeric_limits<long double>::digits10 + 1);

    if(p.isExplicit3D())
    {
        explicitPoint3D ep = p.toExplicit3D();
        return "(" + to_string_prec(ep.X()) + ", " + to_string_prec(ep.Y()) + ", " + to_string_prec(ep.Z()) + ");\n";
    }
    else if(p.isLPI())
    {
        implicitPoint3D_LPI ip = p.toLPI();
        double x, y, z;
        ip.getApproxXYZCoordinates(x, y, z);

        return "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ");\n";
    }
    else if(p.isTPI())
    {
        implicitPoint3D_TPI ip = p.toTPI();
        double x, y, z;
        ip.getApproxXYZCoordinates(x, y, z);

        return "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ");\n";
    }

    assert(false);
    return "(0.0, 0.0, 0.0)"; //warning killer
}


inline void printGenericPoint(const genericPoint &p)
{
    if(p.isExplicit3D())    std::cerr << "EXP: " + genericPointToString(p) << std::endl;
    else if(p.isLPI())      std::cerr << "LPI: " + genericPointToString(p) << std::endl;
    else if(p.isTPI())      std::cerr << "TPI: " + genericPointToString(p) << std::endl;
}


inline void printGenericPointExploded(const genericPoint &gp)
{
    if(gp.isExplicit3D())
        std::cerr << "explicitPoint3D E" + genericPointToString(gp) << std::endl;

    else if(gp.isLPI())
    {
        std::cerr << "explicitPoint3D Pi" + genericPointToString(gp.toLPI().P());
        std::cerr << "explicitPoint3D Qi" + genericPointToString(gp.toLPI().Q());
        std::cerr << "explicitPoint3D Ri" + genericPointToString(gp.toLPI().R());
        std::cerr << "explicitPoint3D Si" + genericPointToString(gp.toLPI().S());
        std::cerr << "explicitPoint3D Ti" + genericPointToString(gp.toLPI().T());
        std::cerr << "implicitPoint3D_LPI L(Pi, Qi, Ri, Si, Ti); " << std::endl;
    }

    else if(gp.isTPI())
    {
        std::cerr << "explicitPoint3D U1i" + genericPointToString(gp.toTPI().U1());
        std::cerr << "explicitPoint3D U2i" + genericPointToString(gp.toTPI().U2());
        std::cerr << "explicitPoint3D U3i" + genericPointToString(gp.toTPI().U3());
        std::cerr << "explicitPoint3D V1i" + genericPointToString(gp.toTPI().V1());
        std::cerr << "explicitPoint3D V2i" + genericPointToString(gp.toTPI().V2());
        std::cerr << "explicitPoint3D V3i" + genericPointToString(gp.toTPI().V3());
        std::cerr << "explicitPoint3D W1i" + genericPointToString(gp.toTPI().W1());
        std::cerr << "explicitPoint3D W2i" + genericPointToString(gp.toTPI().W2());
        std::cerr << "explicitPoint3D W3i" + genericPointToString(gp.toTPI().W3());
        std::cerr << "implicitPoint3D_TPI L(U1i, U2i, U3i, V1i, V2i, V3i, W1i, W2i, W3i); " << std::endl;
    }
}




#endif // DEBUG_H
