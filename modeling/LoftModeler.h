#ifndef MANA_MODELING__PIPEMODELER
#define MANA_MODELING__PIPEMODELER

#include <osg/Geometry>

namespace osgVerse
{

    /** The modeler for creating pipes */
    class LoftModeler : public osg::Referenced
    {
    public:
        struct ShapeData
        {
            ShapeData(const std::vector<osg::Vec3>& sect, const osg::Vec3& c, const osg::Vec3& d, double r)
                : _section(sect), _center(c), _direction(d), _planarRotation(r), _ratio(0.0)
            { _direction.normalize(); }

            std::vector<osg::Vec3> _section;
            osg::Vec3 _center, _direction;
            double _planarRotation, _ratio;
        };

        typedef std::vector<osg::Vec3> VertexList;
        typedef std::vector<ShapeData> ShapeList;
        LoftModeler();

        /** Set max texture coordinates (S and T) of the solid geometry */
        void setTexCoordRange(const osg::Vec2& range) { _texCoordRange = range; }
        const osg::Vec2& getTexCoordRange() const { return _texCoordRange; }

        /** Set color of the wireframe geometry, can be used to clear selected color */
        void setWireframeColor(const osg::Vec4& color);
        osg::Vec4 getWireframeColor() const { return _wirframeColor; }

        /** Set color of a specific section of the wireframe geometry (will be cleared when inserting/removing) */
        void selectOnWireframe(unsigned int pos, const osg::Vec4& color);

        /** Add a center path directly with a constant shape */
        void addSections(const VertexList& va, const VertexList& centers);

        /** Add a new section with a center point from previous section */
        void addSection(const VertexList& va, const osg::Vec3& center,
                        const osg::Vec3& direction = osg::Z_AXIS, double planarRot = 0.0);

        /** Use the last section shape to extrude again */
        void addSection(const osg::Vec3& center, const osg::Vec3& direction = osg::Z_AXIS, double planarRot = 0.0);

        /** Insert a section at a position */
        void insertSection(unsigned int pos, const VertexList& va, const osg::Vec3& center,
                           const osg::Vec3& direction = osg::Z_AXIS, double planarRot = 0.0);

        /** Remove one section */
        void removeSection(unsigned int pos);

        /** Set section shape */
        void setSection(unsigned int pos, const VertexList& va);

        /** Set section center */
        void setCenter(unsigned int pos, const osg::Vec3& center, const osg::Vec3& direction = osg::Z_AXIS);

        /** Set section planar rotation along its direction */
        void setPlanarRotation(unsigned int pos, double planarRot);

        /** Get number of shapes */
        unsigned int getNumShapes() const { return _shapes.size(); }

        /** Get section vertex list of a shape */
        VertexList& getSection(unsigned int i) { return _shapes[i]._section; }
        const VertexList& getSection(unsigned int i) const { return _shapes[i]._section; }

        /** Get center point of a shape */
        osg::Vec3& getCenterPoint(unsigned int i) { return _shapes[i]._center; }
        const osg::Vec3& getCenterPoint(unsigned int i) const { return _shapes[i]._center; }

        /** Get center direction of a shape */
        osg::Vec3& getCenterDir(unsigned int i) { return _shapes[i]._direction; }
        const osg::Vec3& getCenterDir(unsigned int i) const { return _shapes[i]._direction; }

        /** Get center direction of a shape */
        double getPlanarRotation(unsigned int i) const { return _shapes[i]._planarRotation; }

        /** Find nearest shape index from a point */
        unsigned int findShapeIndex(const osg::Vec3& point);

        /** Result geometries to be outputted */
        osg::Geometry* getSolidResult() { return _solidGeom.get(); }
        osg::Geometry* getWireframeResult() { return _wireframeGeom.get(); }

    protected:
        virtual ~LoftModeler();

        void setGeoemtryData(unsigned int pos, const ShapeData& data);
        void insertGeoemtryData(unsigned int pos, const ShapeData& data);
        void removeGeoemtryData(unsigned int pos);

        void updateRatios();
        void updateVertices(unsigned int pos, const ShapeData& data);
        void updatePrimitiveSets();
        void updateBound();

        osg::ref_ptr<osg::Geometry> _solidGeom;
        osg::ref_ptr<osg::Geometry> _wireframeGeom;
        osg::ref_ptr<osg::Vec3Array> _vertices;
        osg::ref_ptr<osg::Vec2Array> _texCoords;
        osg::ref_ptr<osg::Vec3Array> _normals;

        ShapeList _shapes;
        osg::Vec4 _wirframeColor;
        osg::Vec2 _texCoordRange;
        unsigned int _segments;
    };

}

#endif
