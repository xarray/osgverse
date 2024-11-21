#pragma once

namespace mapbox {
namespace geometry {

template <typename T>
struct point
{
    using coordinate_type = T;

    constexpr point()
        : x(), y()
    {
    }
    constexpr point(T x_, T y_)
        : x(x_), y(y_)
    {
    }

    T x;
    T y;
};

#ifndef _WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

template <typename T>
constexpr bool operator==(point<T> const& lhs, point<T> const& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

#ifndef _WIN32
#pragma GCC diagnostic pop
#endif

template <typename T>
constexpr bool operator!=(point<T> const& lhs, point<T> const& rhs)
{
    return !(lhs == rhs);
}

} // namespace geometry
} // namespace mapbox
