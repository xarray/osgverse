#include <osg/Geode>
#include <osg/ProxyNode>
#include <osgDB/ConvertUTF>
#include <osgDB/WriteFile>
//#include <spatialindex/SpatialIndex.h>
#include "SymbolManager.h"

#define RES 512
#define RESV "512"
using namespace osgVerse;

SymbolManager::SymbolManager()
    : _idCounter(0), _firstRun(true)
{
    osg::Image* posImage = new osg::Image;
    posImage->allocateImage(RES, RES, 1, GL_RGBA, GL_FLOAT);
    posImage->setInternalTextureFormat(GL_RGBA32F_ARB);

    osg::Image* posImage2 = new osg::Image;
    posImage2->allocateImage(RES, RES, 1, GL_RGBA, GL_FLOAT);
    posImage2->setInternalTextureFormat(GL_RGBA32F_ARB);

    osg::Image* dirImage = new osg::Image;
    dirImage->allocateImage(RES, RES, 1, GL_RGBA, GL_FLOAT);
    dirImage->setInternalTextureFormat(GL_RGBA32F_ARB);

    _posTexture = new osg::Texture2D;
    _posTexture->setImage(posImage);
    _posTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    _posTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    _posTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
    _posTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_BORDER);
    _posTexture->setBorderColor(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));

    _posTexture2 = new osg::Texture2D;
    _posTexture2->setImage(posImage2);
    _posTexture2->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    _posTexture2->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    _posTexture2->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
    _posTexture2->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_BORDER);
    _posTexture2->setBorderColor(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));

    _dirTexture = new osg::Texture2D;
    _dirTexture->setImage(dirImage);
    _dirTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    _dirTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    _dirTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
    _dirTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_BORDER);
    _dirTexture->setBorderColor(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));

    _textTexture = new osg::Texture2D;
    _textTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    _textTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

    _drawer = new Drawer2D;
    _lodDistances[(int)LOD0] = 1e6;
    _lodDistances[(int)LOD1] = 100.0;
    _lodDistances[(int)LOD2] = 5.0;
}

void SymbolManager::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
    osg::Group* group = node->asGroup();
    if (group != NULL)
    {
        const osg::FrameStamp* fs = nv->getFrameStamp();
        if (!_camera)
        {
            OSG_WARN << "[SymbolManager] Main camera not set. "
                     << "Symbols can't work at present" << std::endl;
        }
        else if (!_firstRun) update(group, fs ? fs->getFrameNumber() : 0);
        else { initialize(group); _firstRun = false; }
    }
    traverse(node, nv);
}

int SymbolManager::updateSymbol(Symbol* sym)
{
    // TODO: add to RTree
    sym->id = _idCounter++;

    if (!sym || (sym && sym->id < 0)) return -1;
    _symbols[sym->id] = sym; return sym->id;
}

bool SymbolManager::removeSymbol(Symbol* sym)
{
    // TODO: remove from RTree
    if (!sym || (sym && sym->id < 0)) return false;
    if (_symbols.find(sym->id) != _symbols.end())
        _symbols.erase(_symbols.find(sym->id));
    return true;
}

Symbol* SymbolManager::getSymbol(int id)
{
    if (_symbols.find(id) == _symbols.end()) return NULL;
    else return _symbols[id].get();
}

const Symbol* SymbolManager::getSymbol(int id) const
{
    std::map<int, osg::ref_ptr<Symbol>>::const_iterator itr = _symbols.find(id);
    if (itr != _symbols.end()) return itr->second.get(); else return NULL;
}

std::vector<Symbol*> SymbolManager::querySymbols(const osg::Vec3d& pos, double radius) const
{
    // TODO: use RTree query
    std::vector<Symbol*> result;
    for (std::map<int, osg::ref_ptr<Symbol>>::const_iterator itr = _symbols.begin();
         itr != _symbols.end(); ++itr)
    {
        Symbol* sym = itr->second.get();
        double length = (sym->position - pos).length();
        if (length < radius) result.push_back(sym);
    }
    return result;
}

std::vector<Symbol*> SymbolManager::querySymbols(const osg::Polytope& polytope) const
{
    // TODO: use RTree query
    std::vector<Symbol*> result;
    for (std::map<int, osg::ref_ptr<Symbol>>::const_iterator itr = _symbols.begin();
         itr != _symbols.end(); ++itr)
    {
        Symbol* sym = itr->second.get();
        if (polytope.contains(sym->position)) result.push_back(sym);
    }
    return result;
}

