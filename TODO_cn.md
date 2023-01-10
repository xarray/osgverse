# osgVerse TODO
- 模块: 编译，系统，渲染，模型，动画，编辑器，插件，例程，脚本，其它
- 所属库：osgVerse的库/例程目录名，或者留空
- 任务类型：新增，优化，除虫
- 完成情况：百分比，已完成(:heavy_check_mark:)，已废止(:heavy-ballot-x:)，或者留空
- 对应版本：initial（版本0），或者版本号；列表中按照版本号从大到小排列任务
- 已完成的"新增"任务如果需要修改/除虫，则新建TODO列表行
- 已完成的"修改/除虫"任务如果需要再次修改/除虫，则在原列表行直接改动

#### TODO列表
| 模块      | 所属库       | 类型   | 完成情况           | 对应版本 | 具体内容描述 | 备注 |
|-----------|--------------|--------|--------------------|----------|------------------------|
| 编译      |              | 新增   | :heavy_check_mark: | initial  | 支持backward-cpp自动打印崩溃时的程序堆栈 | |
| 编译      |              | 新增   | :heavy_check_mark: | initial  | 支持Static编译流程 | |
| 编译      |              | 新增   | :heavy_check_mark: | initial  | 支持MinGW编译流程 | |
| 编译      |              | 除虫   | :heavy_check_mark: | initial  | 解决ept插件和laszip的Ubuntu编译问题 | |
| 编译/渲染 |              | 新增   | :heavy_check_mark: | initial  | 支持GL3 Core Profile | |
| 编译/渲染 |              | 新增   |                    | initial  | 支持GLES2 / GLES3 | |
| 编译/渲染 |              | 新增   |                    | initial  | 支持Angel并通过自定义的GraphicsWindow来切换不同底层(DX/Vulkan) | |
| 编译/渲染 |              | 新增   | 50%                | initial  | 支持GLSL 1.2，可以运行在虚拟机和低端国产显卡上 | 目前着色器可以编译通过，但是虚拟机和兆芯笔记本运行无结果 |
| 编译      |              | 新增   |                    | initial  | 支持Android编译流程，支持直接纳入Android Studio | |
| 编译      |              | 新增   |                    | initial  | 支持Apple Mac OSX和IOS编译流程 | |
| 编译      |              | 新增   |                    | initial  | 支持Emscripten / WebAssembly编译流程，可以输出到浏览器端 | |
| 渲染      | pipeline     | `除虫` |                    | initial  | 帧速率较低时，会明显感受到Deferred场景比Forward慢一拍 | |
| 渲染      | pipeline     | 除虫   | :heavy_check_mark: | initial  | SkyBox对于大坐标场景显示错误，并且被裁切 | |
| 渲染      | pipeline     | 除虫   | :heavy_check_mark: | initial  | SkyBox用tex2d纹理时，会有一条明显的接缝边界线 | |
| 渲染      | pipeline     | 新增   |                    | initial  | SkyBox中支持后处理的Atmospheric Scattering天空盒 | |
| 渲染      | pipeline     | 除虫   | :heavy_check_mark: | initial  | NodeSelector会被物体遮挡 | |
| 渲染      | pipeline     | 除虫   |                    | initial  | 对于大坐标模型，NodeSelector无法正确显示 | |
| 渲染      | pipeline     | 优化   |                    | initial  | ShadowModule支持PSM/LispSM/TSM来提升阴影质量 | |
| 渲染      | pipeline     | 优化   |                    | initial  | ShadowModule支持EVSM/PCSS软阴影 | |
| 渲染      | pipeline     | 新增   |                    | initial  | ShadowModule支持来自多个光源的阴影，包括点光源和平行光 | |
| 渲染      | pipeline     | 优化   | :heavy_check_mark: | initial  | 需要明确贴图metallic和roughness是如何表达的，软件如何导出 | 系统中统一采用Diffusex4, Normalx3, Specularx4, Occlusionx1+Roughnessx1+Metallicx1, Ambientx3, Emissionx3的格式 |
| 渲染      | pipeline     | 除虫   | :heavy_check_mark: | initial  | 解决Sponza法线贴图不能共享以及matallic闪烁的问题 | |
| 渲染      | pipeline     | 优化   |                    | initial  | 对于大坐标模型，阴影bias需要根据坡度值动态修改PolygonOffset | |
| 渲染      | pipeline     | 除虫   | :heavy_check_mark: | initial  | 全屏/窗口切换或者缩放窗口大小后，多层阴影显示不正确 | |
| 渲染      | pipeline     | `除虫` |                    | initial  | 处理某些模型时，HBAO法线出现大量花斑 | |
| 渲染      | pipeline     | `除虫` |                    | initial  | 对于小物件，AO的效果噪声比较严重，且Bloom结果不好 | |
| 渲染      | pipeline     | `除虫` |                    | initial  | 对于简单几何体，光照容易产生异常边界线，效果不好 | |
| 渲染      | pipeline     | 优化   | 35%                | initial  | 在PBR Lighting过程中通过LightManager光源数据表计算多种光照结果 | 目前只支持了平行光，还需要支持点光源和锥光源，并且优化光照效果 |
| 渲染/例程 | pipeline     | 新增   |                    | initial  | 增加一个测试案例演示如何增加多个光源，并测试多光源渲染压力 | |
| 渲染      | pipeline     | 除虫   | :heavy_check_mark: | initial  | 发现OSG 311版本中渲染报错，和Texture2DArray有关 | |
| 渲染      | pipeline     | 除虫   | :heavy_check_mark: | initial  | 发现311版本中缩放窗口后多层阴影的问题，因为没有实现resize() | |
| 渲染      | pipeline     | 除虫   |                    | initial  | 发现摩尔S50/S80显卡上延迟和非延迟场景的深度融合错误 | |
| 渲染      | pipeline     | 优化   |                    | initial  | 后处理Bloom的模糊处理方法不理想，目前可能会产生锯齿 | |
| 渲染      | pipeline     | 新增   | 50%                | initial  | 支持后处理抗锯齿方案，FXAA/TAA | 目前已经支持FXAA |
| 渲染/例程 | pipeline     | 新增   |                    | initial  | 支持自己扩展Pipeline::Stage，增加一个测试案例演示使用NV HBAO | |
| 渲染      | pipeline     | 新增   |                    | initial  | 通过脚本的方式来管理标准和自定义的Pipeline | |
| 插件      | plugins      | 新增   | 80%                | initial  | 支持伪插件方式自动替换PBR贴图顺序(?.D4,S3,N3,X1M1R1.pbrlayout) | 已实现，未测试 |
| 插件      | helpers      | 新增   | :heavy_check_mark: | initial  | 实现Unity的导出插件第一版(静态模型，PBR材质) | |
| 插件      | helpers      | 新增   |                    | initial  | 实现Blender的导出插件第一版(静态模型，PBR材质) | |
| 插件      | helpers      | 新增   |                    | initial  | 实现3dsmax的导出插件第一版(静态模型，PBR材质) | |
| 模型      | readerwriter | 新增   |                    | initial  | 自动检查输入几何体的正确性，尝试用Indirect替换优化 | |
| 模型      | readerwriter | 新增   |                    | initial  | 支持流传输修改模型，实现Blender和编辑器的动态模型切换编辑 | |
| 模型/动画 | readerwriter | 新增   |                    | initial  | FBX和GLTF插件支持导入角色和角色动画并显示 | |
| 脚本      | ui           | 新增   |                    | initial  | 考虑合适的方式接入Lua和Python | |
| 脚本      | ui           | 新增   |                    | initial  | 使用Serialization映射OSG和osgVerse的核心函数 | |
| 脚本      | ui           | 新增   |                    | initial  | 支持通过脚本创建UserComponent接口以及Verse插件 | |
| 编辑器    | applications | 新增   |                    | initial  | 支持窗口的位置和大小自动对齐 | |
| 编辑器    | applications | 新增   |                    | initial  | 显示一个独立的XYZ轴向坐标，用来指示3D场景坐标系 | |
| 编辑器    | applications | 新增   |                    | initial  | 显示工具图标和播放控制按钮，标题栏，工程名 | |
| 编辑器    | applications | 新增   |                    | initial  | 点击场景对象，自动更新层次编辑器和属性编辑器显示 | |
| 编辑器    | applications | 新增   |                    | initial  | 层次编辑器支持共享节点，被隐藏节点的特殊显示 | |
| 编辑器    | applications | 除虫   |                    | initial  | 大坐标模型在UI操作Transform移动时，数据变化异常 | |
| 编辑器    | applications | 新增   |                    | initial  | 漫游器支持以选中对象为中心旋转，并且自动识别对象实际几何中心 | |
| 编辑器    | applications | 新增   |                    | initial  | 节点Mask采用类似Unity Layer的下拉列表，可以自定义 | |
| 编辑器    | applications | 新增   |                    | initial  | 层次编辑器支持选择多个节点，显示包围盒 | |
| 编辑器    | applications | 新增   |                    | initial  | 层次编辑器支持剪切/拷贝/粘贴节点，共享/解除共享节点 | |
| 编辑器    | applications | 新增   |                    | initial  | 支持新建相机对象(用于RTT)，新建光源对象 | |
| 编辑器    | applications | 新增   |                    | initial  | 支持LOD对象和PagedLOD对象，层次编辑器合理显示PagedLOD层次 | |
| 编辑器    | applications | 新增   |                    | initial  | 层次编辑器在MainCamera下自动加入所有Slave(以及Stage)显示 | |
| 编辑器    | applications | 新增   |                    | initial  | 属性编辑器中删除属性，复制/粘贴属性，上移/下移属性 | |
| 编辑器    | applications | 新增   |                    | initial  | 属性编辑器中新增、删除、替换纹理图片，对接资源管理 | |
| 编辑器    | applications | 新增   |                    | initial  | 属性编辑器中显示标准属性Shader/Attribute/Camera/Slave/Lod/Light | |
| 编辑器    | applications | 新增   |                    | initial  | 属性编辑器中加载插件，从插件中新建UserComponent属性 | |
| 编辑器    | applications | 新增   |                    | initial  | 搭建leveldb资源数据库，导入/管理：模型/Tile/纹理/其它资源 | |
| 编辑器    | applications | 新增   |                    | initial  | 自动获取和通过leveldb数据库管理已导入场景的资源 | |
| 编辑器    | applications | 新增   |                    | initial  | 资源窗口中显示资源列表，记录所有共享节点 | |
| 编辑器    | applications | 新增   |                    | initial  | 蜘蛛窗口中列出基本变量/基本方法/UserComponent属性方法 | |
| 编辑器    | applications | 新增   |                    | initial  | 蜘蛛窗口中选择方法并新建蜘蛛节点，连接蜘蛛节点 | |
| 编辑器    | applications | 新增   |                    | initial  | 蜘蛛窗口中剪切，复制，删除蜘蛛节点，成组/解组 | |
| 编辑器    | applications | 新增   |                    | initial  | 保存蜘蛛到当前节点(与节点对应，存为一个ValueObject) | |
| 编辑器    | applications | 新增   |                    | initial  | 时间线窗口中列出基本变量/基本方法/UserComponent属性方法 | |
| 编辑器    | applications | 新增   |                    | initial  | 时间线窗口中选择方法并新建节点，剪辑节点 | |
| 编辑器    | applications | 新增   |                    | initial  | 时间线窗口中剪切，复制，删除节点，修改通道属性 | |
| 编辑器    | applications | 新增   |                    | initial  | 保存时间线到当前节点(与节点对应，存为一个ValueObject) | |
| 编辑器    | applications | 新增   |                    | initial  | 日志窗口中，捕捉错误信息并通过backward-cpp简单显示堆栈内容 | |
| 编辑器    | applications | 新增   |                    | initial  | 日志窗口中自动清除过多的控制台数据，选中数据可复制 | |
