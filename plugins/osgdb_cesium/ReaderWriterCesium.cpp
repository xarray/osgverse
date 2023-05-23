#include <osg/io_utils>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/ImageStream>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgDB/Archive>

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <Cesium3DTilesSelection/Tileset.h>
#include <Cesium3DTilesSelection/registerAllTileContentTypes.h>
// reference: https://github.com/timoore/vsgCs

class FileAssetResponse : public CesiumAsync::IAssetResponse
{
public:
    FileAssetResponse(const std::string& fileName, uint16_t statusCode,
                      const std::string& contentType, const CesiumAsync::HttpHeaders& headers,
                      const std::vector<std::byte>& fileData)
        : _fileName(fileName), _statusCode(statusCode), _contentType(contentType),
          _headers(headers), _fileData(fileData) {}

    uint16_t statusCode() const noexcept override { return _statusCode; }
    virtual const CesiumAsync::HttpHeaders& headers() const override { return this->_headers; }
    std::string contentType() const noexcept override { return _contentType; }

    gsl::span<const std::byte> data() const noexcept override
    { return gsl::span<const std::byte>(_fileData.data(), _fileData.size()); }

private:
    std::string _fileName;
    uint16_t _statusCode;
    std::string _contentType;
    CesiumAsync::HttpHeaders _headers;
    std::vector<std::byte> _fileData;
};

class FileAssetRequest : public CesiumAsync::IAssetRequest
{
public:
    FileAssetRequest(const std::string& method, const std::string& url,
                     const CesiumAsync::HttpHeaders& headers) noexcept
    {
        uint16_t statusCode = 200;
        std::vector<std::byte> contents;
        std::ifstream ifs(url, std::ios::in | std::ios::binary);
        if (!ifs)
            statusCode = 404;
        else
        {
            std::string fileData = std::string(
                std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
            contents.resize(fileData.size());
            std::transform(fileData.begin(), fileData.end(), contents.begin(),
                           [](const char& ch) { return std::byte(ch); });
        }
        printf("%s: %zd\n", url.c_str(), contents.size());
        
        // TODO: The content type is ONLY determined by the file extension here!
        std::string actualExtension = "." + osgDB::getFileExtension(url);
        std::vector<std::string> extensions { ".json", ".b3dm", ".cmpt", ".glTF", ".glb", ".subtree" };
        for (const std::string& extension : extensions)
        {
            if (actualExtension == extension)
            {
                std::string contentType = extension;
                _pResponse = std::make_unique<FileAssetResponse>(
                    url, statusCode, contentType, headers, contents);
            }
        }

        if (!_pResponse)
            OSG_WARN << "[FileAssetRequest] Unkonwn content type for " << url << std::endl;
    }

    virtual const std::string& method() const override { return this->_method; }
    virtual const CesiumAsync::HttpHeaders& headers() const override { return this->_headers; }
    const std::string& url() const noexcept override { return this->_url; }
    CesiumAsync::IAssetResponse* response() const noexcept override { return this->_pResponse.get(); }

private:
    std::string _method;
    std::string _url;
    CesiumAsync::HttpHeaders _headers;
    std::unique_ptr<CesiumAsync::IAssetResponse> _pResponse;
};

class FileAssetAccessor : public CesiumAsync::IAssetAccessor
{
public:
    CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>> get(
            const CesiumAsync::AsyncSystem& asyncSystem, const std::string& url,
            const std::vector<THeader>& headers = {}) override
    {
        CesiumAsync::HttpHeaders httpHeaders; std::string fullUrl = databasePath + url;
        for (size_t i = 0; i < headers.size(); ++i)
        {
            if (headers[i].first == "DatabasePath") fullUrl = headers[i].second + url;
            else httpHeaders.insert(headers[i]);
        }

        std::shared_ptr<CesiumAsync::IAssetRequest> value =
            std::make_shared<FileAssetRequest>("GET", fullUrl, httpHeaders);
        CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>> result =
            asyncSystem.createResolvedFuture(std::shared_ptr<CesiumAsync::IAssetRequest>(value));
        return result;
    }

    virtual CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>> request(
            const CesiumAsync::AsyncSystem& asyncSystem, const std::string& verb,
            const std::string& url, const std::vector<THeader>& headers = std::vector<THeader>(),
            const gsl::span<const std::byte>& contentPayload = {}) override
    {
        CesiumAsync::HttpHeaders httpHeaders; std::string fullUrl = databasePath + url;
        for (size_t i = 0; i < headers.size(); ++i)
        {
            if (headers[i].first == "DatabasePath") fullUrl = headers[i].second + url;
            else httpHeaders.insert(headers[i]);
        }

        std::shared_ptr<CesiumAsync::IAssetRequest> value =
            std::make_shared<FileAssetRequest>(verb, fullUrl, httpHeaders);
        CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>> result =
            asyncSystem.createResolvedFuture(std::shared_ptr<CesiumAsync::IAssetRequest>(value));
        return result;
    }

