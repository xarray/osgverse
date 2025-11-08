#pragma once

#include <cassert>

namespace Discregrid
{

class Halfedge
{
public:

    Halfedge() : m_code(3) {}
    Halfedge(Halfedge const&) = default;
    Halfedge(unsigned int f, unsigned char e)
        : m_code((f << 2) | e)
    {
        //assert(e < 3);
    }

    Halfedge next() const
    {
        return Halfedge(face(), (edge() + 1) % 3);
    }

    Halfedge previous() const
    {
        return Halfedge(face(), (edge() + 2) % 3);
    }

    bool operator==(Halfedge const& other) const
    {
        return m_code == other.m_code;
    }

    unsigned int face() const { return m_code >> 2; }
    unsigned char edge() const { return m_code & 0x3; }
    bool isBoundary() const { return edge() == 3; }

private:

    Halfedge(unsigned int code) : m_code(code) {}
    unsigned int m_code;
};
}


