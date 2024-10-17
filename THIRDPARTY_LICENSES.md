### Thirdparty libraries embedded / depended by osgVerse
| Library          | Mode     | Version   | Used by modules    | License      | Website |
|------------------|----------|-----------|--------------------|--------------|---------|
| OpenSceneGraph   | Depended | > 3.1.1   | All                | LGPL (+)     | https://github.com/openscenegraph/OpenSceneGraph |
| SDL2             | Depended |           | App, Android, WASM | Zlib         | https://github.com/libsdl-org/SDL |
| Bullet           | Depended | > 3.17    | Animation          | Zlib (+)     | https://github.com/bulletphysics/bullet3 |
| ZLMediaKit       | Depended |           | verse_ms           | MIT          | https://github.com/ZLMediaKit/ZLMediaKit |
| libDraco         | Depended | > 1.5     | ReaderWriter       | Apache2      | https://github.com/google/draco |
| libIGL           | Depended | > 2.5     | Modeling           | MPL2         | https://github.com/libigl/libigl |
| OpenVDB          | Depended | > 10.0    | verse_vdb          | MPL2         | https://github.com/AcademySoftwareFoundation/openvdb |
| osgEarth         | Depended | > 2.10    | App                | LGPL (+)     | https://github.com/gwaldron/osgearth |
| Qt               | Depended | > 5.5     | App                | LGPL         | |
| any              | Embedded |           | -                  | Boost        | https://github.com/thelink2012/any |
| ApproxMVBB       | Embedded |           | Modeling           | MPL2         | https://github.com/gabyx/ApproxMVBB |
| backward-cpp     | Embedded |           | -                  | MIT          | https://github.com/bombela/backward-cpp |
| blend2d          | Embedded | 0.11.4    | Pipeline           | Zlib         | https://github.com/blend2d/blend2d |
| BSplineFitting   | Embedded |           | Modeling           | MIT          | https://github.com/QianZheng/BSplineFitting/tree/master |
| CDT              | Embedded | 1.4.0     | Modeling           | MPL2         | https://github.com/artem-ogre/CDT |
| Clipper2         | Embedded | 1.3.0     | Modeling           | Boost        | https://github.com/collmot/Clipper2/tree/feat/cpp11-support |
| Eigen            | Embedded |           | -                  | MPL2         | https://gitlab.com/libeigen/eigen |
| entt             | Embedded |           | -                  | MIT          | https://github.com/skypjack/entt |
| exprtk           | Embedded |           | Modeling           | MIT          | https://github.com/ArashPartow/exprtk |
| Fir & Iir        | Embedded |           | Animation          | MIT          | https://github.com/berndporr |
| gainput          | Embedded |           | -                  | MIT          | https://github.com/jkuhlmann/gainput |
| ghc_filesystem   | Embedded |           | -                  | MIT          | https://github.com/gulrak/filesystem |
| ImFileDialog     | Embedded |           | UI                 | MIT          | https://github.com/dfranx/ImFileDialog |
| imgui            | Embedded | 1.83      | UI, App            | MIT          | https://github.com/ocornut/imgui |
| ImGuizmo         | Embedded |           | UI                 | MIT          | https://github.com/CedricGuillemet/ImGuizmo |
| imnode-editor    | Embedded |           | UI                 | MIT          | https://github.com/thedmd/imgui-node-editor |
| implot           | Embedded |           | UI                 | MIT          | https://github.com/epezent/implot |
| ktx              | Embedded |           | verse_ktx          | Apache2 (+)  | https://github.com/KhronosGroup/KTX-Software |
| laplace_deform   | Embedded |           | -                  | -            | |
| laszip           | Embedded |           | verse_ept          | Apache2      | https://github.com/LASzip/LASzip |
| layout           | Embedded |           | -                  | -            | |
| least-squares    | Embedded |           | Modeling           | MIT          | https://github.com/Rookfighter/least-squares-cpp |
| leveldb          | Embedded |           | verse_db           | BSD3         | https://github.com/google/leveldb |
| libhv            | Embedded |           | verse_web          | BSD3         | https://github.com/ithewei/libhv |
| lightmapper      | Embedded |           | -                  | -            | https://github.com/ands/lightmapper |
| marl             | Embedded |           | -                  | Apache2      | https://github.com/google/marl |
| MeshOptimizer    | Embedded | 0.26      | Modeling           | MIT          | https://github.com/zeux/meshoptimizer |
| mikktspace       | Embedded |           | Pipeline           | Public       | https://github.com/mmikk/MikkTSpace |
| miniz            | Embedded |           | -                  | MIT          | https://github.com/richgel999/miniz |
| mio              | Embedded |           | -                  | MIT          | https://github.com/vimpunk/mio |
| nanoflann        | Embedded |           | -                  | BSD          | https://github.com/jlblancoc/nanoflann |
| nanoid           | Embedded |           | -                  | MIT          | https://github.com/mcmikecreations/nanoid_cpp |
| NormalGenerator  | Embedded |           | Pipeline           |              | |
| OpenFBX          | Embedded |           | verse_fbx          | MIT          | https://github.com/nem0/OpenFBX |
| ozz-animation    | Embedded |           | Animation          | MIT          | https://github.com/guillaumeblanc/ozz-animation |
| picojson         | Embedded |           | -                  | BSD2         | https://github.com/kazuho/picojson |
| pinyin           | Embedded |           | UI                 |              | |
| PoissonGenerator | Embedded |           | Pipeline           | -            | |
| polylabel        | Embedded | 2.0.1     | Modeling           | ISC          | https://github.com/mapbox/polylabel |
| pmp-library      | Embedded |           | Modeling           | MIT          | https://github.com/pmp-library/pmp-library |
| quickjs          | Embedded |           | Script             | MIT          | https://github.com/bellard/quickjs |
| rapidjson        | Embedded |           | -                  | MIT (+)      | https://github.com/Tencent/rapidjson |
| rapidxml         | Embedded |           | -                  | Boost / MIT  | https://rapidxml.sourceforge.net/ |
| rasterizer       | Embedded |           | Pipeline           | -            | https://github.com/rawrunprotected/rasterizer |
| recastnavigation | Embedded |           | AI                 | Zlib         | https://github.com/recastnavigation/recastnavigation |
| span             | Embedded |           | -                  | Boost        | https://github.com/tcbrindle/span |
| sqlite3          | Embedded |           | -                  | Public       | https://www.sqlite.org/index.html |
| stb              | Embedded |           | -                  | MIT / Public | https://github.com/nothings/stb |
| strtk            | Embedded |           | -                  | MIT          | https://github.com/ArashPartow/strtk |
| supercluster     | Embedded |           | Pipeline           | ISC          | https://github.com/mapbox/supercluster.hpp |
| tiny_gltf        | Embedded |           | verse_gltf         | MIT          | https://github.com/syoyo/tinygltf |
| tiny_gltf_loader | Embedded |           | verse_gltf         | MIT          | https://github.com/syoyo/tinygltfloader |
| tiny_obj_loader  | Embedded |           | -                  | MIT          | https://github.com/tinyobjloader/tinyobjloader |
| tinyexr          | Embedded |           | -                  | BSD          | https://github.com/syoyo/tinyexr |
| tinyfiledialogs  | Embedded | 3.18.2    | -                  | Zlib         | https://sourceforge.net/projects/tinyfiledialogs |
| tinyspline       | Embedded |           | -                  | MIT          | https://github.com/msteinbeck/tinyspline |
| tweeny           | Embedded |           | Animation          | MIT          | https://github.com/mobius3/tweeny |
| VHACD            | Embedded | 4.1       | Modeling           | BSD3         | https://github.com/kmammou/v-hacd |
| xatlas           | Embedded |           | -                  | MIT          | https://github.com/jpcy/xatlas |
| xxYUV            | Embedded |           | -                  | MIT          | https://github.com/metarutaiga/xxYUV |