std::vector<Symbol*> SymbolManager::querySymbols(const osg::Vec2d& proj, double e) const
{
    // TODO: use RTree query
    osg::BoundingBoxd bb;
    bb._min.set(proj[0] - e, proj[1] - e, -1.0);
    bb._max.set(proj[0] + e, proj[1] + e, 1.0);

    osg::Polytope polytope;
    polytope.setToBoundingBox(bb);
    polytope.transformProvidingInverse(
        _camera->getViewMatrix() * _camera->getProjectionMatrix());

    std::vector<Symbol*> result;
    for (std::map<int, osg::ref_ptr<Symbol>>::const_iterator itr = _symbols.begin();
         itr != _symbols.end(); ++itr)
    {
        Symbol* sym = itr->second.get();
        if (polytope.contains(sym->position)) result.push_back(sym);
    }
    return result;
}

void SymbolManager::initialize(osg::Group* group)
{
    if (!_instanceGeom)
    {   // Create default instance geometry which shows a simple triangle
        osg::Vec3Array* va = new osg::Vec3Array(3);
        (*va)[0] = osg::Vec3(0.6f, 0.0f, 0.0f);
        (*va)[1] = osg::Vec3(-0.2f, 0.2f, 0.0f);
        (*va)[2] = osg::Vec3(-0.2f, -0.2f, 0.0f);

        _instanceGeom = new osg::Geometry;
        _instanceGeom->setUseDisplayList(false);
        _instanceGeom->setUseVertexBufferObjects(true);
#if OSG_VERSION_GREATER_THAN(3, 2, 2)
        _instanceGeom->setCullingActive(false);
#endif
        _instanceGeom->setVertexArray(va);
        _instanceGeom->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, 3, 0));

        // Apply instancing shader
        osg::StateSet* ss = _instanceGeom->getOrCreateStateSet();
        {
            const char* instanceVertShader = {
                "#version 120\n"
                "#extension GL_EXT_draw_instanced : enable\n"
                "uniform sampler2D PosTexture, DirTexture;\n"
                "mat4 rotationMatrix(vec3 axis0, float angle) {\n"
                "    float s = sin(angle), c = cos(angle);\n"
                "    float oc = 1.0 - c; vec3 a = normalize(axis0);\n"
                "    return mat4(oc * a.x * a.x + c, oc * a.x * a.y - a.z * s, oc * a.z * a.x + a.y * s, 0.0,\n"
                "                oc * a.x * a.y + a.z * s, oc * a.y * a.y + c, oc * a.y * a.z - a.x * s, 0.0,\n"
                "                oc * a.z * a.x - a.y * s, oc * a.y * a.z + a.x * s, oc * a.z * a.z + c, 0.0,\n"
                "                0.0, 0.0, 0.0, 1.0);\n"
                "}\n"

                "void main() {\n"
                "    float r = float(gl_InstanceID) / " RESV ".0;\n"
                "    float c = floor(r) / " RESV ".0; r = fract(r);\n"
                "    vec4 pos = texture2D(PosTexture, vec2(r, c));\n"
                "    vec4 dir = texture2D(DirTexture, vec2(r, c));\n"

                "    gl_FrontColor = vec4(dir.xyz, 1.0);\n"
                "    vec4 v0 = vec4(gl_Vertex.xyz * pos.w, 1.0);\n"
                "    vec4 v1 = rotationMatrix(vec3(0.0, 0.0, 1.0), dir.w) * v0;\n"
                "    vec4 projP = gl_ProjectionMatrix * vec4(pos.xyz, 1.0);\n"
                "    gl_Position = vec4(v1.xyz / v1.w + projP.xyz / projP.w, 1.0);\n"
                "}"
            };

            const char* instanceFragShader = {
                "void main() {\n"
                "    gl_FragColor = gl_Color;\n"
                "}"
            };

            osg::Program* program = new osg::Program;
            program->addShader(new osg::Shader(osg::Shader::VERTEX, instanceVertShader));
            program->addShader(new osg::Shader(osg::Shader::FRAGMENT, instanceFragShader));
            ss->setAttributeAndModes(program);
        }

        // Apply default parameter textures
        ss->setTextureAttributeAndModes(0, _posTexture.get());
        ss->setTextureAttributeAndModes(1, _dirTexture.get());
        ss->addUniform(new osg::Uniform("PosTexture", (int)0));
        ss->addUniform(new osg::Uniform("DirTexture", (int)1));
    }

    if (!_instanceBoard)
    {   // Create default instance billboard for displaying text boards
        osg::Vec3Array* va = new osg::Vec3Array(4);
        (*va)[3] = osg::Vec3(-0.5f, -0.4f, 0.0f);
        (*va)[2] = osg::Vec3(0.5f, -0.4f, 0.0f);
        (*va)[1] = osg::Vec3(0.5f, 0.4f, 0.0f);
        (*va)[0] = osg::Vec3(-0.5f, 0.4f, 0.0f);
        osg::Vec2Array* ta = new osg::Vec2Array(4);
        (*ta)[0] = osg::Vec2(0.0f, 0.0f); (*ta)[1] = osg::Vec2(1.0f, 0.0f);
        (*ta)[2] = osg::Vec2(1.0f, 1.0f); (*ta)[3] = osg::Vec2(0.0f, 1.0f);

        _instanceBoard = new osg::Geometry;
        _instanceBoard->setUseDisplayList(false);
        _instanceBoard->setUseVertexBufferObjects(true);
#if OSG_VERSION_GREATER_THAN(3, 2, 2)
        _instanceBoard->setCullingActive(false);
#endif
        _instanceBoard->setVertexArray(va);
        _instanceBoard->setTexCoordArray(0, ta);
        _instanceBoard->addPrimitiveSet(new osg::DrawArrays(GL_QUADS, 0, 4, 0));

        // Apply instancing shader
        osg::StateSet* ss = _instanceBoard->getOrCreateStateSet();
        {
            const char* instanceVertShader2 = {
                "#version 120\n"
                "#extension GL_EXT_draw_instanced : enable\n"
                "uniform sampler2D PosTexture;\n"
                "uniform vec3 Offset;\n"
                "varying vec2 TexCoord;\n"
                "void main() {\n"
                "    float r = float(gl_InstanceID) / " RESV ".0;\n"
                "    float c = floor(r) / " RESV ".0; r = fract(r);\n"
                "    vec4 pos = texture2D(PosTexture, vec2(r, c));\n"

                "    float tx = float(gl_InstanceID) * Offset.z;\n"
                "    float ty = floor(tx) * Offset.z; tx = fract(tx);\n"
                "    TexCoord = vec2(tx, ty) + gl_MultiTexCoord0.xy * Offset.z;\n"

                "    gl_FrontColor = vec4(1.0);\n"
                "    vec4 v0 = vec4(gl_Vertex.xyz * pos.w, 1.0);\n"
                "    vec4 projP = gl_ProjectionMatrix * vec4(pos.xyz, 1.0);\n"
                "    gl_Position = vec4(v0.xyz / v0.w + projP.xyz / projP.w, 1.0)\n"
                "                + vec4(Offset.xy, 0.0, 0.0);\n"
                "}"
            };

            const char* instanceFragShader2 = {
                "uniform sampler2D TextTexture;\n"
                "varying vec2 TexCoord;\n"
                "void main() {\n"
                "    vec4 baseColor = texture2D(TextTexture, TexCoord);\n"
                "    gl_FragColor = baseColor * gl_Color;\n"
                "}"
            };

            osg::Program* program = new osg::Program;
            program->addShader(new osg::Shader(osg::Shader::VERTEX, instanceVertShader2));
            program->addShader(new osg::Shader(osg::Shader::FRAGMENT, instanceFragShader2));
            ss->setAttributeAndModes(program);
        }

        // Apply default parameter textures
        ss->setTextureAttributeAndModes(0, _posTexture2.get());
        ss->setTextureAttributeAndModes(1, _textTexture.get());
        ss->addUniform(new osg::Uniform("PosTexture", (int)0));
        ss->addUniform(new osg::Uniform("TextTexture", (int)1));
        ss->addUniform(new osg::Uniform("Offset", osg::Vec3(0.1f, 0.05f, 1.0f / 10.0f)));
    }

    // Add to geode
    osg::Geode* geode1 = new osg::Geode;
    geode1->setName("SymbolGeometryGeode");
    geode1->addDrawable(_instanceGeom.get());
    geode1->setCullingActive(false);

    osg::Geode* geode2 = new osg::Geode;
    geode2->setName("SymbolTextBoardGeode");
    geode2->addDrawable(_instanceBoard.get());
    geode2->setCullingActive(false);
    geode2->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    geode2->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);

    group->addChild(geode1); group->addChild(geode2);
    group->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
}

