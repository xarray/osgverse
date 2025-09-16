#include <osg/Geode>
#include <osg/Billboard>
#include <osg/ProxyNode>
#include <osgDB/ConvertUTF>
#include <osgDB/WriteFile>
#include <3rdparty/dkm_parallel.hpp>
#include "Pipeline.h"
#include "SymbolManager.h"

#define RES 512
using namespace osgVerse;

static osg::Texture2D* createParameterTable(osg::Image* image)
{
    image->allocateImage(RES, RES, 1, GL_RGBA, GL_FLOAT);
    image->setInternalTextureFormat(GL_RGBA32F_ARB);
    memset(image->data(), 0, image->getTotalSizeInBytes());

    osg::Texture2D* tex = new osg::Texture2D; tex->setImage(image);
    tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    tex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
    tex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_BORDER);
    tex->setBorderColor(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f)); return tex;
}

osg::Vec3 Symbol::getCorner2D(SymbolManager* mgr, int index) const
{
    osg::Viewport* vp = mgr->getMainCamera()->getViewport();
    osg::Vec3 midScale = mgr->getMidDistanceTextScale(),
              midOffset = mgr->getMidDistanceTextOffset(),
              pos = osg::Vec3(projAndScale[0], projAndScale[1], projAndScale[2]);
    float aspect = (vp ? ((float)vp->height() / (float)vp->width()) : 1.0f);
    float halfPY = 0.5f, scaleY = projAndScale[3]; float scaleX = aspect * scaleY;
    float halfPX = halfPY, halfNX = halfPX, halfNY = halfPY;
    if (state > FarDistance)
    {
        float midW = midScale[0], midH = midScale[1];
        halfPX = osg::maximum(midW * 0.5f + midW * midOffset[0], halfPX);
        halfPY = osg::maximum(midH * 0.5f + midH * midOffset[1], halfPY);
        halfNX = osg::maximum(midW * 0.5f - midW * midOffset[0], halfNX);
        halfNY = osg::maximum(midH * 0.5f - midH * midOffset[1], halfNY);
    }

    switch (index)
    {
    case 0: return pos + osg::Vec3(-halfNX * scaleX, -halfNY * scaleY, 0.0f);
    case 1: return pos + osg::Vec3(halfPX * scaleX, -halfNY * scaleY, 0.0f);
    case 2: return pos + osg::Vec3(halfPX * scaleX, halfPY * scaleY, 0.0f);
    case 3: return pos + osg::Vec3(-halfNX * scaleX, halfPY * scaleY, 0.0f);
    default: return pos;
    }
}

