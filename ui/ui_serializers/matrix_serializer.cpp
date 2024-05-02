#include "../SerializerInterface.h"
#include <osg/Matrix>
using namespace osgVerse;

template<typename T>
class MatrixSerializerInterface : public SerializerInterface
{
public:
    MatrixSerializerInterface(osg::Object* obj, LibraryEntry* entry, const LibraryEntry::Property& prop)
        : SerializerInterface(obj, entry, prop, true)
    {
        _check = new CheckBox(TR(_property.name) + _postfix, false);
        _check->tooltip = prop.ownerClass + "::set" + prop.name + "()";
        //_check->callback = [this](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase*)
        //{ _entry->setProperty(_object.get(), _property.name, _check->value); };
    }

    virtual bool showProperty(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        //if (isDirty()) _entry->getProperty(_object.get(), _property.name, _check->value);
        return _check->show(mgr, content);
    }

protected:
    osg::ref_ptr<CheckBox> _check;
};

typedef MatrixSerializerInterface<osg::Matrix> Matrix0SerializerInterface;
REGISTER_SERIALIZER_INTERFACE(RW_MATRIX, Matrix0SerializerInterface)

#if OSG_VERSION_GREATER_THAN(3, 4, 0)
typedef MatrixSerializerInterface<osg::Matrixf> MatrixfSerializerInterface;
typedef MatrixSerializerInterface<osg::Matrixd> MatrixdSerializerInterface;

REGISTER_SERIALIZER_INTERFACE(RW_MATRIXF, MatrixfSerializerInterface)
REGISTER_SERIALIZER_INTERFACE(RW_MATRIXD, MatrixdSerializerInterface)
#endif
