# osgVerse - AI Coding Agent Guide

## Project Overview

**osgVerse** is a complete 3D engine solution based on OpenSceneGraph (OSG). It provides a modern rendering pipeline with PBR (Physically Based Rendering), deferred shading, real-time shadows, and comprehensive 3D functionality including physics, animation, and UI systems.

- **Language**: C++ (C++14/C++17)
- **Build System**: CMake 3.10+
- **License**: See LICENSE file

## Technology Stack

### Core Technologies
- **Graphics API**: OpenGL 2.0+, OpenGL 3.3+ Core Profile, GLES2/GLES3, WebGL 1/2
- **3D Framework**: (Minimum) OpenSceneGraph 3.1.1; (Preferred) OpenSceneGraph 3.6.5
- **Build System**: CMake 3.10+
- **Platforms**: Windows (10-11), Linux (Ubuntu/Debian/Kylin/UOS), macOS, Android, WebAssembly (Emscripten)

### Key Dependencies
- **OpenSceneGraph** (Required): Core 3D framework
- **OpenGL/GLES**: Graphics rendering
- **SDL2**: Windowing for Android/IOS/WASM
- **CUDA/MUSA** (Optional): GPU compute
- **Bullet3** (Optional): Physics simulation
- **Qt5/Qt6** (Optional): Qt-based applications
- **FFmpeg** (Optional): Video decoding/encoding
- **Draco** (Optional): Mesh compression
- **libIGL** (Optional): Geometry processing
- **ZLMediaKit** (Optional): Media streaming

### Embedded 3rdparty Libraries (3rdparty/)
The project includes many embedded third-party libraries:
- **blend2d**: 2D vector graphics engine
- **imgui**: Immediate mode GUI (with extensions like ImGuizmo, implot)
- **leveldb**: Key-value storage
- **libhv**: High-performance network library
- **ktx**: Khronos texture format
- **ozz**: Animation runtime
- **recastnavigation**: Navigation mesh
- **marl**: Task scheduler
- **meshoptimizer**: Mesh optimization
- **Eigen**: Linear algebra
- And many more (see THIRDPARTY_LICENSES.md for complete list)

## Project Structure

```
osgVerse/
├── CMakeLists.txt              # Root CMake configuration
├── Setup.sh / Setup.bat        # Automated build scripts
├── VerseCommon.h               # Main unified header
├── CODE_STYLE.md               # Coding style guidelines
│
├── 3rdparty/                   # Embedded third-party libraries
│   ├── blend2d/               # 2D graphics
│   ├── imgui/                 # GUI library
│   ├── leveldb/               # Database
│   ├── libhv/                 # Network library
│   ├── ktx/                   # Texture format
│   ├── ozz/                   # Animation
│   └── ...                    # Many more
│
├── pipeline/                   # Rendering pipeline (osgVersePipeline)
│   ├── Pipeline.h/cpp         # Core pipeline
│   ├── DeferredCallback.h/cpp # Deferred shading
│   ├── ShadowModule.h/cpp     # Shadow rendering
│   ├── LightModule.h/cpp      # Lighting system
│   ├── ShaderLibrary.h/cpp    # Shader management
│   └── ...
│
├── modeling/                   # Geometry processing (osgVerseModeling)
│   ├── MeshDeformer.h/cpp     # Mesh deformation
│   ├── GeometryMerger.h/cpp   # Geometry merging
│   ├── Math.h/cpp             # Math utilities
│   └── ...
│
├── readerwriter/              # I/O utilities (osgVerseReaderWriter)
│   ├── DracoProcessor.h/cpp   # Draco compression
│   ├── KTXProcessor.h/cpp     # KTX textures
│   ├── GLTFReader.h/cpp       # GLTF support
│   └── ...
│
├── animation/                  # Animation & physics (osgVerseAnimation)
│   ├── PhysicsEngine.h/cpp    # Bullet physics
│   ├── PlayerAnimation.h/cpp  # Character animation
│   ├── TweenAnimation.h/cpp   # Tweening
│   └── ...
│
├── ui/                         # User interface (osgVerseUI)
│   ├── imgui/                 # ImGui integration
│   ├── Canvas2D.h/cpp         # 2D canvas
│   └── ...
│
├── script/                     # Scripting (osgVerseScript)
│   └── ...
│
├── ai/                         # AI & navigation (osgVerseAI)
│   └── ...
│
├── wrappers/                   # Serialization wrappers (osgVerseWrappers)
│   └── ...
│
├── plugins/                    # OSG plugins
│   ├── osgdb_fbx/             # FBX format
│   ├── osgdb_gltf/            # GLTF format
│   ├── osgdb_ktx/             # KTX format
│   ├── osgdb_3dgs/            # 3D Gaussian Splatting
│   ├── osgdb_ffmpeg/          # Video support
│   └── ...
│
├── tests/                      # Test applications
│   ├── pipeline_test.cpp      # Pipeline testing
│   ├── shadow_test.cpp        # Shadow testing
│   ├── physics_basic_test.cpp # Physics testing
│   └── ...
│
├── applications/               # Main applications
│   ├── viewer/                # Basic viewer
│   ├── viewer_composite/      # Multi-view viewer
│   ├── scene_editor/          # Scene editor
│   ├── earth_explorer/        # Earth visualization
│   ├── qt_viewer/             # Qt integration
│   └── ...
│
├── wasm/                       # WebAssembly specific code
├── android/                    # Android specific code
├── helpers/                    # Build helpers
│   ├── toolchain_builder/     # 3rdparty build tools
│   └── osg_builder/           # OSG build helpers
│
├── assets/                     # Assets (models, shaders, textures)
│   ├── models/                # 3D models
│   ├── shaders/               # GLSL shaders
│   ├── skyboxes/              # HDR skyboxes
│   └── textures/              # Textures
│
├── cmake/                      # CMake modules
│   ├── VerseMacros.cmake      # Build macros
│   └── ...
│
└── build/                      # Build output (generated)
```

