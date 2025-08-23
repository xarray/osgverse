#ifndef MANA_PP_SHADERLIBRARY_HPP
#define MANA_PP_SHADERLIBRARY_HPP

#include <osg/Version>
#include <osg/Shader>
#include <osg/Program>
#include <osgDB/Registry>
#include <vector>
#include <map>

namespace osgVerse
{
    class Pipeline;

    class ShaderLibrary : public osg::Referenced
    {
    public:
        static ShaderLibrary* instance();
        static int guessShaderVersion(int& glContext);

        enum PreDefinedModule
        {
            UTILITY_SHADERS = 0x1,
            LIGHTING_SHADERS = 0x2,
            COMMON_SHADERS = UTILITY_SHADERS | LIGHTING_SHADERS,
            ALL_SHADERS = 0xFF
        };
        void refreshModules(const std::string& baseDir);

        /** Add pre-defined shader modules to given program.
            Function implementations will be added as independent shader objects first,
            And declarations will be added to each existing shaders of the program.
            Parameter moduleFlags is the commbination of PreDefinedModules
        */
        void updateProgram(osg::Program& program, Pipeline* pipeline = NULL,
                           int moduleFlags = COMMON_SHADERS, bool needDefinitions = true);
        void updateProgram(osg::StateSet& ss, Pipeline* pipeline = NULL,
                           int moduleFlags = COMMON_SHADERS, bool needDefinitions = true)
        {
            osg::Program* prog = static_cast<osg::Program*>(ss.getAttribute(osg::StateAttribute::PROGRAM));
            if (prog) updateProgram(*prog, pipeline, moduleFlags, needDefinitions);
        }

        /** Add necessaray definitions for GLSL shaders
            Special macros:
            - osg_Vertex/osg_Color/osg_MultiTexCoord0/osg_MultiTexCoord1/osg_Normal: attributes in vertex shader
            - VERSE_GLES2: for GLES2 shaders, set to 1
            - VERSE_GLES3: for GLES3 and GLCore shaders, set to 1
            - VERSE_MATRIX_MVP: model-view-projection matrix in vertex shader
            - VERSE_MATRIX_MV: model-view matrix in vertex shader
            - VERSE_MATRIX_P: projection matrix in vertex shader
            - VERSE_MATRIX_N: normal matrix in vertex shader
            - VERSE_VS_IN/VERSE_VS_OUT: in/out variables in vertex shader
            - VERSE_FS_IN/VERSE_FS_OUT: in/out variables in fragment shader
            - VERSE_FS_FINAL: final frag-color for GLCompatible / GLES2 shaders in fragment shader
            - VERSE_MAX_SHADOWS: maximum number of shadows in fragment shader
            - VERSE_TEX1D/VERSE_TEX2D/VERSE_TEX3D/VERSE_TEXCUBE: texture sampling function
            - VERSE_SCRIPT_DEF(): definitions of all script segments, used by ScriptableProgram
            - VERSE_SCRIPT_FUNC(num): input position <num> of script segments, used by ScriptableProgram
        */
        void createShaderDefinitions(osg::Shader& s, int glVer, int glslVer,
                                     const std::vector<std::string>& userDefs = std::vector<std::string>(),
                                     const osgDB::ReaderWriter::Options* options = NULL);

    protected:
        ShaderLibrary();
        virtual ~ShaderLibrary();
        void processIncludes(osg::Shader& shader, const osgDB::ReaderWriter::Options* options) const;
        void updateModuleData(PreDefinedModule m, osg::Shader::Type type,
                              const std::string& baseDir, const std::string& name);

        std::map<PreDefinedModule, std::string> _moduleHeaders;
        std::map<PreDefinedModule, osg::ref_ptr<osg::Shader>> _moduleShaders;
    };

    /** Scriptable program that can insert script segments to internal shaders
        - VERSE_SCRIPT_DEF(): definitions of all script segments
        - VERSE_SCRIPT_FUNC(num): input position <num> of script segments
    */
    class ScriptableProgram : public osg::Program
    {
    public:
        ScriptableProgram();
        ScriptableProgram(const ScriptableProgram& rhs, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);
        META_Object(osgVerse, ScriptableProgram)

        void addDefinitions(osg::Shader::Type t, const std::string& code);
        void addSegment(osg::Shader::Type t, int pos, const std::string& code);
        void dirty() { _dirty = true; }

        typedef std::vector<std::string> CodeSegmentList;
        std::map<int, CodeSegmentList>& getSegments(osg::Shader::Type t) { return _segments[t]; }
        void getSegments(osg::Shader::Type t, std::map<int, CodeSegmentList>& segments) const;

        CodeSegmentList& getDefinitions(osg::Shader::Type t) { return _definitions[t]; }
        void getDefinitions(osg::Shader::Type t, CodeSegmentList& defs) const;

    protected:
        virtual void compileGLObjects(osg::State& state) const;

        std::map<osg::Shader::Type, std::map<int, CodeSegmentList>> _segments;
        std::map<osg::Shader::Type, CodeSegmentList> _definitions;
        mutable std::map<osg::Shader::Type, std::string> _originalShaderCodes;
        mutable bool _dirty;
    };
}

#endif
