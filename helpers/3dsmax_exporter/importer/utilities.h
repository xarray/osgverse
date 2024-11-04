/* -*-c++-*- osg2max - Copyright (C) 2010 by Wang Rui <wangray84 at gmail dot com>
* The scene import utilities
*/

#ifndef OSG2MAX_UTILITIES
#define OSG2MAX_UTILITIES

#include <osg/Vec2>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/Quat>
#include <osg/Matrix>
#include <max.h>
#include <codecvt>
#include <string>

static std::wstring s2ws(const std::string& str)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;
    return converterX.from_bytes(str);
}

static std::string ws2s(const std::wstring& wstr)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;
    return converterX.to_bytes(wstr);
}

inline COLORREF convertColorRef(const osg::Vec4& color)
{ return RGB(color[0] * 255.0f, color[1] * 255.0f, color[2] * 255.0f); }

inline Point3 convertPoint2(const osg::Vec2& vec)
{ return Point3(vec[0], vec[1], 0.0f); }

inline Point3 convertPoint(const osg::Vec3& vec)
{ return Point3(vec[0], vec[1], vec[2]); }

inline Point3 convertPoint4(const osg::Vec4& vec)
{ return Point3(vec[0], vec[1], vec[2]); }

inline Quat convertQuat(const osg::Quat& quat)
{ return Quat(quat[0], quat[1], quat[2], quat[3]); }

inline Matrix3 convertMatrix(const osg::Matrix& matrix)
{
    return Matrix3(Point3(matrix(0, 0), matrix(0, 1), matrix(0, 2)),
                   Point3(matrix(1, 0), matrix(1, 1), matrix(1, 2)),
                   Point3(matrix(2, 0), matrix(2, 1), matrix(2, 2)),
                   Point3(matrix(3, 0), matrix(3, 1), matrix(3, 2)) );
}

#endif