void SymbolManager::update(osg::Group* group, unsigned int frameNo)
{
    osg::BoundingBox boundBox;
    osg::Matrix viewMatrix = _camera->getViewMatrix();
    double inv1 = 1.0 / (_lodDistances[0] - _lodDistances[1]);
    double inv2 = 1.0 / (_lodDistances[1] - _lodDistances[2]);
    Symbol* nearestSym = NULL; double nearest = FLT_MAX;
    int numInstances = 0, numInstances2 = 0;

    osg::Polytope frustum;
    frustum.setToUnitFrustum(false, false);
    frustum.transformProvidingInverse(
        viewMatrix * _camera->getProjectionMatrix());

    // Use RTree to query symbols and find visible ones
    // TODO

    // Traverse all symbols
    osg::Vec4f* posHandle = (osg::Vec4f*)_posTexture->getImage()->data();
    osg::Vec4f* posHandle2 = (osg::Vec4f*)_posTexture2->getImage()->data();
    osg::Vec4f* dirHandle = (osg::Vec4f*)_dirTexture->getImage()->data();
    std::vector<std::string> texts;
    for (std::map<int, osg::ref_ptr<Symbol>>::iterator itr = _symbols.begin();
         itr != _symbols.end(); ++itr)
    {
        // Update state and eye-space position
        Symbol* sym = itr->second.get();
        osg::Vec3f eyePos = sym->position * viewMatrix;
        double distance = -eyePos.z(), interpo = 0.0, scale = sym->scale;
        if (distance < nearest) { nearest = distance; nearestSym = sym; }

        // Check distance state of each symbol
        if (distance > _lodDistances[0] || !frustum.contains(sym->position))
            sym->state = Symbol::Hidden;
        else if (distance > _lodDistances[1])
        {
            sym->state = Symbol::FarDistance;
            interpo = (distance - _lodDistances[1]) * inv1;
            scale = sym->scale * (interpo + 2.0f * (1.0 - interpo));
        }
        else if (distance > _lodDistances[2])
        {
            sym->state = Symbol::MidDistance;
            interpo = (distance - _lodDistances[2]) * inv2;
            scale = sym->scale * 2.0 * (interpo + 4.0f * (1.0 - interpo));
        }
        else
        {
            // Load or re-use symbol model
            sym->modelFrame0 = frameNo;
            if (!sym->fileName.empty())
            {
                sym->state = Symbol::NearDistance;
                updateNearDistance(sym, group);
            }
            else sym->state = Symbol::MidDistance;
        }

        if (sym->state != Symbol::NearDistance && sym->loadedModel.valid())
        {
            // If not in NearDistance mode, hide the model and see if we should delete it
            int dt = frameNo - sym->modelFrame0;
            if (dt > 120) group->removeChild(sym->loadedModel.get());
            else sym->loadedModel->setNodeMask(0);
        }

        // TODO: when convert to FarClustered?
        if (sym->state == Symbol::Hidden ||
            sym->state == Symbol::NearDistance) continue;

        // Save to parameter textures
        int y = itr->first / RES, x = itr->first % RES;
        if (y < 0 || y > (RES - 1))
        { OSG_WARN << "[SymbolManager] Data overflow!" << std::endl; break; }

        *(posHandle + numInstances) = osg::Vec4(eyePos, (float)scale);
        *(dirHandle + numInstances) = osg::Vec4(sym->color, sym->rotateAngle);
        boundBox.expandBy(sym->position); numInstances++;

        if (sym->state == Symbol::MidDistance)
        {
            *(posHandle2 + numInstances2) = osg::Vec4(eyePos, (float)scale * 3.0f);
            texts.push_back(sym->name); numInstances2++;
        }
    }

    // If only one symbol left and near enough, select it as NearDistance one
    if (numInstances == 1 && nearest < _lodDistances[1])
    {
        nearestSym->modelFrame0 = frameNo;
        if (!nearestSym->fileName.empty())
        {
            nearestSym->state = Symbol::NearDistance;
            updateNearDistance(nearestSym, group);
        }
    }

    // Draw far/middle distance symbols
    if (numInstances > 0)
    {
        osg::PrimitiveSet* p = (_instanceGeom->getNumPrimitiveSets() > 0)
                             ? _instanceGeom->getPrimitiveSet(0) : NULL;
        if (p) { p->setNumInstances(numInstances); p->dirty(); }
        _instanceGeom->setInitialBound(boundBox);
        _instanceGeom->getParent(0)->setNodeMask(0xffffffff);
        _posTexture->getImage()->dirty();
        _dirTexture->getImage()->dirty();
    }
    else
        _instanceGeom->getParent(0)->setNodeMask(0);

    // Draw middle distance text boards
    if (numInstances2 > 0)
    {
        osg::PrimitiveSet* p = (_instanceBoard->getNumPrimitiveSets() > 0)
                             ? _instanceBoard->getPrimitiveSet(0) : NULL;
        if (p) { p->setNumInstances(numInstances2); p->dirty(); }
        _instanceBoard->setInitialBound(boundBox);
        _instanceBoard->getParent(0)->setNodeMask(0xffffffff);
        _posTexture2->getImage()->dirty();

        // Collect labels and recreate texture
        _textTexture->setImage(createGrid(2048, 1024, 10, texts));
    }
    else
        _instanceBoard->getParent(0)->setNodeMask(0);
}

