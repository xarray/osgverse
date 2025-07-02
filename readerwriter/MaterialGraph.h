#ifndef MANA_READERWRITER_MATERIALGRAPH_HPP
#define MANA_READERWRITER_MATERIALGRAPH_HPP

#include <osg/Transform>
#include <osg/Geometry>
#include <osgDB/ReaderWriter>
#include <sstream>
#include <iostream>
#include "Export.h"

namespace osgVerse
{
    class OSGVERSE_RW_EXPORT MaterialGraph : public osg::Referenced
    {
    public:
        static MaterialGraph* instance();
        bool readFromBlender(const std::string& data, osg::StateSet& ss);

        struct MaterialPin : public osg::Referenced
        {
            unsigned int id;
            std::string name, type;
            std::vector<double> values;
        };

        struct MaterialNode : public osg::Referenced
        {
            unsigned int id;
            std::string name, type, imageName, imagePath;
            osg::ref_ptr<osg::Texture> texture;
            std::map<std::string, std::string> attributes;
            std::map<std::string, std::map<int, osg::ref_ptr<MaterialPin>>> inputs, outputs;

            std::map<int, osg::ref_ptr<MaterialPin>>::iterator findPin(
                    bool out, const std::string& name, const std::string& type = "")
            {
                std::map<int, osg::ref_ptr<MaterialPin>>& pinIndices = out ? outputs[name] : inputs[name];
                if (type.empty()) return pinIndices.empty() ? pinIndices.end() : pinIndices.begin();

                for (std::map<int, osg::ref_ptr<MaterialPin>>::iterator it = pinIndices.begin();
                     it != pinIndices.end(); ++it) { if (it->second->type == type) return it; }
                return pinIndices.end();
            }
        };

        struct MaterialLink : public osg::Referenced
        {
            int idFrom, idTo;
            osg::ref_ptr<MaterialNode> nodeFrom, nodeTo;
            osg::observer_ptr<MaterialPin> pinFrom, pinTo;
        };

        typedef std::map<std::string, osg::ref_ptr<MaterialNode>> MaterialNodeMap;
        typedef std::map<int, osg::ref_ptr<MaterialPin>> MaterialPinIndices;
        typedef std::map<std::string, MaterialPinIndices> MaterialPinMap;
        typedef std::vector<osg::ref_ptr<MaterialLink>> MaterialLinkList;

    protected:
        MaterialGraph() {}
        MaterialLink* findLink(MaterialLinkList& links, MaterialNode* node, MaterialPin* pin, int id, bool findFrom);

        struct BlenderComposition
        {
            inline std::string variable(MaterialNode* n, MaterialPin* p)
            { std::stringstream var; var << "var_" << n->id << "_" << p->id; return var.str(); }
            inline std::string textureVariable(MaterialNode* n)
            { std::stringstream var; var << "var_tex_" << n->id; return var.str(); }
            inline std::string sampler(MaterialNode* n)
            { std::stringstream var; var << "tex_" << n->id; return var.str(); }

            inline void prependCode(const std::string& v1) { glslCode = v1 + glslCode; }
            inline void prependVariables(const std::string& v1) { glslVars = v1 + glslVars; }
            inline void prependGlobal(const std::string& v1) { glslGlobal = v1 + glslGlobal; }

            std::string glslCode, glslVars, glslFuncs, glslGlobal, bsdfChannelName;
            osg::ref_ptr<osg::StateSet> stateset; MaterialLinkList links;
        };
        void processBlenderLinks(MaterialNodeMap& nodes, MaterialLinkList& links, osg::StateSet& ss);
        void processBlenderLink(BlenderComposition& comp, const osg::StateSet& ss,
                                MaterialNode* lastNode, MaterialPin* lastOutPin, int lastOutID);
        void findAndProcessBlenderLink(BlenderComposition& comp, const osg::StateSet& ss,
                                       MaterialNode* node, MaterialPin* pin, int id, bool findFrom);
        void applyBlenderTexture(BlenderComposition& comp, const osg::StateSet& ss, const std::string& samplerName);
        unsigned int getBlenderTextureUnit(BlenderComposition& comp);
    };
}

#endif
