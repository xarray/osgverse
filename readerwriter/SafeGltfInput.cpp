#include "SafeGltfInput.h"

#include <cctype>
#include <limits>

namespace
{
    static unsigned int readU32(const std::vector<char>& data, size_t offset)
    {
        return static_cast<unsigned int>(static_cast<unsigned char>(data[offset])) |
               (static_cast<unsigned int>(static_cast<unsigned char>(data[offset + 1])) << 8) |
               (static_cast<unsigned int>(static_cast<unsigned char>(data[offset + 2])) << 16) |
               (static_cast<unsigned int>(static_cast<unsigned char>(data[offset + 3])) << 24);
    }

    static bool checkedAdd(size_t a, size_t b, size_t& result)
    {
        if (b > std::numeric_limits<size_t>::max() - a) return false;
        result = a + b; return true;
    }

    static bool fail(std::string* error, const char* message)
    {
        if (error) *error = message;
        return false;
    }
}

namespace osgVerse
{
    ThreeDTileHeader::ThreeDTileHeader()
        : kind(NONE), version(0), gltfFormat(1), byteLength(0),
          featureJsonOffset(0), featureJsonLength(0),
          featureBinaryOffset(0), featureBinaryLength(0),
          batchJsonOffset(0), batchJsonLength(0),
          batchBinaryOffset(0), batchBinaryLength(0),
          payloadOffset(0), payloadSize(0)
    {}

    bool parseThreeDTileHeader(const std::vector<char>& data, ThreeDTileHeader& header,
                               std::string* error)
    {
        header = ThreeDTileHeader();
        if (error) error->clear();
        if (data.size() < 12) return fail(error, "3D Tiles header is truncated");
        if (data[0] != 'b' && data[0] != 'i') return fail(error, "unsupported 3D Tiles magic");
        const bool isB3dm = data[0] == 'b' && data[1] == '3' && data[2] == 'd' && data[3] == 'm';
        const bool isI3dm = data[0] == 'i' && data[1] == '3' && data[2] == 'd' && data[3] == 'm';
        if (!isB3dm && !isI3dm) return fail(error, "unsupported 3D Tiles magic");
        header.kind = isB3dm ? ThreeDTileHeader::B3DM : ThreeDTileHeader::I3DM;
        header.version = readU32(data, 4);
        if (header.version != 1) return fail(error, "unsupported 3D Tiles version");
        const size_t totalSize = static_cast<size_t>(readU32(data, 8));
        header.byteLength = totalSize;
        if (totalSize > data.size()) return fail(error, "declared 3D Tiles length exceeds input");

        const size_t headerSize = isB3dm ? 28u : 32u;
        if (totalSize < headerSize || data.size() < headerSize)
            return fail(error, "3D Tiles header is truncated");

        header.featureJsonLength = static_cast<size_t>(readU32(data, 12));
        header.featureBinaryLength = static_cast<size_t>(readU32(data, 16));
        header.batchJsonLength = static_cast<size_t>(readU32(data, 20));
        header.batchBinaryLength = static_cast<size_t>(readU32(data, 24));

        size_t cursor = headerSize;
        header.featureJsonOffset = cursor;
        if (!checkedAdd(cursor, header.featureJsonLength, cursor))
            return fail(error, "3D Tiles section length overflows");
        header.featureBinaryOffset = cursor;
        if (!checkedAdd(cursor, header.featureBinaryLength, cursor))
            return fail(error, "3D Tiles section length overflows");
        header.batchJsonOffset = cursor;
        if (!checkedAdd(cursor, header.batchJsonLength, cursor))
            return fail(error, "3D Tiles section length overflows");
        header.batchBinaryOffset = cursor;
        if (!checkedAdd(cursor, header.batchBinaryLength, cursor) || cursor > totalSize)
            return fail(error, "3D Tiles sections exceed declared length");

        header.payloadOffset = cursor;
        header.payloadSize = totalSize - cursor;
        header.gltfFormat = isB3dm ? 1u : readU32(data, 28);
        if (isI3dm && header.gltfFormat > 1u)
            return fail(error, "unsupported i3dm gltfFormat");
        if (header.gltfFormat == 1u && header.payloadSize < 12u)
            return fail(error, "embedded GLB is truncated");
        return true;
    }

    bool appendRemoteChunk(std::vector<unsigned char>& output, const char* data, size_t size,
                           size_t maxBytes, std::string* error)
    {
        if (size == 0) return true;
        if (!data) return fail(error, "remote chunk has no data");
        if (maxBytes == 0 || output.size() > maxBytes || size > maxBytes - output.size())
            return fail(error, "remote resource exceeds configured limit");
        const unsigned char* bytes = reinterpret_cast<const unsigned char*>(data);
        output.insert(output.end(), bytes, bytes + size);
        return true;
    }

    bool parseContentLength(const std::string& value, size_t& result)
    {
        result = 0;
        size_t first = 0, last = value.size();
        while (first < last && std::isspace(static_cast<unsigned char>(value[first]))) ++first;
        while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1]))) --last;
        if (first == last) return false;
        size_t parsed = 0;
        for (size_t i = first; i < last; ++i)
        {
            const unsigned char c = static_cast<unsigned char>(value[i]);
            if (c < '0' || c > '9') return false;
            const size_t digit = static_cast<size_t>(c - '0');
            if (parsed > (std::numeric_limits<size_t>::max() - digit) / 10) return false;
            parsed = parsed * 10 + digit;
        }
        result = parsed; return true;
    }
}
