#include <osg/Version>
#include <osg/Math>
#include <osg/io_utils>
#include <wrappers/Export.h>
#include <script/GenericReserializer.h>
#include <iostream>
#include <sstream>

class ReserializeVisitor : public osgVerse::SerializerVisitor
{
public:
    ReserializeVisitor() : _indent(2)
    {
        registerType<int>([this](osgVerse::ValueSerializer<int>& s) { applyInt(s); });
        registerType<unsigned int>([this](osgVerse::ValueSerializer<unsigned int>& s) { applyUInt(s); });
        registerType<float>([this](osgVerse::ValueSerializer<float>& s) { applyFloat(s); });
        registerType<double>([this](osgVerse::ValueSerializer<double>& s) { applyDouble(s); });
        registerType<osg::Vec2>([this](osgVerse::ValueSerializer<osg::Vec2>& s) { applyVec2f(s); });
        registerType<osg::Vec3>([this](osgVerse::ValueSerializer<osg::Vec3>& s) { applyVec3f(s); });
        registerType<osg::Vec4>([this](osgVerse::ValueSerializer<osg::Vec4>& s) { applyVec4f(s); });
        registerType<osg::Vec2d>([this](osgVerse::ValueSerializer<osg::Vec2d>& s) { applyVec2d(s); });
        registerType<osg::Vec3d>([this](osgVerse::ValueSerializer<osg::Vec3d>& s) { applyVec3d(s); });
        registerType<osg::Vec4d>([this](osgVerse::ValueSerializer<osg::Vec4d>& s) { applyVec4d(s); });
        registerType<osg::Quat>([this](osgVerse::ValueSerializer<osg::Quat>& s) { applyQuat(s); });
        registerType<osg::Matrix>([this](osgVerse::ValueSerializer<osg::Matrix>& s) { applyMatrix(s); });
    }
    
    void applyInt(osgVerse::ValueSerializer<int>& obj) { propAndIndent() << ": Int " << obj._v0 << "\n"; }
    void applyUInt(osgVerse::ValueSerializer<unsigned int>& obj) { propAndIndent() << ": UInt " << obj._v0 << "\n"; }
    void applyFloat(osgVerse::ValueSerializer<float>& obj) { propAndIndent() << ": Float " << obj._v0 << "\n"; }
    void applyDouble(osgVerse::ValueSerializer<double>& obj) { propAndIndent() << ": Double " << obj._v0 << "\n"; }
    void applyVec2f(osgVerse::ValueSerializer<osg::Vec2f>& obj) { propAndIndent() << ": Vec2f " << obj._v0 << "\n"; }
    void applyVec3f(osgVerse::ValueSerializer<osg::Vec3f>& obj) { propAndIndent() << ": Vec3f " << obj._v0 << "\n"; }
    void applyVec4f(osgVerse::ValueSerializer<osg::Vec4f>& obj) { propAndIndent() << ": Vec4f " << obj._v0 << "\n"; }
    void applyVec2d(osgVerse::ValueSerializer<osg::Vec2d>& obj) { propAndIndent() << ": Vec2d " << obj._v0 << "\n"; }
    void applyVec3d(osgVerse::ValueSerializer<osg::Vec3d>& obj) { propAndIndent() << ": Vec3d " << obj._v0 << "\n"; }
    void applyVec4d(osgVerse::ValueSerializer<osg::Vec4d>& obj) { propAndIndent() << ": Vec4d " << obj._v0 << "\n"; }
    void applyQuat(osgVerse::ValueSerializer<osg::Quat>& obj) { propAndIndent() << ": Quat " << obj._v0 << "\n"; }
    void applyMatrix(osgVerse::ValueSerializer<osg::Matrix>& obj) { propAndIndent() << ": Matrix\n"; }

    virtual void apply(osgVerse::ObjectSerializer& obj)
    {
        propAndIndent() << ": Object " << obj._type << "\n";
        _indent += 2; traverse(obj); _indent -= 2;
    }

    virtual void apply(osgVerse::ImageSerializer& obj)
    {
        propAndIndent() << ": Image " << obj._type << "\n";
        _indent += 2; traverse(obj); _indent -= 2;
    }

    virtual void apply(osgVerse::VectorSerializer& obj)
    { propAndIndent() << ": " << obj._type << "\n"; }

    virtual void apply(osgVerse::UserSerializer& obj)
    { propAndIndent() << ": User\n"; }

    virtual void apply(osgVerse::BoolSerializer& obj)
    { propAndIndent() << ": Bool " << obj._v0 << "\n"; }

    virtual void apply(osgVerse::StringSerializer& obj)
    { propAndIndent() << ": String " << obj._v0 << "\n"; }

    virtual void apply(osgVerse::EnumSerializer& obj)
    { propAndIndent() << ": Enum " << obj._v0 << "\n"; }

    virtual void apply(osgVerse::GLenumSerializer& obj)
    { propAndIndent() << ": " << obj._type << " " << obj._v0 << "\n"; }

protected:
    std::ostream& propAndIndent()
    {
        for (int i = 0; i < _indent; ++i) std::cout << " ";
        std::cout << (_propertyList.empty() ? "NULL" : _propertyList.top());
        return std::cout;
    }
    int _indent;
};

int main(int argc, char** argv)
{
    osgVerse::RewrapperManager* manager = osgVerse::loadRewrappers();
    for (osgVerse::RewrapperManager::RewrapperMap::const_iterator it = manager->getRewrappers().begin();
         it != manager->getRewrappers().end(); ++it)
    {
        osgVerse::Rewrapper* rewrapper = const_cast<osgVerse::Rewrapper*>(it->second.get());
        std::cout << "Rewrapper " << it->first << "\n";
        ReserializeVisitor rv; rewrapper->accept(manager, rv, OPENSCENEGRAPH_SOVERSION);
    }
    return 0;
}
