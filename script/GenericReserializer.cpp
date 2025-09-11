#include "GenericReserializer.h"
using namespace osgVerse;

void SerializerVisitor::traverse(WrappableSerializer& obj)
{
    if (_propertyList.empty() || !_manager) return;
    Rewrapper* r = _manager->getRewrapper(obj._type);
    if (r != NULL) r->accept(_manager, *this, _inputVersion, true);
}

void SerializerVisitor::applyValue(BaseSerializer& obj)
{
    std::type_index typeID(typeid(obj));
    SerializerMap::iterator it = _registry.find(typeID);
    if (it != _registry.end()) it->second(obj);
    else std::cerr << "[SerializerVisitor] No callback for " << typeID.name() << "\n";
}

void Rewrapper::splitAssociates(const std::string& src)
{
    std::string::size_type start = src.find_first_not_of(' ');
    while (start != std::string::npos)
    {
        std::string::size_type end = src.find_first_of(' ', start);
        if (end != std::string::npos)
        {
            _associates.push_back(WrapperAssociate(std::string(src, start, end - start)));
            start = src.find_first_not_of(' ', end);
        }
        else
        {
            _associates.push_back(WrapperAssociate(std::string(src, start, src.size() - start)));
            start = end;
        }
    }
}

void Rewrapper::add(const std::string& n, BaseSerializer* s)
{
    s->_firstVersion = _version;
    _serializers.push_back(SerializerPair(n, s));
}

void Rewrapper::clear()
{ _serializers.clear(); }

void Rewrapper::markSerializerAsRemoved(const std::string& name)
{
    for (std::vector<SerializerPair>::iterator itr = _serializers.begin();
         itr != _serializers.end(); ++itr)
    { if (itr->first == name) itr->second->_lastVersion = _version - 1; }
}

void Rewrapper::markAssociateAsRemoved(const std::string& name)
{
    for (std::vector<WrapperAssociate>::iterator itr = _associates.begin();
         itr != _associates.end(); ++itr)
    { if (itr->_name == name) {itr->_lastVersion = _version - 1; return;} }
}

void Rewrapper::markAssociateAsAdded(const std::string& name)
{
    for (std::vector<WrapperAssociate>::iterator itr = _associates.begin();
         itr != _associates.end(); ++itr)
    { if (itr->_name == name) {itr->_firstVersion = _version; return;} }
}

void Rewrapper::accept(RewrapperManager* manager, SerializerVisitor& v,
                       int inputVersion, bool includingAssociates)
{
    v.setManager(manager, inputVersion);
    if (!includingAssociates)
    {
        for (std::vector<SerializerPair>::iterator itr = _serializers.begin();
             itr != _serializers.end(); ++itr)
        {
            BaseSerializer* s = itr->second; if (!s) continue;
            if (s->_firstVersion <= inputVersion &&
                inputVersion <= s->_lastVersion) s->accept(itr->first, v);
        }
        return;
    }

    for (std::vector<WrapperAssociate>::iterator itr = _associates.begin();
         itr != _associates.end(); ++itr)
    {
        if (itr->_firstVersion <= inputVersion && inputVersion <= itr->_lastVersion)
        {
            Rewrapper* r = manager->getRewrapper(itr->_name);
            if (r != NULL) r->accept(manager, v, inputVersion, false);
            else { std::cerr << "[Rewrapper] Associated wrapper " << itr->_name << " not found\n"; }
        }
    }
}

std::shared_ptr<RewrapperManager> RewrapperManager::instance()
{
    static std::shared_ptr<RewrapperManager> instance =
        std::make_shared<RewrapperManager>(); return instance;
}

RewrapperManager::~RewrapperManager()
{
}

void RewrapperManager::addRewrapper(const std::string& name, Rewrapper* r)
{
    RewrapperMap::iterator itr = _rewrappers.find(name); _rewrappers[name] = r;
}

void RewrapperManager::removeRewrapper(const std::string& name)
{
    RewrapperMap::iterator itr = _rewrappers.find(name);
    if (itr != _rewrappers.end()) { _rewrappers.erase(itr); }
}

Rewrapper* RewrapperManager::getRewrapper(const std::string& name)
{
    RewrapperMap::iterator itr = _rewrappers.find(name);
    if (itr != _rewrappers.end()) return itr->second; else return NULL;
}

RegisterWrapperProxy::RegisterWrapperProxy(
        const std::string& name, const std::string& associates, AddPropFunc func)
{
    Rewrapper* r = new Rewrapper; r->setAssociates(associates); if (func) (*func)(*r);
    RewrapperManager::instance()->addRewrapper(name, r); _className = name;
}

RegisterWrapperProxy::~RegisterWrapperProxy()
{ RewrapperManager::instance()->removeRewrapper(_className); }
