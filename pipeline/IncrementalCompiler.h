#ifndef MANA_PP_INCREMENTALCOMPILER_HPP
#define MANA_PP_INCREMENTALCOMPILER_HPP

#include <osg/Version>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osgUtil/IncrementalCompileOperation>

namespace osgVerse
{

    class IncrementalCompileCallback : public osg::Referenced
    {
    public:
        typedef osgUtil::IncrementalCompileOperation ICO;
        virtual bool compile(ICO::CompileDrawableOp*, ICO::CompileInfo&);
        virtual bool compile(ICO::CompileTextureOp*, ICO::CompileInfo&);
        virtual bool compile(ICO::CompileProgramOp*, ICO::CompileInfo&);
    };

    class IncrementalCompiler : public osgUtil::IncrementalCompileOperation
    {
    public:
        IncrementalCompiler();
        virtual void operator()(osg::GraphicsContext* context);

        void setCompileCallback(IncrementalCompileCallback* cb) { _callback = cb; }
        IncrementalCompileCallback* getCompileCallback() { return _callback.get(); }

    protected:
        virtual ~IncrementalCompiler();
        void compileGLDataSets(CompileSets& toCompile, CompileInfo& compileInfo);
        virtual bool compileGLDataSubList(CompileList& toCompile, CompileInfo& compileInfo);
        osg::ref_ptr<IncrementalCompileCallback> _callback;
    };

}

#endif