    void tick() noexcept override {}
    std::string databasePath;
};

class ResourcePreparer : public Cesium3DTilesSelection::IPrepareRendererResources
{
public:
    ResourcePreparer() {}

    CesiumAsync::Future<Cesium3DTilesSelection::TileLoadResultAndRenderResources> prepareInLoadThread(
            const CesiumAsync::AsyncSystem& asyncSystem,
            Cesium3DTilesSelection::TileLoadResult&& tileLoadResult,
            const glm::dmat4& transform, const std::any& rendererOptions) override
    {
        Cesium3DTilesSelection::TileLoadResultAndRenderResources res;
        CesiumAsync::Future<Cesium3DTilesSelection::TileLoadResultAndRenderResources> r =
            asyncSystem.createResolvedFuture(Cesium3DTilesSelection::TileLoadResultAndRenderResources(res));
        
        if (std::holds_alternative<CesiumGltf::Model>(tileLoadResult.contentKind))
        {
            CesiumGltf::Model& model = std::get<CesiumGltf::Model>(tileLoadResult.contentKind);
            std::cout << model.scenes.size() << "... prepareInLoadThread\n";
        }
        return r;  // TODO
    }

    void* prepareInMainThread(Cesium3DTilesSelection::Tile& tile, void* pLoadThreadResult) override
    {
        std::string id = Cesium3DTilesSelection::TileIdUtilities::createTileIdString(tile.getTileID());
        std::cout << "prepareInMainThread " << id << "\n"; 
        return nullptr;  // TODO
    }

    void free(Cesium3DTilesSelection::Tile& tile, void* pLoadThreadResult,
              void* pMainThreadResult) noexcept override
    { return; }  // TODO

    void* prepareRasterInLoadThread(CesiumGltf::ImageCesium& image,
                                    const std::any& rendererOptions)  override
    { return nullptr; }  // TODO

    void* prepareRasterInMainThread(Cesium3DTilesSelection::RasterOverlayTile& rasterTile,
                                    void* pLoadThreadResult) override
    { return nullptr; }  // TODO

    void freeRaster(const Cesium3DTilesSelection::RasterOverlayTile& rasterTile,
                    void* pLoadThreadResult, void* pMainThreadResult) noexcept override
    { return; }  // TODO

    void attachRasterInMainThread(
        const Cesium3DTilesSelection::Tile& tile, int32_t overlayTextureCoordinateID,
        const Cesium3DTilesSelection::RasterOverlayTile& rasterTile,
        void* pMainThreadRendererResources, const glm::dvec2& translation,
        const glm::dvec2& scale) override
    { return; }  // TODO

    void detachRasterInMainThread(
        const Cesium3DTilesSelection::Tile& tile, int32_t overlayTextureCoordinateID,
        const Cesium3DTilesSelection::RasterOverlayTile& rasterTile,
        void* pMainThreadRendererResources) noexcept override
    { return; }  // TODO
};

class TaskProcessor : public CesiumAsync::ITaskProcessor
{
public:
    TaskProcessor() : _sleepDurationMs(50), _blocking(false) {}
    TaskProcessor(int32_t ms, bool b) : _sleepDurationMs(ms), _blocking(b) {}

    void startTask(std::function<void()> f) override
    {
        std::thread thread([this, f] {
            try
            {
                if (_sleepDurationMs > 0)
                {
                    std::chrono::milliseconds duration{ std::chrono::milliseconds(_sleepDurationMs) };
                    std::this_thread::sleep_for(duration);
                }
                f();
            }
            catch (const std::exception& e)
            {
                OSG_WARN << "[TaskProcessor] Exception in background thread: " << e.what() << std::endl;
            }
        });

        if (_blocking) thread.join();
        else thread.detach();
    }

private:
    int32_t _sleepDurationMs;
    bool _blocking;
};

class ReaderWriterCesium : public osgDB::ReaderWriter
{
public:
    ReaderWriterCesium() : _externals(NULL)
    {
        supportsExtension("verse_cesium", "Pseudo file extension, used to select Cesium plugin.");
    }

    virtual ~ReaderWriterCesium()
    {
    }

    virtual const char* className() const
    { return "[osgVerse] Cesium plugin for reading/writing 3dtiles"; }

    virtual ReadResult readNode(const std::string& fullFileName, const Options* options) const
    {
        std::string fileName(fullFileName);
        std::string ext = osgDB::getFileExtension(fullFileName);
        bool usePseudo = (ext == "verse_cesium");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(fullFileName);
            ext = osgDB::getFileExtension(fileName);
        }

        if (!fileExists(fileName, options)) return ReadResult::FILE_NOT_FOUND;
        std::string path = osgDB::getFilePath(fileName) + osgDB::getNativePathSeparator();
        std::string file = osgDB::getSimpleFileName(fileName);

