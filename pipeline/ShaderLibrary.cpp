#include <osgDB/ReadFile>
#include "Pipeline.h"
#include "ShadowModule.h"
#include "ShaderLibrary.h"
#include <iostream>
#include <sstream>
using namespace osgVerse;

static std::string trimString(const std::string& str)
{
    if (!str.size()) return str;
    std::string::size_type first = str.find_first_not_of(" \t");
    std::string::size_type last = str.find_last_not_of("  \t\r\n");
    if ((first == str.npos) || (last == str.npos)) return std::string("");
    return str.substr(first, last - first + 1);
}

ShaderLibrary* ShaderLibrary::instance()
{
    static osg::ref_ptr<ShaderLibrary> s_instance = new ShaderLibrary;
    return s_instance.get();
}

ShaderLibrary::ShaderLibrary()
{}

ShaderLibrary::~ShaderLibrary()
{}

int ShaderLibrary::guessShaderVersion(int& glContext)
{
#if defined(OSG_GL3_AVAILABLE)
    glContext = 300; return 330;
#elif defined(OSG_GLES2_AVAILABLE)
    glContext = 100; return 200;
#elif defined(OSG_GLES3_AVAILABLE)
    glContext = 100; return 300;
#else
    glContext = 100; return 120;
#endif
}

void ShaderLibrary::updateModuleData(PreDefinedModule m, osg::Shader::Type type,
                                     const std::string& baseDir, const std::string& name)
{
    std::ifstream fin(baseDir + name + ".module.h");
    osg::ref_ptr<osg::Shader> shader = osgDB::readRefShaderFile(baseDir + name + ".module.glsl");
    std::string moduleMarker = "//! osgVerse module: " + name + "\n";

    if (fin && shader.valid())
    {
        std::istreambuf_iterator<char> eos;
        _moduleHeaders[m] = std::string(std::istreambuf_iterator<char>(fin), eos);
        _moduleShaders[m] = shader;
        shader->setName(name + ".module"); shader->setType(type);
        shader->setShaderSource(moduleMarker + shader->getShaderSource());
    }
    else
        OSG_WARN << "[ShaderLibrary] Shader module " << name << " not found. "
                 << "Your base directory <" << baseDir << "> may be broken." << std::endl;
}

void ShaderLibrary::refreshModules(const std::string& baseDir)
{
    updateModuleData(UTILITY_SHADERS, osg::Shader::VERTEX, baseDir, "common_vert");
    updateModuleData(UTILITY_SHADERS, osg::Shader::FRAGMENT, baseDir, "common_frag");
    updateModuleData(LIGHTING_SHADERS, osg::Shader::FRAGMENT, baseDir, "lighting");
}

void ShaderLibrary::updateProgram(osg::Program& program, Pipeline* pipeline,
                                  int moduleFlags, bool needDefinitions)
{
    int glVer = 0; int glslVer = guessShaderVersion(glVer);
    if (needDefinitions)
    {
        for (size_t i = 0; i < program.getNumShaders(); ++i)
        {
            if (!pipeline) createShaderDefinitions(*program.getShader(i), glVer, glslVer);
            else pipeline->createShaderDefinitionsFromPipeline(program.getShader(i));
        }
    }

    std::vector<osg::ref_ptr<osg::Shader>> shadersToAdd;
    for (std::map<PreDefinedModule, osg::ref_ptr<osg::Shader>>::iterator
         itr = _moduleShaders.begin(); itr != _moduleShaders.end(); ++itr)
    {
        bool alreadyAdded = false;
        osg::Shader::Type type = itr->second->getType();
        std::vector<osg::Shader*> shadersToHaveHeader;
        if ((moduleFlags & itr->first) == 0) continue;

        for (size_t i = 0; i < program.getNumShaders(); ++i)
        {   // check if existing shaders can have module declarations
            osg::Shader* s = program.getShader(i);
            if (s == itr->second) { alreadyAdded = true; break; }
            else if (s->getShaderSource().empty()) continue;
            else if (s->getType() == type) shadersToHaveHeader.push_back(s);
        }

        if (!alreadyAdded)
        {   // add declarations and ready to add modules
#if defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
            std::string defs = itr->second->getShaderSource() + "\n";
#else
            std::string defs = _moduleHeaders[itr->first] + "\n";
#endif

            for (size_t i = 0; i < shadersToHaveHeader.size(); ++i)
            {
                std::string code = shadersToHaveHeader[i]->getShaderSource();
                std::string line; std::stringstream ss; ss << code;
                while (std::getline(ss, line))
                {
                    line = trimString(line); if (line.empty()) continue;
                    if (line.find("precision") != std::string::npos) continue;
                    if (line[0] == '#' || line[0] == '/') continue; else break;
                }

                size_t posToInsert = line.empty() ? std::string::npos : code.find(line);
                if (posToInsert != std::string::npos)
                {
                    shadersToHaveHeader[i]->setShaderSource(
                        code.substr(0, posToInsert) + defs + code.substr(posToInsert));
                }
                else
                    shadersToHaveHeader[i]->setShaderSource(defs + code);
            }
#if !defined(OSG_GLES2_AVAILABLE) && !defined(OSG_GLES3_AVAILABLE)
            shadersToAdd.push_back(itr->second);
#endif
        }
    }

    if (!shadersToAdd.empty())
    {
        for (size_t i = 0; i < shadersToAdd.size(); ++i)
            program.addShader(shadersToAdd[i].get());
        program.dirtyProgram();
    }
}

