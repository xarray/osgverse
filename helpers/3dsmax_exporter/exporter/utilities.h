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

inline osg::Vec4 convertColorRef(COLORREF color)
{ return osg::Vec4(GetRValue(color) / 255.0f, GetGValue(color) / 255.0f, GetBValue(color) / 255.0f, 1.0f); }

inline osg::Vec4 convertColor(Color& color, float alpha=1.0f)
{ return osg::Vec4(color.r, color.g, color.b, alpha); }

inline osg::Vec2 convertPoint2(const Point3& vec)
{ return osg::Vec2(vec[0], vec[1]); }

inline osg::Vec3 convertPoint(const Point3& vec)
{ return osg::Vec3(vec[0], vec[1], vec[2]); }

inline osg::Vec4 convertPoint4(const Point3& vec)
{ return osg::Vec4(vec[0], vec[1], vec[2], 1.0f); }

inline osg::Quat convertQuat(const Quat& quat)
{ return osg::Quat(quat[0], quat[1], quat[2], quat[3]); }

inline osg::Matrix convertMatrix(const Matrix3& matrix)
{
    const MRow* m = matrix.GetAddr();
    return osg::Matrix( m[0][0], m[0][1], m[0][2], 0.0,
                        m[1][0], m[1][1], m[1][2], 0.0,
                        m[2][0], m[2][1], m[2][2], 0.0,
                        m[3][0], m[3][1], m[3][2], 1.0 );
}

#endif