        Cesium3DTilesSelection::TilesetOptions tsOptions;
        tsOptions.maximumScreenSpaceError = 100;
        tsOptions.loadErrorCallback = [this](const Cesium3DTilesSelection::TilesetLoadFailureDetails& details)
        {
            OSG_NOTICE << "[ReaderWriterCesium] Error (Type = " << (int)details.type << ", Code = "
                       << details.statusCode << "): " << details.message << std::endl;
        };

        Cesium3DTilesSelection::Tileset tileset(getOrCreateExternals(path), file, tsOptions);
        Cesium3DTilesSelection::ViewState viewState = Cesium3DTilesSelection::ViewState::create(
            glm::dvec3{ 0.0, 0.0, 0.0 }, glm::dvec3{ 0.0, 0.0, 1.0 },
            glm::dvec3{ 0.0, 1.0, 0.0 }, glm::dvec2{ 800.0, 600.0 }, osg::PI / 2.0, osg::PI / 2.0);
        tileset.updateViewOffline({ viewState });

        bool success = waitFor(tileset.getAsyncSystem(), 1000, [&]() { return tileset.getRootTile(); });
        if (!success) return ReadResult::ERROR_IN_READING_FILE;

        traverse(tileset, tileset.getRootTile(), path, 0);
        return ReadResult::FILE_NOT_HANDLED;
    }

    virtual WriteResult writeNode(const osg::Node& node, const std::string& fullFileName,
                                  const Options* options) const
    {
        std::string fileName(fullFileName);
        std::string ext = osgDB::getFileExtension(fullFileName);
        bool usePseudo = (ext == "verse_cesium");
        if (usePseudo)
        {
            fileName = osgDB::getNameLessExtension(fullFileName);
            ext = osgDB::getFileExtension(fileName);
        }

        return WriteResult::FILE_NOT_HANDLED;
    }

protected:
    Cesium3DTilesSelection::TilesetExternals& getOrCreateExternals(const std::string& path) const
    {
        if (!_externals)
        {
            Cesium3DTilesSelection::registerAllTileContentTypes();
            _externals = new Cesium3DTilesSelection::TilesetExternals {
                std::make_shared<FileAssetAccessor>(), std::make_shared<ResourcePreparer>(),
                std::make_shared<TaskProcessor>(0, true), nullptr,
                spdlog::default_logger() };
        }

        FileAssetAccessor* fa = static_cast<FileAssetAccessor*>(_externals->pAssetAccessor.get());
        if (fa) fa->databasePath = path;
        return *_externals;
    }

    void traverse(Cesium3DTilesSelection::Tileset& tileset,
                  Cesium3DTilesSelection::Tile* tile, const std::string& path, int lv) const
    {
        std::string id = Cesium3DTilesSelection::TileIdUtilities::createTileIdString(tile->getTileID());
        /*if (tile->getState() == Cesium3DTilesSelection::TileLoadState::Unloaded)
        {
            std::vector<CesiumAsync::IAssetAccessor::THeader> headers;
            headers.push_back(CesiumAsync::IAssetAccessor::THeader("DatabasePath", path));
            
            auto future = tile->getLoader()->loadTileContent(Cesium3DTilesSelection::TileLoadInput(
                *tile, tileset.getOptions().contentOptions, _externals->asyncSystem,
                _externals->pAssetAccessor, _externals->pLogger, headers));
            _externals->asyncSystem.dispatchMainThreadTasks();

            Cesium3DTilesSelection::TileLoadResult result = future.wait();
            if (result.state == Cesium3DTilesSelection::TileLoadResultState::Success)
            {
                if (result.tileInitializer) result.tileInitializer(*tile);
                //Cesium3DTilesSelection::TileChildrenResult chilResult =
                //    tile->getLoader()->createTileChildren(*tile);
                //printf("STATE %d, CHILD %d\n", chilResult.state, chilResult.children.size());
            }
        }*/

        OSG_NOTICE << "TILE " << id << ": LEVEL = " << lv << ", " << (int)tile->getState()
                   << "; CHILD = " << tile->getChildren().size() << std::endl;
        for (Cesium3DTilesSelection::Tile& child : tile->getChildren())
        { traverse(tileset , &child, path, lv + 1); }
    }

    bool waitFor(CesiumAsync::AsyncSystem& asyncSystem,
                 uint32_t maxWaitTimeMs, std::function<bool()> condition) const
    {
        uint32_t sleepMs = 50;
        auto start = std::chrono::high_resolution_clock::now();
        while (true)
        {
            bool conditionFulfilled = condition();
            if (conditionFulfilled) break;

            asyncSystem.dispatchMainThreadTasks();
            auto current = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(current - start);
            double currentMs = duration.count() / 1e6;
            if (currentMs > maxWaitTimeMs) return false;
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
        }
        return true;
    }

    mutable Cesium3DTilesSelection::TilesetExternals* _externals;
};

// Now register with Registry to instantiate the above reader/writer.
REGISTER_OSGPLUGIN(verse_cesium, ReaderWriterCesium)