## Module Dependency Chain

| Module | Dependencies | Optional External |
|--------|-------------|-------------------|
| osgVerseDependency | - | - |
| osgVerseModeling | Dependency | libIGL |
| osgVersePipeline | Dependency, Modeling | CUDA |
| osgVerseScript | Dependency, Pipeline | - |
| osgVerseAI | Dependency, Modeling | - |
| osgVerseAnimation | Dependency, Pipeline, Modeling | Bullet, Effekseer |
| osgVerseUI | Dependency, Modeling, Script | libCEF |
| osgVerseReaderWriter | Dependency, Animation, Modeling, Pipeline | libDraco, SDL |
| osgVerseWrappers | ALL | - |

## Build Instructions

### Quick Build (Recommended)

Use the provided setup scripts:

**Linux/macOS:**
```bash
./Setup.sh [optional_path]
```

**Windows:**
```cmd
Setup.bat [optional_path]
```

The script will:
1. Download OpenSceneGraph if not present
2. Build third-party libraries
3. Build OpenSceneGraph
4. Build osgVerse

### Manual Build

```bash
# 1. Ensure OpenSceneGraph is built and OSG_ROOT is set
export OSG_ROOT=/path/to/osg

# 2. Create build directory
mkdir build && cd build

# 3. Configure
cmake .. -DOSG_ROOT=$OSG_ROOT

# 4. Build
cmake --build . --target install --config Release
```

### Key CMake Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `OSG_ROOT` | Path | - | OpenSceneGraph root directory |
| `VERSE_3RDPARTY_PATH` | Path | ../Dependencies | Third-party libraries path |
| `VERSE_BUILD_EXAMPLES` | Bool | ON | Build examples and tests |
| `VERSE_BUILD_WITH_QT` | Bool | ON | Build Qt-based applications |
| `VERSE_BUILD_WITH_CUDA` | Bool | ON | Build CUDA-based libraries |
| `VERSE_STATIC_BUILD` | Bool | OFF | Static library build |
| `VERSE_USE_OSG_STATIC` | Bool | OFF | Use static OSG |
| `VERSE_SUPPORT_CPP17` | Bool | ON | Enable C++17 features |
| `VERSE_BUILD_DEPRECATED_TESTS` | Bool | OFF | Build deprecated tests |

### Platform-Specific Builds

**WebAssembly (Emscripten):**
```bash
./Setup.sh /path/to/emsdk
# Select option 3 (WebGL 1) or 4 (WebGL 2)
```

**Android:**
```bash
# Set ANDROID_SDK and ANDROID_NDK environment variables
export ANDROID_SDK=/path/to/sdk
export ANDROID_NDK=/path/to/ndk
./Setup.sh
# Select option 5 (Android)
```

**OpenGL ES:**
```bash
./Setup.sh /path/to/gles/libs
# Select option 2 (GLES)
```

## Code Style Guidelines

See `CODE_STYLE.md` for full details. Key points:

### Formatting
- 4 spaces indent
- 8 spaces continuation indent  
- 100 columns max
- Opening brace `{` on new line
- Spaces around operators

### Naming Conventions
- **Constants**: UPPER_CASE (e.g., `FOO_COUNT`)
- **Global variables**: `g_` prefix (e.g., `g_globalWarming`)
- **Static/class variables**: `s_` prefix
- **Private/protected attributes**: `_` prefix (e.g., `_attributeName`)
- **Class names**: UpperCamelCase (e.g., `FooBar`)
- **Methods/variables**: lowerCamelCase (e.g., `methodName`)

### File Conventions
- Headers: `.h`
- Implementation: `.cpp`
- Inline includes: `.inline`
- Public headers: `#include <module/Header.h>`
- Private headers: `#include "Header.h"`

