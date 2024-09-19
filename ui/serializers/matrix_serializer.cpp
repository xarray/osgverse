#include "../SerializerInterface.h"
#include "../../modeling/Math.h"
#include <osg/Matrix>
using namespace osgVerse;

template<typename T>
class MatrixSerializerInterface : public SerializerInterface
{
public:
    MatrixSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
        : SerializerInterface(obj, entry, prop, true), _numToShow(3), _matType(TransformMatrix)
    {
        _vector[0] = new InputVectorField(TR("T") + _postfix);
        _vector[1] = new InputVectorField(TR("R") + _postfix);
        _vector[2] = new InputVectorField(TR("S") + _postfix);
        updateVectorFields(_matrixValue);

        _vector[0]->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        {
            osg::Vec3d value; _vector[0]->getVector(value); applyToMatrix(0, value);
            _entry->setProperty(_object.get(), _property.name, _matrixValue);
        };

        _vector[1]->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        {
            osg::Vec3d value; _vector[1]->getVector(value); applyToMatrix(1, value);
            _entry->setProperty(_object.get(), _property.name, _matrixValue);
        };

        _vector[2]->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        {
            osg::Vec3d value; _vector[2]->getVector(value); applyToMatrix(2, value);
            _entry->setProperty(_object.get(), _property.name, _matrixValue);
        };
    }

    virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        if (isDirty())
        {
            _entry->getProperty(_object.get(), _property.name, _matrixValue);
            updateVectorFields(_matrixValue);
        }

        bool edited = false;
        for (int i = 0; i < _numToShow; ++i) edited |= _vector[i]->show(mgr, content);
        return edited;
    }

