#ifndef MANA_READERWRITER_SAFEGLTFINPUT_H
#define MANA_READERWRITER_SAFEGLTFINPUT_H

#include <cstddef>
#include <string>
#include <vector>

namespace osgVerse
{
    static const size_t MAX_REMOTE_GLTF_RESOURCE_BYTES = 64u * 1024u * 1024u;

    struct ThreeDTileHeader
    {
        enum Kind { NONE, B3DM, I3DM };
        Kind kind;
        unsigned int version, gltfFormat;
        size_t byteLength;
        size_t featureJsonOffset, featureJsonLength;
        size_t featureBinaryOffset, featureBinaryLength;
        size_t batchJsonOffset, batchJsonLength;
        size_t batchBinaryOffset, batchBinaryLength;
        size_t payloadOffset, payloadSize;

        ThreeDTileHeader();
    };

    bool parseThreeDTileHeader(const std::vector<char>& data, ThreeDTileHeader& header,
                               std::string* error = NULL);
    bool appendRemoteChunk(std::vector<unsigned char>& output, const char* data, size_t size,
                           size_t maxBytes, std::string* error = NULL);
    bool parseContentLength(const std::string& value, size_t& result);
}

#endif