SymbolManager::SymbolManager()
    : _idCounter(0), _firstRun(true), _showIconsInMidDistance(true)
{
    osg::Image* posImage = new osg::Image;
    osg::Image* posImage2 = new osg::Image;
    osg::Image* dirImage = new osg::Image;
    osg::Image* dirImage2 = new osg::Image;
    osg::Image* colorImage = new osg::Image;
    osg::Image* colorImage2 = new osg::Image;

    osg::Image* emptyImage = new osg::Image;
    emptyImage->allocateImage(1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    memset(emptyImage->data(), 0, emptyImage->getTotalSizeInBytes());

    _posTexture = createParameterTable(posImage);
    _posTexture2 = createParameterTable(posImage2);
    _dirTexture = createParameterTable(dirImage);
    _dirTexture2 = createParameterTable(dirImage2);
    _colorTexture = createParameterTable(colorImage);
    _colorTexture2 = createParameterTable(colorImage2);

    _iconTexture = new osg::Texture2D; _iconTexture->setResizeNonPowerOfTwoHint(false);
    _iconTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
    _iconTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    _iconTexture->setImage(emptyImage);

    _bgIconTexture = new osg::Texture2D; _bgIconTexture->setResizeNonPowerOfTwoHint(false);
    _bgIconTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
    _bgIconTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    _bgIconTexture->setImage(emptyImage);

    _textTexture = new osg::Texture2D; _textTexture->setResizeNonPowerOfTwoHint(false);
    _textTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
    _textTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

    _drawer = new Drawer2D; _lodIconScaleFactor.set(1.5f, 1.4f, 1.0f);
    _lodDistances[(int)LOD0] = 1e6; _lodDistances[(int)LOD1] = 100.0; _lodDistances[(int)LOD2] = 5.0;
    _midDistanceOffset = new osg::Uniform("Offset", osg::Vec3(2.0f, 0.0f, -0.001f));
    _midDistanceScale = new osg::Uniform("Scale", osg::Vec3(3.0f, 1.0f, 1.0f / 10.0f));
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
    if (sym && sym->id < 0) sym->id = _idCounter++;

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
    osg::BoundingBox bb;
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

void SymbolManager::setShaders(osg::Shader* vs, osg::Shader* fs)
{
    _farDistanceProgram = new osg::Program; _farDistanceProgram->setName("FarDistanceProgram");
    _midDistanceProgram = new osg::Program; _midDistanceProgram->setName("MidDistanceProgram");
    _farDistanceProgram->addShader(vs); _farDistanceProgram->addShader(fs);
    _midDistanceProgram->addShader(vs); _midDistanceProgram->addShader(fs);
    if (vs) Pipeline::createShaderDefinitions(vs, 100, 130);
    if (fs) Pipeline::createShaderDefinitions(fs, 100, 130);  // FIXME
}

void SymbolManager::initialize(osg::Group* group)
{
    if (!_instanceGeom)
    {   // Create default instance geometry which shows a simple quad
        osg::Vec3Array* va = new osg::Vec3Array(4);
        (*va)[0] = osg::Vec3(-0.5f, -0.5f, 0.0f);
        (*va)[1] = osg::Vec3(0.5f, -0.5f, 0.0f);
        (*va)[2] = osg::Vec3(0.5f, 0.5f, 0.0f);
        (*va)[3] = osg::Vec3(-0.5f, 0.5f, 0.0f);
        osg::Vec2Array* ta = new osg::Vec2Array(4);
        (*ta)[0] = osg::Vec2(0.0f, 0.0f); (*ta)[1] = osg::Vec2(1.0f, 0.0f);
        (*ta)[2] = osg::Vec2(1.0f, 1.0f); (*ta)[3] = osg::Vec2(0.0f, 1.0f);

        _instanceGeom = new osg::Geometry;
        _instanceGeom->setUseDisplayList(false);
        _instanceGeom->setUseVertexBufferObjects(true);
#if OSG_VERSION_GREATER_THAN(3, 2, 3)
        _instanceGeom->setCullingActive(false);
#endif
        _instanceGeom->setVertexArray(va);
        _instanceGeom->setTexCoordArray(0, ta);

        osg::DrawElementsUByte* de = new osg::DrawElementsUByte(GL_TRIANGLES);
        de->push_back(0); de->push_back(1); de->push_back(2);
        de->push_back(0); de->push_back(2); de->push_back(3);
        de->setNumInstances(0); _instanceGeom->addPrimitiveSet(de);

        // Apply instancing shader
        osg::StateSet* ss = _instanceGeom->getOrCreateStateSet();
        ss->setAttributeAndModes(_farDistanceProgram.get());
        ss->setDefine("FAR_DISTANCE");

        // Apply default parameter textures
        ss->setTextureAttributeAndModes(0, _posTexture.get());
        ss->setTextureAttributeAndModes(1, _dirTexture.get());
        ss->setTextureAttributeAndModes(2, _colorTexture.get());
        ss->setTextureAttributeAndModes(3, _iconTexture.get());
        ss->addUniform(new osg::Uniform("PosTexture", (int)0));
        ss->addUniform(new osg::Uniform("DirTexture", (int)1));
        ss->addUniform(new osg::Uniform("ColorTexture", (int)2));
        ss->addUniform(new osg::Uniform("IconTexture", (int)3));
        ss->addUniform(new osg::Uniform("InvResolution", 1.0f / (float)RES));
    }

    if (!_instanceBoard)
    {   // Create default instance billboard for displaying text boards
        osg::Vec3Array* va = new osg::Vec3Array(4);
        (*va)[3] = osg::Vec3(-0.5f, -0.5f, 0.0f);
        (*va)[2] = osg::Vec3(0.5f, -0.5f, 0.0f);
        (*va)[1] = osg::Vec3(0.5f, 0.5f, 0.0f);
        (*va)[0] = osg::Vec3(-0.5f, 0.5f, 0.0f);
        osg::Vec2Array* ta = new osg::Vec2Array(4);
        (*ta)[0] = osg::Vec2(0.0f, 0.0f); (*ta)[1] = osg::Vec2(1.0f, 0.0f);
        (*ta)[2] = osg::Vec2(1.0f, 1.0f); (*ta)[3] = osg::Vec2(0.0f, 1.0f);

        _instanceBoard = new osg::Geometry;
        _instanceBoard->setUseDisplayList(false);
        _instanceBoard->setUseVertexBufferObjects(true);
#if OSG_VERSION_GREATER_THAN(3, 2, 3)
        _instanceBoard->setCullingActive(false);
#endif
        _instanceBoard->setVertexArray(va);
        _instanceBoard->setTexCoordArray(0, ta);

        osg::DrawElementsUByte* de = new osg::DrawElementsUByte(GL_TRIANGLES);
        de->push_back(0); de->push_back(1); de->push_back(2);
        de->push_back(0); de->push_back(2); de->push_back(3);
        de->setNumInstances(0); _instanceBoard->addPrimitiveSet(de);

        // Apply instancing shader
        osg::StateSet* ss = _instanceBoard->getOrCreateStateSet();
        ss->setAttributeAndModes(_midDistanceProgram.get());
        ss->setDefine("MID_DISTANCE");

        // Apply default parameter textures
        ss->setTextureAttributeAndModes(0, _posTexture2.get());
        ss->setTextureAttributeAndModes(1, _dirTexture2.get());
        ss->setTextureAttributeAndModes(2, _colorTexture2.get());
        ss->setTextureAttributeAndModes(3, _textTexture.get());
        ss->setTextureAttributeAndModes(4, _bgIconTexture.get());
        ss->addUniform(new osg::Uniform("PosTexture", (int)0));
        ss->addUniform(new osg::Uniform("DirTexture", (int)1));
        ss->addUniform(new osg::Uniform("ColorTexture", (int)2));
        ss->addUniform(new osg::Uniform("TextTexture", (int)3));
        ss->addUniform(new osg::Uniform("BackgroundTexture", (int)4));
        ss->addUniform(new osg::Uniform("InvResolution", 1.0f / (float)RES));
        ss->addUniform(_midDistanceOffset.get());
        ss->addUniform(_midDistanceScale.get());
    }

    // Add to geode
    osg::Geode* geode1 = new osg::Geode;
    geode1->setName("SymbolIconGeode");
    geode1->addDrawable(_instanceGeom.get());
    geode1->setCullingActive(false);
    geode1->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    geode1->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

    osg::Geode* geode2 = new osg::Geode;
    geode2->setName("SymbolTextBoardGeode");
    geode2->addDrawable(_instanceBoard.get());
    geode2->setCullingActive(false);
    geode2->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    geode2->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    group->addChild(geode1); group->addChild(geode2);
}

void SymbolManager::update(osg::Group* group, unsigned int frameNo)
{
    osg::BoundingBox boundBox;
    osg::Matrix viewMatrix = _camera->getViewMatrix();
    osg::Matrix projMatrix = _camera->getProjectionMatrix();
    double inv1 = 1.0 / (_lodDistances[0] - _lodDistances[1]);
    double inv2 = 1.0 / (_lodDistances[1] - _lodDistances[2]);
    Symbol* nearestSym = NULL; double nearest = FLT_MAX;
    int numInstances = 0, numInstances2 = 0;

    osg::Polytope frustum;
    frustum.setToUnitFrustum(false, false);
    frustum.transformProvidingInverse(viewMatrix * projMatrix);

    // Use octree / RTree to query symbols and find visible ones
    // TODO

    // Traverse all symbols
    osg::Vec4f* posHandle = (osg::Vec4f*)_posTexture->getImage()->data();
    osg::Vec4f* posHandle2 = (osg::Vec4f*)_posTexture2->getImage()->data();
    osg::Vec4f* dirHandle = (osg::Vec4f*)_dirTexture->getImage()->data();
    osg::Vec4f* dirHandle2 = (osg::Vec4f*)_dirTexture2->getImage()->data();
    osg::Vec4f* colorHandle = (osg::Vec4f*)_colorTexture->getImage()->data();
    osg::Vec4f* colorHandle2 = (osg::Vec4f*)_colorTexture2->getImage()->data();
    float lodScale0 = _lodIconScaleFactor[0] - _lodIconScaleFactor[1];
    float lodScale1 = _lodIconScaleFactor[1] - _lodIconScaleFactor[2];

    std::vector<Symbol*> texts;
    std::map<double, std::vector<std::pair<Symbol*, osg::Vec4>>> symbolsInOrder;
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
            scale = sym->scale * (interpo * lodScale0 + _lodIconScaleFactor[1]);
        }
        else if (distance > _lodDistances[2])
        {
            sym->state = Symbol::MidDistance;
            interpo = (distance - _lodDistances[2]) * inv2;
            scale = sym->scale * (interpo * lodScale1 + _lodIconScaleFactor[2]);
        }
        else
        {
            // Load or re-use symbol model
            scale = sym->scale * _lodIconScaleFactor[2];
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
        if (sym->state == Symbol::Hidden || sym->state == Symbol::NearDistance) continue;
        symbolsInOrder[distance].push_back(
            std::pair<Symbol*, osg::Vec4>(sym, osg::Vec4(eyePos, (float)scale)));

        int y = itr->first / RES, x = itr->first % RES;
        if (y < 0 || y >(RES - 1)) { OSG_WARN << "[SymbolManager] Data overflow!" << std::endl; break; }
    }

    std::vector<std::array<float, 2>> kmeansPoints;
    std::vector<std::pair<Symbol*, osg::Vec4>> symbolsInOrder2;
    std::map<double, std::vector<std::pair<Symbol*, osg::Vec4>>>::iterator itr;
    for (itr = symbolsInOrder.begin(); itr != symbolsInOrder.end(); ++itr)
    {
        // Save ordered points to vector and prepare for clustering
        std::vector<std::pair<Symbol*, osg::Vec4>>& pairList = itr->second;
        for (size_t n = 0; n < pairList.size(); ++n)
        {
            std::pair<Symbol*, osg::Vec4>& pair = pairList[n];
            osg::Vec3 proj = osg::Vec3(pair.second[0], pair.second[1],
                                       pair.second[2]) * projMatrix;
            std::array<float, 2> vec; vec[0] = proj[0]; vec[1] = proj[1];
            pair.first->projAndScale = osg::Vec4(proj, pair.second[3]);
            kmeansPoints.push_back(vec); symbolsInOrder2.push_back(pair);
        }
    }

    if (!kmeansPoints.empty())
    {
#if false  // use kmean to cluster?
        size_t numK = kmeansPoints.size() / 4; if (numK == 0) numK = 1;
        auto result = dkm::kmeans_lloyd_parallel(kmeansPoints, numK);
        std::vector<std::array<float, 2>> centers = std::get<0>(result);
        std::vector<uint32_t> classIndices = std::get<1>(result);
        std::set<uint32_t> usedIndices;

        for (size_t n = 0; n < symbolsInOrder2.size(); ++n)
        {
            Symbol* sym = symbolsInOrder2[n].first;
            uint32_t classId = classIndices[n];
            osg::Vec2 offset(kmeansPoints[n][0] - centers[classId][0],
                             kmeansPoints[n][1] - centers[classId][1]);
            if (offset.length() < 0.01f)
            {
                //if (usedIndices.find(classId) == usedIndices.end())
                //    { usedIndices.insert(classId); }
                //else
                //    { continue; }
            }

            // Save to parameter textures
            const osg::Vec4 posAndScale = symbolsInOrder2[n].second;
            if (sym->state == Symbol::MidDistance)
            {
                *(posHandle2 + numInstances2) = posAndScale;
                *(dirHandle2 + numInstances2) = osg::Vec4(sym->tiling2, 1.0f);
                *(colorHandle2 + numInstances) = sym->color;
                texts.push_back(sym); numInstances2++;
                if (!_showIconsInMidDistance) continue;
            }

            *(posHandle + numInstances) = posAndScale;
            *(dirHandle + numInstances) = osg::Vec4(sym->tiling, sym->rotateAngle);
            *(colorHandle + numInstances) = sym->color;
            boundBox.expandBy(sym->position); numInstances++;  // FarDistance
        }
#else
        for (size_t n = 0; n < symbolsInOrder2.size(); ++n)
        {
            Symbol* sym = symbolsInOrder2[n].first;
            const osg::Vec4 posAndScale = symbolsInOrder2[n].second;

            // Save to parameter textures
            if (sym->state == Symbol::MidDistance)
            {
                *(posHandle2 + numInstances2) = posAndScale;
                *(dirHandle2 + numInstances2) = osg::Vec4(sym->tiling2, 1.0f);
                *(colorHandle2 + numInstances) = sym->color;
                texts.push_back(sym); numInstances2++;
                if (!_showIconsInMidDistance) continue;
            }

            *(posHandle + numInstances) = posAndScale;
            *(dirHandle + numInstances) = osg::Vec4(sym->tiling, sym->rotateAngle);
            *(colorHandle + numInstances) = sym->color;
            boundBox.expandBy(sym->position); numInstances++;  // FarDistance
        }
#endif
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
        _posTexture->getImage()->dirty(); _dirTexture->getImage()->dirty();
        _colorTexture->getImage()->dirty();
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
        _posTexture2->getImage()->dirty(); _dirTexture2->getImage()->dirty();
        _colorTexture2->getImage()->dirty();

        // Collect labels and recreate texture
        if (_drawGridCallback.valid())
            _textTexture->setImage(_drawGridCallback->create(_drawer.get(), texts));
        else
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
        group->addChild(mt.get()); sym->loadedModel = mt.get();

        osg::ref_ptr<osg::ProxyNode> proxy = new osg::ProxyNode;
        proxy->setFileName(0, sym->fileName); mt->addChild(proxy.get());
    }
    else
    {
        osg::MatrixTransform* mt = static_cast<osg::MatrixTransform*>(sym->loadedModel.get());
        osg::Vec3d dir = sym->position; dir.normalize();
        osg::Quat q; q.makeRotate(osg::Z_AXIS, osg::Vec3(dir));
        mt->setMatrix(osg::Matrix::rotate(osg::PI - sym->rotateAngle, osg::Z_AXIS) *
                      osg::Matrix::rotate(q) * osg::Matrix::translate(sym->position));
        sym->loadedModel->setNodeMask(0xffffffff);
    }
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


osg::Image* SymbolManager::createGrid(int w, int h, int grid, const std::vector<Symbol*>& texts)
{
    if (w != _drawer->s() || h != _drawer->t())
        _drawer->allocateImage(w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    _drawer->start(false); _drawer->clear();
    //_drawer->fillBackground(osg::Vec4(0.5f, 0.5f, 0.5f, 0.8f));

    float textSize = 30.0f;
    int stepW = w / grid, stepH = h / grid;
    size_t numText = texts.size();
    for (size_t j = 0; j < numText; ++j)
    {
        int tx = j % grid, ty = j / grid;
        std::vector<std::string> lines;
        osgDB::split(texts[j]->name, lines, '\n');

        float x = stepW * tx + 30.0f, y = stepH * ty + (stepH + textSize) * 0.5f;
        for (size_t i = 0; i < lines.size(); ++i)
        {
            std::wstring t = osgDB::convertUTF8toUTF16(lines[i]);
            _drawer->drawText(osg::Vec2(x, y + i * stepH), textSize, t, "",
                              Drawer2D::StyleData(texts[j]->textColor, true));
        }
    }
    _drawer->finish();
    return (osg::Image*)_drawer->clone(osg::CopyOp::DEEP_COPY_ALL);
}