void SymbolManager::updateNearDistance(Symbol* sym, osg::Group* group)
{
    if (!sym->loadedModel)
    {
        osg::Vec3d dir = sym->position; dir.normalize();
        osg::Quat q; q.makeRotate(osg::Z_AXIS, osg::Vec3(dir));
        osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
        mt->setMatrix(osg::Matrix::rotate(osg::PI - sym->rotateAngle, osg::Z_AXIS) *
                      osg::Matrix::rotate(q) * osg::Matrix::translate(sym->position));

        osg::ref_ptr<osg::ProxyNode> proxy = new osg::ProxyNode;
        proxy->setFileName(0, sym->fileName); mt->addChild(proxy.get());
        group->addChild(mt.get()); sym->loadedModel = mt.get();
    }
    else
        sym->loadedModel->setNodeMask(0xffffffff);
}

osg::Image* SymbolManager::createLabel(int w, int h, const std::string& text,
                                       const osg::Vec4& color)
{
    _drawer->allocateImage(w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    _drawer->start(false);
    _drawer->fillBackground(osg::Vec4(0.6f, 0.6f, 0.8f, 0.6f));

    std::vector<std::string> lines;
    osgDB::split(text, lines, '\n');

    float yStep = (float)h / (float)(lines.size() + 1);
    float size = yStep * 0.6f;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        std::wstring t = osgDB::convertUTF8toUTF16(lines[i]);
        _drawer->drawText(osg::Vec2(20.0f, yStep * (i + 1)), size, t, "",
                          Drawer2D::StyleData(color, true));
    }
    _drawer->finish();
    return (osg::Image*)_drawer->clone(osg::CopyOp::DEEP_COPY_ALL);
}

osg::Image* SymbolManager::createGrid(int w, int h, int grid,
                                      const std::vector<std::string>& texts,
                                      const osg::Vec4& color)
{
    if (w != _drawer->s() || h != _drawer->t())
        _drawer->allocateImage(w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    _drawer->start(false);
    _drawer->clear();

    int stepW = w / grid, stepH = h / grid;
    size_t numText = texts.size();
    for (size_t j = 0; j < numText; ++j)
    {
        int tx = j % grid, ty = j / grid;
        std::vector<std::string> lines;
        osgDB::split(texts[j], lines, '\n');

        float x = stepW * tx + 10.0f, y = stepH * ty + 30.0f;
        for (size_t i = 0; i < lines.size(); ++i)
        {
            std::wstring t = osgDB::convertUTF8toUTF16(lines[i]);
            _drawer->drawText(osg::Vec2(x, y + i * 40.0f), 30.0f, t, "",
                              Drawer2D::StyleData(color, true));
        }
    }
    _drawer->finish();
    return (osg::Image*)_drawer->clone(osg::CopyOp::DEEP_COPY_ALL);
}