protected:
    enum MatrixType
    {
        TransformMatrix, LookAtMatrix, OrthoMatrix,
        PerspectiveMatrix, Generic2x2, Generic3x3
    };

    void applyToMatrix(unsigned int index, const osg::Vec3d& value)
    {
        switch (_matType)
        {
        case LookAtMatrix:
            if (index < 3)
            {
                osg::Vec3d eye, center, up; _matrixValue.getLookAt(eye, center, up, 100.0f);
                if (index == 0) eye = value; else if (index == 1) center = value;
                else { up = value; up.normalize(); }

                osg::Vec3d dir = center - eye; double length = dir.normalize();
                osg::Vec3d side = up ^ dir; up = dir ^ side;
                _matrixValue = osg::Matrix::lookAt(eye, center, up);
            }
            break;
        case OrthoMatrix:
            if (index < 2)
            {
                double l = 0.0, r = 0.0, b = 0.0, t = 0.0, zn = 0.0, zf = 0.0;
                _matrixValue.getOrtho(l, r, b, t, zn, zf);

                if (index == 1) { zn = value[0]; zf = value[1]; }
                else
                {
                    double sizeX = value[0] * 0.5, sizeY = value[1] * 0.5;
                    l = -sizeX; r = sizeX; b = -sizeY; t = sizeY;
                }
                _matrixValue = osg::Matrix::ortho(l, r, b, t, zn, zf);
            }
            break;
        case PerspectiveMatrix:
            if (index < 2)
            {
                double fov = 0.0, aspectRatio = 0.0, zn = 0.0, zf = 0.0;
                _matrixValue.getPerspective(fov, aspectRatio, zn, zf);

                if (index == 1) { zn = value[0]; zf = value[1]; }
                else { fov = value[0]; aspectRatio = value[1] / value[2]; }
                _matrixValue = osg::Matrix::perspective(fov, aspectRatio, zn, zf);
            }
            break;
        case Generic2x2:
            for (int i = 0; i < 2; ++i) _matrixValue(index, i) = value[i]; break;
        case Generic3x3:
            for (int i = 0; i < 3; ++i) _matrixValue(index, i) = value[i]; break;
        default:
            if (index == 0) _matrixValue.setTrans(value);
            else
            {
                osg::Vec3d pos, sc; osg::Quat rot, so; osg::Matrixd rMat;
                _matrixValue.decompose(pos, rot, sc, so); rMat = osg::Matrix::rotate(rot);
                if (index == 1)
                {
                    rMat = osg::Matrixd::rotate(osg::inDegrees(value[2]), osg::Z_AXIS)
                         * osg::Matrixd::rotate(osg::inDegrees(value[0]), osg::X_AXIS)
                         * osg::Matrixd::rotate(osg::inDegrees(value[1]), osg::Y_AXIS);
                }
                else sc = value;
                _matrixValue = osg::Matrix::scale(sc) * rMat * osg::Matrix::translate(pos);
            }
            break;
        }
    }

    void updateVectorFields(const T& matrix)
    {
        switch (_matType)
        {
        case LookAtMatrix:
            _vector[0]->name = TR("Eye") + _postfix; _vector[0]->vecNumber = 3;
            _vector[0]->tooltip = tooltip(_property, "Eye");
            _vector[1]->name = TR("Forward") + _postfix; _vector[1]->vecNumber = 3;
            _vector[1]->tooltip = tooltip(_property, "Forward Direction");
            _vector[2]->name = TR("Up") + _postfix; _vector[2]->vecNumber = 3;
            _vector[2]->tooltip = tooltip(_property, "Up Direction");
            {
                osg::Vec3d eye, center, up; osg::Matrixd(matrix).getLookAt(eye, center, up, 100.0f);
                _vector[0]->setVector(eye); _vector[2]->setVector(up);
                center = center - eye; center.normalize(); _vector[1]->setVector(center);
            }
            _numToShow = 3; break;
        case OrthoMatrix:
            _vector[0]->name = TR("Size") + _postfix; _vector[0]->vecNumber = 2;
            _vector[0]->tooltip = tooltip(_property, "Size-X & Size-Y");
            _vector[1]->name = TR("NearFar") + _postfix; _vector[1]->vecNumber = 2;
            _vector[1]->tooltip = tooltip(_property, "Near & Far");
            {
                double l = 0.0, r = 0.0, b = 0.0, t = 0.0, zn = 0.0, zf = 0.0;
                osg::Matrixd(matrix).getOrtho(l, r, b, t, zn, zf);
                _vector[0]->setVector(osg::Vec2d(r - l, t - b)); _vector[1]->setVector(osg::Vec2d(zn, zf));
            }
            _numToShow = 2; break;
        case PerspectiveMatrix:
            _vector[0]->name = TR("Persp") + _postfix; _vector[0]->vecNumber = 3;
            _vector[0]->tooltip = tooltip(_property, "FOV, Width & Height");
            _vector[1]->name = TR("NearFar") + _postfix; _vector[1]->vecNumber = 2;
            _vector[1]->tooltip = tooltip(_property, "Near & Far");
            {
                double fov = 0.0, aspectRatio = 0.0, zn = 0.0, zf = 0.0;
                osg::Matrixd(matrix).getPerspective(fov, aspectRatio, zn, zf);
                _vector[0]->setVector(osg::Vec3d(fov, aspectRatio * 1080.0, 1080.0));
                _vector[1]->setVector(osg::Vec2d(zn, zf));
            }
            _numToShow = 2; break;
        case Generic2x2:
            _vector[0]->name = TR("Row0") + _postfix; _vector[0]->vecNumber = 2;
            _vector[0]->tooltip = tooltip(_property, "Row0");
            _vector[0]->setVector(osg::Vec2d(matrix(0, 0), matrix(0, 1)));
            _vector[1]->name = TR("Row1") + _postfix; _vector[1]->vecNumber = 2;
            _vector[1]->tooltip = tooltip(_property, "Row1");
            _vector[1]->setVector(osg::Vec2d(matrix(1, 0), matrix(1, 1)));
            _numToShow = 2; break;
        case Generic3x3:
            _vector[0]->name = TR("Row0") + _postfix; _vector[0]->vecNumber = 3;
            _vector[0]->tooltip = tooltip(_property, "Row0");
            _vector[0]->setVector(osg::Vec3d(matrix(0, 0), matrix(0, 1), matrix(0, 2)));
            _vector[1]->name = TR("Row1") + _postfix; _vector[1]->vecNumber = 3;
            _vector[1]->tooltip = tooltip(_property, "Row1");
            _vector[1]->setVector(osg::Vec3d(matrix(1, 0), matrix(1, 1), matrix(1, 2)));
            _vector[2]->name = TR("Row2") + _postfix; _vector[2]->vecNumber = 3;
            _vector[2]->tooltip = tooltip(_property, "Row2");
            _vector[2]->setVector(osg::Vec3d(matrix(2, 0), matrix(2, 1), matrix(2, 2)));
            _numToShow = 3; break;
        default:
            _vector[0]->name = TR("T") + _postfix; _vector[0]->vecNumber = 3;
            _vector[0]->tooltip = tooltip(_property, "Translation");
            _vector[1]->name = TR("R") + _postfix; _vector[1]->vecNumber = 3;
            _vector[1]->tooltip = tooltip(_property, "Rotation");
            _vector[2]->name = TR("S") + _postfix; _vector[2]->vecNumber = 3;
            _vector[2]->tooltip = tooltip(_property, "Scale");
            {
                osg::Vec3d pos, sc; osg::Quat rot, so; osg::Matrixd(matrix).decompose(pos, rot, sc, so);
                osg::Vec3d eulers = computeHPRFromQuat(rot);
                _vector[0]->setVector(pos); _vector[2]->setVector(sc);
                _vector[1]->setVector(osg::Vec3d(osg::RadiansToDegrees(eulers[1]),
                                                 osg::RadiansToDegrees(eulers[2]),
                                                 osg::RadiansToDegrees(eulers[0])));
            }
            _numToShow = 3; break;
        }
    }

    /* TransformMatrix  : Translation (vec3); Rotation (vec3); Scale (vec3)
       LookAtMatrix     : Eye (vec3); Forward (vec3); Up (vec3)
       OrthoMatrix      : SizeX, SizeY (vec2); Near, Far (vec2)
       PerspectiveMatrix: Aspect Ratio, Width, Height (vec3); Near, Far (vec2) */
    osg::ref_ptr<InputVectorField> _vector[3];
    T _matrixValue; int _numToShow;
    MatrixType _matType;
};

typedef MatrixSerializerInterface<osg::Matrix> Matrix0SerializerInterface;
REGISTER_SERIALIZER_INTERFACE(MATRIX, Matrix0SerializerInterface)

#if OSG_VERSION_GREATER_THAN(3, 4, 0)
typedef MatrixSerializerInterface<osg::Matrixf> MatrixfSerializerInterface;
typedef MatrixSerializerInterface<osg::Matrixd> MatrixdSerializerInterface;

REGISTER_SERIALIZER_INTERFACE(MATRIXF, MatrixfSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(MATRIXD, MatrixdSerializerInterface)
#endif