### Example:
```cpp
class FooBar
{
public:
    void methodName(int arg1, bool arg2);
    int sizeInBytes;
    
private:
    int _attributeName;
    static int s_globalAttribute;
    enum { ONE, TWO, THREE };
};
```

## Testing

### Test Organization
Tests are in `tests/` directory and categorized as:

**Regular Tests (NEW_TEST):**
- `osgVerse_Test_Compressing` - KTX/Draco compression
- `osgVerse_Test_Thread` - Marl task scheduler
- `osgVerse_Test_Volume_Rendering` - Volume rendering methods
- `osgVerse_Test_Auto_LOD` - Geometry merging/optimization
- `osgVerse_Test_Sky_Box` - Skybox and atmosphere

**Examples (NEW_EXAMPLE):**
- `osgVerse_Test_Pipeline` - Basic pipeline usage
- `osgVerse_Test_Shadow` - Shadow algorithms
- `osgVerse_Test_Forward_Pbr` - Forward PBR rendering
- `osgVerse_Test_ImGui` - ImGui integration
- `osgVerse_Test_Earth` - Earth/atmosphere/ocean
- `osgVerse_Test_Physics_Basic` - Physics simulation (requires Bullet)
- And many more...

### Running Tests
Tests are built automatically when `VERSE_BUILD_EXAMPLES=ON`. Run from build directory:
```bash
./bin/osgVerse_Test_Pipeline
```

## Development Conventions

### Adding a New Library

1. Add source files to appropriate module's `CMakeLists.txt`
2. Use `NEW_LIBRARY()` macro for regular libraries
3. Use `NEW_CUDA_LIBRARY()` for CUDA/MUSA libraries
4. Link dependencies using `TARGET_LINK_LIBRARIES()`

### Adding a New Plugin

1. Create subdirectory in `plugins/`
2. Create `ReaderWriterXXX.cpp` implementing OSG plugin interface
3. Add subdirectory to `plugins/CMakeLists.txt`
4. Use `NEW_PLUGIN()` macro

### Adding a New Test

1. Add `.cpp` file to `tests/`
2. Add `NEW_TEST(exe_name source.cpp)` to `tests/CMakeLists.txt`
3. Or use `NEW_EXAMPLE()` for example applications

### Export Macros

Use export macros for cross-platform compatibility:
```cpp
#include <wrappers/Export.h>
// For pipeline: OSGVERSE_PIPELINE_EXPORT
// For modeling: OSGVERSE_MODELING_EXPORT
// etc.
```

## Key Architecture Patterns

### Pipeline Architecture
The rendering pipeline uses a stage-based architecture:
- `Pipeline`: Main orchestrator
- `Pipeline::Stage`: Individual render pass
- Modules (ShadowModule, LightModule, etc.) extend functionality

### Memory Management
- Uses OSG's reference counting (`osg::ref_ptr`)
- Custom allocators available via `VERSE_USE_MIMALLOC`

### SIMD Support
Automatic SIMD detection and compilation:
- SSE/SSE2/SSE3/SSSE3/SSE4.1/SSE4.2
- AVX/AVX2/AVX512
- Automatic flags applied via `VERSE_SIMD_FEATURES`

## Security Considerations

1. **Buffer Overflow**: Use OSG's array classes and bounds checking
2. **Shader Injection**: Validate all shader inputs
3. **File I/O**: Use OSG's virtual file system for sandboxing
4. **Network**: libhv includes mbedtls for TLS support

## Troubleshooting

### Common Build Issues

**OpenSceneGraph not found:**
- Set `OSG_ROOT` environment variable or CMake variable
- Or place OSG source at `../OpenSceneGraph`

**GLES libraries not found:**
- For GLES builds, provide path to libEGL.so and libGLESv2.so
- Use `Setup.sh <path_to_gles_libs>`

**CUDA/Compiler incompatibility:**
- Check CUDA version against compiler version compatibility
- See CMakeLists.txt line ~353-396 for version checks

**Static build issues:**
- Enable `VERSE_USE_OSG_STATIC` to use static OSG
- This forces `VERSE_STATIC_BUILD` automatically

### Platform-Specific Notes

**Linux (Kylin/UOS):**
- May need `VERSE_FIND_LEGACY_OPENGL=ON`
- May need `VERSE_USE_GLIBCXX11_ABI=OFF` for older systems

**macOS:**
- Uses `.so` suffix for shared libraries
- May need Google Angle for GLES support

**Windows:**
- Use Visual Studio 2017-2022 recommended
- MSYS2/MinGW also supported
- PDB files installed with `VERSE_INSTALL_PDB_FILES=ON`

## Resources

- **README.md**: Full project documentation
- **CODE_STYLE.md**: Detailed coding standards
- **TODO_cn.md**: Development roadmap (Chinese)
- **THIRDPARTY_LICENSES.md**: Third-party license information

## Contributing

When contributing:
1. Follow the code style in `CODE_STYLE.md`
2. Test on multiple platforms if possible
3. Update this file if build processes change
4. Add tests for new features
