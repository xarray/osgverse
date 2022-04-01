#ifndef MANA_MESH_DEFORMER_HPP
#define MANA_MESH_DEFORMER_HPP

#include <map>
#include <vector>
#include <iostream>

namespace osgVerse
{
    namespace helper
    {
        class vec2
        {
        public:
            double x, y;
            vec2(double x, double y) { this->x = x; this->y = y; }
            vec2(double v) { this->x = v; this->y = v; }
            vec2() { this->x = this->y = 0.0; }
        };

        class vec3
        {
        public:
            double x, y, z;
            vec3(double x, double y, double z) { this->x = x; this->y = y; this->z = z; }
            vec3(double v) { this->x = v; this->y = v; this->z = v; }
            vec3() { this->x = this->y = this->z = 0; }
            vec3& operator+=(const vec3& b) { (*this) = (*this) + b; return (*this); }
            vec3& operator-=(const vec3& b) { (*this) = (*this) - b; return (*this); }

            friend bool operator<(const vec3& a, const vec3& b)
            {
                if (a.x < b.x) return true; else if (a.x > b.x) return false;
                if (a.y < b.y) return true; else if (a.y > b.y) return false;
                if (a.z < b.z) return true; else if (a.z > b.z) return false; return false;
            }

            friend vec3 operator-(const vec3& a, const vec3& b) { return vec3(a.x - b.x, a.y - b.y, a.z - b.z); }
            friend vec3 operator+(const vec3& a, const vec3& b) { return vec3(a.x + b.x, a.y + b.y, a.z + b.z); }
            friend vec3 operator*(const double s, const vec3& a) { return vec3(s * a.x, s * a.y, s * a.z); }
            friend vec3 operator*(const vec3& a, const double s) { return s * a; }

            static double length(const vec3& a) { return sqrt(vec3::dot(a, a)); }
            static double dot(const vec3& a, const vec3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
            static double distance(const vec3& a, const vec3& b) { return length(a - b); }
            static vec3 normalize(const vec3& a) { return (1.0f / vec3::length(a)) * a; }
            static vec3 cross(const vec3& a, const vec3& b)
            { return vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }

            static vec3 rotate(const vec3& v, const double theta, const vec3& axis)
            {
                vec3 k = vec3::normalize(axis); // normalize for good measure.
                return v * cos(theta) + vec3::cross(k, v)* sin(theta) + (k * vec3::dot(k, v)) * (1.0f - cos(theta));
            }
        };
    }

    class MeshDeformer
    {
    public:
        MeshDeformer();
        ~MeshDeformer();

        void initialize(const std::vector<helper::vec3>& pos, const std::vector<int>& cells);
        bool setHandle(int mainHandle, int handleSize, int unconstrainedSize);
        bool updateDeformation(double dx, double dy, double dz);

        void getHandles(std::vector<int>& handles, std::vector<int>& unconstrained);
        const std::vector<helper::vec3>& getPositions(bool original = true) const;

    protected:
        typedef std::vector<std::vector<int>> AdjacancyList;
        AdjacancyList computeAdjacancy(int numVertices, const std::vector<int>& cells);

        std::map<int, std::vector<int>> _sharedToOriginIndexMap;
        std::map<int, int> _sharedIndexMap;
        std::vector<helper::vec3> _positions, _positions0;
        std::vector<int> _cells, _combined;
        AdjacancyList _adjacancies;

        std::vector<int> _handles;
        std::vector<int> _unconstrained;
        int _boundaryBegin, _updatingCount;
    };
}

#endif