void ShaderLibrary::createShaderDefinitions(osg::Shader& shader, int glVer, int glslVer,
                                            const std::vector<std::string>& userDefs,
                                            const osgDB::ReaderWriter::Options* options)
{
    std::vector<std::string> extraDefs; processIncludes(shader, options);
    std::string source = shader.getShaderSource();
    if (source.find("//! osgVerse") != std::string::npos) return;

    std::string m_mvp = "gl_ModelViewProjectionMatrix", m_mv = "gl_ModelViewMatrix";
    std::string m_p = "gl_ProjectionMatrix", m_n = "gl_NormalMatrix";
    std::string tex1d = "texture", tex2d = "texture", tex3d = "texture", texCube = "texture";
    std::string vin = "in", vout = "out", fin = "in", fout = "out", finalColor = "//";
#if !defined(OSG_GLES1_AVAILABLE) && !defined(OSG_GLES2_AVAILABLE)
    if (glslVer <= 120)
#endif
    {
        tex1d = "texture1D"; tex2d = "texture2D"; tex3d = "texture3D"; texCube = "textureCube";
        vin = "attribute"; vout = "varying"; fin = "varying"; fout = "";
        finalColor = "gl_FragColor = ";

        extraDefs.push_back("float round(float v) { return v<0.0 ? ceil(v-0.5) : floor(v+0.5); }");
        extraDefs.push_back("vec2 round(vec2 v) { return vec2(round(v.x), round(v.y)); }");
        extraDefs.push_back("vec3 round(vec3 v) { return vec3(round(v.x), round(v.y), round(v.z)); }");
        extraDefs.push_back("vec4 textureLod(sampler2D t, vec2 uv, float l) { return texture2D(t, uv); }");
    }

    if (shader.getType() == osg::Shader::VERTEX)
    {
#if !defined(OSG_GLES1_AVAILABLE) && !defined(OSG_GLES2_AVAILABLE)
        if (glVer >= 300 || glslVer >= 140)
        {
            m_mvp = "osg_ModelViewProjectionMatrix"; m_mv = "osg_ModelViewMatrix";
            m_p = "osg_ProjectionMatrix"; m_n = "osg_NormalMatrix";
            extraDefs.push_back("uniform mat4 osg_ModelViewProjectionMatrix, "
                "osg_ModelViewMatrix, osg_ProjectionMatrix;");
            extraDefs.push_back("uniform mat3 osg_NormalMatrix;");
            extraDefs.push_back("VERSE_VS_IN vec4 osg_Vertex, osg_Color, "
                "osg_MultiTexCoord0, osg_MultiTexCoord1;");
            extraDefs.push_back("VERSE_VS_IN vec3 osg_Normal;");
        }
        else
#endif
        {
            extraDefs.push_back("#define osg_Vertex gl_Vertex");
            extraDefs.push_back("#define osg_Color gl_Color");
            extraDefs.push_back("#define osg_MultiTexCoord0 gl_MultiTexCoord0");
            extraDefs.push_back("#define osg_MultiTexCoord1 gl_MultiTexCoord1");
            extraDefs.push_back("#define osg_Normal gl_Normal");
        }
    }

    std::stringstream ss;
#if defined(OSG_GLES1_AVAILABLE) || defined(OSG_GLES2_AVAILABLE)
    ss << "#extension GL_EXT_draw_buffers: enable" << std::endl;
    ss << "#extension GL_OES_standard_derivatives: enable" << std::endl;
    ss << "#define VERSE_GLES2 1" << std::endl;
#elif defined(OSG_GLES3_AVAILABLE)
    ss << "#version " << osg::maximum(glslVer, 300) << " es" << std::endl;
    ss << "#define VERSE_GLES3 1" << std::endl;
#elif defined(OSG_GL3_AVAILABLE)
    ss << "#version " << osg::maximum(glslVer, 330) << " core" << std::endl;
    ss << "#define VERSE_GLES3 1" << std::endl;
#else
    if (glslVer > 0)
    {
        if (glslVer < 300) ss << "#version " << glslVer << std::endl;
        else ss << "#version " << glslVer << " compatibility" << std::endl;
    }
#endif

#if defined(VERSE_WEBGL1)
    ss << "#define VERSE_WEBGL1 1" << std::endl;
#elif defined(VERSE_WEBGL2)
    ss << "#define VERSE_WEBGL2 1" << std::endl;
#endif
    ss << "//! osgVerse generated shader: " << glslVer << std::endl;

#if defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE)
    ss << "precision highp float;" << std::endl;
#endif
    if (shader.getType() == osg::Shader::VERTEX)
    {
        ss << "#define VERSE_MATRIX_MVP " << m_mvp << std::endl;
        ss << "#define VERSE_MATRIX_MV " << m_mv << std::endl;
        ss << "#define VERSE_MATRIX_P " << m_p << std::endl;
        ss << "#define VERSE_MATRIX_N " << m_n << std::endl;
        ss << "#define VERSE_VS_IN " << vin << std::endl;
        ss << "#define VERSE_VS_OUT " << vout << std::endl;
    }
    else if (shader.getType() == osg::Shader::FRAGMENT)
    {
        ss << "#define VERSE_FS_IN " << fin << std::endl;
        ss << "#define VERSE_FS_OUT " << fout << std::endl;
        ss << "#define VERSE_FS_FINAL " << finalColor << std::endl;
        ss << "#define VERSE_MAX_SHADOWS " << MAX_SHADOWS << std::endl;
    }
    ss << "#define VERSE_TEX1D " << tex1d << std::endl;
    ss << "#define VERSE_TEX2D " << tex2d << std::endl;
    ss << "#define VERSE_TEX3D " << tex3d << std::endl;
    ss << "#define VERSE_TEXCUBE " << texCube << std::endl;

    for (size_t i = 0; i < extraDefs.size(); ++i) ss << extraDefs[i] << std::endl;
    for (size_t i = 0; i < userDefs.size(); ++i) ss << userDefs[i] << std::endl;
    if (source.find("#include") != std::string::npos)
    {
        OSG_WARN << "[Pipeline] Found not working '#include' flags: "
                 << shader.getName() << std::endl;
    }
    shader.setShaderSource(ss.str() + source);
}

