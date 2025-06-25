#include "IncrementalCompiler.h"
#include <iostream>
#include <algorithm>
#include <iterator>
using namespace osgVerse;

bool IncrementalCompileCallback::compile(ICO::CompileDrawableOp* op, ICO::CompileInfo& compileInfo)
{ op->_drawable->compileGLObjects(compileInfo); return true; }

bool IncrementalCompileCallback::compile(ICO::CompileTextureOp* op, ICO::CompileInfo& compileInfo)
{
    osg::Geometry* forceGeometry = compileInfo.incrementalCompileOperation->getForceTextureDownloadGeometry();
    if (forceGeometry)
    {
        if (forceGeometry->getStateSet()) compileInfo.getState()->apply(forceGeometry->getStateSet());
        compileInfo.getState()->applyTextureMode(0, op->_texture->getTextureTarget(), true);
        compileInfo.getState()->applyTextureAttribute(0, op->_texture.get());
        forceGeometry->draw(compileInfo);
    }
    else
        op->_texture->apply(*compileInfo.getState());
    return true;
}

bool IncrementalCompileCallback::compile(ICO::CompileProgramOp* op, ICO::CompileInfo& compileInfo)
{ op->_program->compileGLObjects(*compileInfo.getState()); return true; }

IncrementalCompiler::IncrementalCompiler()
    : osgUtil::IncrementalCompileOperation()
{}

IncrementalCompiler::~IncrementalCompiler()
{}

void IncrementalCompiler::operator()(osg::GraphicsContext* context)
{
    const osg::FrameStamp* fs = context->getState()->getFrameStamp();
    double currentElapsedFrameTime = context->getTimeSinceLastClear();
    double currentTime = fs ? fs->getReferenceTime() : 0.0;
    double minimumTimeAvailableForGLCompileAndDeletePerFrame = _minimumTimeAvailableForGLCompileAndDeletePerFrame;
    double targetFrameTime = 1.0 / _targetFrameRate;
    double _flushTimeRatio(0.5), _conservativeTimeRatio(0.5);

    double availableTime = std::max((targetFrameTime - currentElapsedFrameTime) * _conservativeTimeRatio,
                                    minimumTimeAvailableForGLCompileAndDeletePerFrame);
    double flushTime = availableTime * _flushTimeRatio;
    double compileTime = availableTime - flushTime;

    CompileInfo compileInfo(context, this);
    compileInfo.allocatedTime = compileTime;
    compileInfo.maxNumObjectsToCompile = _maximumNumOfObjectsToCompilePerFrame;
    compileInfo.compileAll = (_compileAllTillFrameNumber > _currentFrameNumber);

    CompileSets toCompileCopy;
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex>  toCompile_lock(_toCompileMutex);
        std::copy(_toCompile.begin(), _toCompile.end(), std::back_inserter<CompileSets>(toCompileCopy));
    }

    if (!toCompileCopy.empty()) compileGLDataSets(toCompileCopy, compileInfo);
    osg::flushDeletedGLObjects(context->getState()->getContextID(), currentTime, flushTime);
    if (!toCompileCopy.empty() && compileInfo.maxNumObjectsToCompile > 0)
    {
        // if any time left over from flush add on this remaining time to a second pass of compiling.
        compileInfo.allocatedTime += flushTime;
        if (compileInfo.okToCompile()) compileGLDataSets(toCompileCopy, compileInfo);
    }
}

void IncrementalCompiler::compileGLDataSets(CompileSets& toCompile, CompileInfo& compileInfo)
{
    osg::GraphicsContext* gc = compileInfo.getState()->getGraphicsContext();
    for (CompileSets::iterator itr = toCompile.begin();
         itr != toCompile.end() && compileInfo.okToCompile();)
    {
        CompileSet* cs = itr->get(); bool compiled = false;
        CompileList& compileList = cs->_compileMap[gc];
        if (!compileList.empty())
        {
            if (compileGLDataSubList(compileList, compileInfo))
            {
                --cs->_numberCompileListsToCompile;
                compiled = cs->_numberCompileListsToCompile == 0;
            }
        }
        else
            compiled = cs->_numberCompileListsToCompile == 0;

        if (compiled)
        {
            {
                // remove from the _toCompile list, note cs won't be deleted here as the tempoary
                // toCompile_Copy list will retain a reference.
                OpenThreads::ScopedLock<OpenThreads::Mutex>  toCompile_lock(_toCompileMutex);
                CompileSets::iterator cs_itr = std::find(_toCompile.begin(), _toCompile.end(), *itr);
                if (cs_itr != _toCompile.end()) _toCompile.erase(cs_itr);
            }

            if (cs->_compileCompletedCallback.valid() && cs->_compileCompletedCallback->compileCompleted(cs))
            {
                // callback will handle merging of subgraph so no need to place CompileSet in merge.
            }
            else
            {
                OpenThreads::ScopedLock<OpenThreads::Mutex>  compilded_lock(_compiledMutex);
                _compiled.push_back(cs);
            }
            itr = toCompile.erase(itr);
        }
        else ++itr;
    }
}

bool IncrementalCompiler::compileGLDataSubList(CompileList& toCompile, CompileInfo& compileInfo)
{
    for (CompileList::CompileOps::iterator itr = toCompile._compileOps.begin();
         itr != toCompile._compileOps.end() && compileInfo.okToCompile();)
    {
        CompileList::CompileOps::iterator saved_itr(itr); ++itr;
        bool compiled = false; compileInfo.maxNumObjectsToCompile--;

        IncrementalCompiler::CompileOp* op = (*saved_itr).get();
        if (_callback.valid())
        {
            IncrementalCompiler::CompileDrawableOp* d = dynamic_cast<IncrementalCompiler::CompileDrawableOp*>(op);
            if (d) compiled = _callback->compile(d, compileInfo);
            else
            {
                IncrementalCompiler::CompileTextureOp* t = dynamic_cast<IncrementalCompiler::CompileTextureOp*>(op);
                if (t) compiled = _callback->compile(t, compileInfo);
                else
                {
                    IncrementalCompiler::CompileProgramOp* p = dynamic_cast<IncrementalCompiler::CompileProgramOp*>(op);
                    if (p) compiled = _callback->compile(p, compileInfo);
                }
            }
        }
        if (!compiled) compiled = op->compile(compileInfo);
        if (compiled) toCompile._compileOps.erase(saved_itr);
    }
    return toCompile.empty();
}