void ShaderLibrary::processIncludes(osg::Shader& shader, const osgDB::ReaderWriter::Options* options) const
{
    std::string code = shader.getShaderSource();
    std::string startOfIncludeMarker("// BEGIN: ");
    std::string endOfIncludeMarker("// END: ");
    std::string failedLoadMarker("// FAILED: ");

#if defined(__APPLE__)
    std::string endOfLine("\r");
#elif defined(_WIN32)
    std::string endOfLine("\r\n");
#else
    std::string endOfLine("\n");
#endif

    std::string::size_type pos = 0, pragma_pos = 0, include_pos = 0;
    while ((pos != std::string::npos) && (((pragma_pos = code.find("#pragma", pos)) != std::string::npos) ||
                                           (include_pos = code.find("#include", pos)) != std::string::npos))
    {
        pos = (pragma_pos != std::string::npos) ? pragma_pos : include_pos;
        std::string::size_type start_of_pragma_line = pos;
        std::string::size_type end_of_line = code.find_first_of("\n\r", pos);
        if (pragma_pos != std::string::npos)
        {   // we have #pragma usage so skip to the start of the first non white space
            pos = code.find_first_not_of(" \t", pos + 7);
            if (pos == std::string::npos) break;

            // check for include part of #pragma imclude usage
            if (code.compare(pos, 7, "include") != 0) { pos = end_of_line; continue; }

            // found include entry so skip to next non white space
            pos = code.find_first_not_of(" \t", pos + 7);
            if (pos == std::string::npos) break;
        }
        else
        {   // we have #include usage so skip to next non white space
            pos = code.find_first_not_of(" \t", pos + 8);
            if (pos == std::string::npos) break;
        }

        std::string::size_type num_characters = (end_of_line == std::string::npos)
                                              ? code.size() - pos : end_of_line - pos;
        if (num_characters == 0) continue;

        // prune trailing white space
        while (num_characters > 0 && (code[pos + num_characters - 1] == ' ' ||
               code[pos + num_characters - 1] == '\t')) --num_characters;
        if (code[pos] == '\"')
        {
            if (code[pos + num_characters - 1] != '\"') num_characters -= 1;
            else num_characters -= 2; ++pos;
        }

        std::string filename(code, pos, num_characters);
        code.erase(start_of_pragma_line, (end_of_line == std::string::npos) ?
                   (code.size() - start_of_pragma_line) : (end_of_line - start_of_pragma_line));
        pos = start_of_pragma_line;

        osg::ref_ptr<osg::Shader> innerShader = osgDB::readRefShaderFile(filename, options);
        if (innerShader.valid())
        {
            if (!startOfIncludeMarker.empty())
            {
                code.insert(pos, startOfIncludeMarker); pos += startOfIncludeMarker.size();
                code.insert(pos, filename); pos += filename.size();
                code.insert(pos, endOfLine); pos += endOfLine.size();
            }

            code.insert(pos, innerShader->getShaderSource());
            pos += innerShader->getShaderSource().size();
            if (!endOfIncludeMarker.empty())
            {
                code.insert(pos, endOfIncludeMarker); pos += endOfIncludeMarker.size();
                code.insert(pos, filename); pos += filename.size();
                code.insert(pos, endOfLine); pos += endOfLine.size();
            }
        }
        else
        {
            if (!failedLoadMarker.empty())
            {
                code.insert(pos, failedLoadMarker); pos += failedLoadMarker.size();
                code.insert(pos, filename); pos += filename.size();
                code.insert(pos, endOfLine); pos += endOfLine.size();
            }
        }
    }
    shader.setShaderSource(code);
}
