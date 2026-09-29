# EarthExplorer 帮助文档

EarthExplorer 是基于 osgVerse 核心库的地球可视化示例应用：显示一个可漫游的地球
（在线/本地瓦片图源），叠加可配置的"实时世界数据"标记层（灾害、地震等），并提供
一套 ImGui 界面用于图层管理、要素详情与环境控制。

设计总原则：**任何外部依赖（配置文件 / 网络 / API Key / 字体）缺失时都只降级 +
控制台告警，绝不崩溃、绝不退出**。启动后窗口、地球、UI 永远会出现。

---

## 1. 源码结构

| 文件 | 职责 |
|------|------|
| `earth_main.cpp` | 程序入口。装配顺序：加载配置 → 建 AI Provider/ChatCore → 建窗口/UI → 后台加载地球根瓦片 → 大气/海洋管线 → 城市数据(可选) → 数据源 → 漫游器 → AI 工具注册/主线程 drain → 主循环 |
| `EarthAppConfig.h/.cpp` | 统一配置系统。`loadAppConfig()` 读取 JSON，所有字段有内置默认值；`resolveKey()` 解析凭证（环境变量优先） |
| `DataSource.h/.cpp` | 数据源框架：`DataSource`（抓取线程+主线程重建标记+拾取）、`HttpJsonSource`（HTTP GET + picojson 解析基类）、`DataSourceFactory`（类型注册表）、`DataSourceManager` |
| `feed_sources.cpp` | 三个内置数据源实现（GDACS / USGS / EONET）并调用 `registerBuiltinSourceTypes()` 注册到工厂 |
| `LayerRegistry.h` | 图层注册表（线程安全）：UI 线程只提交"启用/禁用"请求，主线程 `drainPending()` 执行实际挂接 |
| `ai_chat.h/.cpp` | AI 助手核心：`ToolRegistry`（工具注册表）、`LLMProvider` 抽象 + 三种实现（OpenAI 兼容/Gemini/离线 fake）、`AIChatCore` 代理循环状态机（主线程 submit/drain + worker 线程调模型） |
| `ai_setup.h/.cpp` | AI 的地球侧装配：内置工具（fly_to / set_layer / get_view_state / get_data_summary）、默认系统提示词、工作区上下文快照 |
| `earth_ui.h/.cpp` | `EarthUIHandler`（继承核心库 `ImGuiContentHandler`）：图层面板、要素详情卡、环境控制面板、AI 聊天面板；`EnvironmentState` + `EnvironmentUpdater` 负责 UI→渲染线程的状态落地 |
| `marker_style.h/.cpp` | 标记形状/颜色库（`visualForLayer`、GLSL 点精灵形状、图层面板图标绘制），场景与 UI 共用一份 |
| `render_effects.cpp` | `configureEarthRendering()`：搭建大气散射 / 海洋 / 合成管线的 RTT 相机组 |
| `city_data.cpp` | 可选的本地城市建筑数据加载（配置 `city.enabled` 开启） |
| `ReaderWriterCSV.cpp` | CSV 读入支持（城市/标注数据用） |

依赖的核心库组件：

- `pipeline/Pipeline.h` — RTT 相机、着色器宏定义、管线骨架
- `pipeline/Utilities.h` — `EarthAtmosphereOcean`（大气/海洋）、`loadFileData`（HTTP GET 客户端）
- `readerwriter/Utilities.h` — `WebAuxiliary::httpRequest`（AI 聊天的同步 HTTP POST 客户端）
- `ui/ImGui.h` — `ImGuiManager` / `ImGuiContentHandler`（含中文输入支持）
- `readerwriter/EarthManipulator.h` — 地球漫游器（含动画路径）
- `readerwriter/FileCache.h` — 瓦片磁盘缓存（`earth_cache` 目录）
- `3rdparty/picojson` — JSON 解析

## 2. 启动时序（为什么这样设计）

```
globalInitialize → loadAppConfig(失败只告警)
  → FileCache 挂上 Registry(第二次启动根瓦片命中缓存,几乎秒开)
  → 建窗口 + ImGui(立即 realize,不等待网络)
  → std::async 后台加载 0-0-0.verse_tms,期间主线程持续 viewer.frame()
      UI 显示"正在加载地球影像..."覆盖层
  → 地球就绪:configureEarthRendering → 数据源 initialize → EarthManipulator
  → viewer.run() 进入主循环
  → 退出:viewer.stopThreading() → imgui->shutdown() → setSceneData(NULL)
```

要点：

- **窗口先行**：根瓦片是同步网络 I/O（Google 不可达时约 40 秒超时），若先加载后建窗，
  窗口会迟迟不出现。后台加载 + 覆盖层提示保证"秒开窗口"。
- **兜底球体**：`readNodeFile` 返回空（瓦片源彻底不可用）时改用内置蓝色球体，
  保证"无论如何都有一个地球"。
- **退出必须先 `stopThreading()`**：ImGui 的绘制回调在 GL 线程执行
  （`ui/ImGui.cpp` 把 PRE/POST_DRAW 回调挂在相机上），主线程直接 `shutdown()`
  会与 GL 线程竞争导致访问违例。scene_editor 的退出修复同理。

## 3. 配置文件（外部 API 的唯一来源）

搜索顺序：`--config <file>` 命令行 > `<安装目录>/misc/earth_explorer.json` >
`./earth_explorer.json`。模板见 `assets/misc/earth_explorer.json`（构建到
`sdk/misc/`）。

```jsonc
{
  "globe": {
    "orthophoto": "https://mt1.google.com/vt/lyrs%3ds&x%3d{x}&y%3d{y}&z%3d{z}",
    "elevation":  "https://s3.amazonaws.com/elevation-tiles-prod/terrarium/{z}/{x}/{y}.png.verse_terrain",
    "ocean_mask": "",
    "skirt_ratio": 0.05, "elevation_scale": 1.0,
    "web_mercator": true, "origin_bottom_left": false
  },
  "city":  { "enabled": false, "folder": "" },
  "ui":    { "font": "" },
  "api_keys": { "firms": "", "deepseek": "", "gemini": "" },
  "data_sources": [
    {
      "id": "gdacs", "type": "gdacs",
      "name": "灾害预警 (GDACS)", "group": "实时数据 / Live",
      "subtitle": "GDACS · UN 全球灾害预警",
      "url": "https://www.gdacs.org/gdacsapi/api/events/geteventlist/MAP",
      "refresh_seconds": 600, "timeout_seconds": 15, "lift_meters": 3000,
      "enabled": false, "key_env": "", "key_config": ""
    }
    // ... usgs / eonet 同构
  ]
}
```

### 字段说明

| 段 | 字段 | 说明 |
|----|------|------|
| globe | orthophoto / elevation / ocean_mask | 瓦片 URL 模板，`{z}/{x}/{y}` 占位。**URL 中的 `=` 必须写成 `%3d`**（选项串按空格切分）。留空 elevation = 纯椭球，留空 ocean_mask = 关闭海洋效果 |
| globe | web_mercator / origin_bottom_left / skirt_ratio / elevation_scale | 投影与几何参数，与 osgdb_tms 插件选项对应 |
| city | enabled / folder | 本地城市建筑（默认关） |
| ui | font | 中文字体路径；空 = 内置 LXGW 字体 |
| api_keys | *任意键名* | 凭证表。**环境变量优先于配置文件**（如 `EARTH_FIRMS_KEY`），源码不硬编码任何 key 值 |
| data_sources | 见下 | 数据源列表；数组为空时使用内置三源兜底 |

### data_sources 条目

| 字段 | 说明 |
|------|------|
| id | 稳定 id，标记形状选择（`visualForLayer`）与图层检索用 |
| type | `DataSourceFactory` 注册的类型名，未知类型 → 告警 + 跳过 |
| name / group / subtitle | UI 显示名 / 分组 / 数据来源署名 |
| url | API 地址；`{key}` 占位符被 `resolveKey()` 的结果替换 |
| key_env / key_config | 凭证来源（环境变量名 / api_keys 键名）；都为空 = 免 key |
| refresh_seconds / timeout_seconds | 轮询间隔与单次超时 |
| lift_meters | 标记抬离椭球的高度（米），防止沉入地形 |
| enabled | 启动时是否自动开启 |

**内置默认数据源（全部免 key）**：

| type | API | 内容 |
|------|-----|------|
| gdacs | `gdacs.org/gdacsapi/api/events/geteventlist/MAP` | UN 全球灾害预警（取 Point_Centroid，红/橙级） |
| usgs | `earthquake.usgs.gov/.../2.5_day.geojson` | 近 24 小时 M2.5+ 地震（震级→大小/颜色） |
| eonet | `eonet.gsfc.nasa.gov/api/v3/events` | NASA 活跃自然事件 |

### ai 段（AI 助手，多供应商）

```jsonc
"ai": {
  "enabled": true,          // false = 彻底关闭(不显示聊天面板)
  "active": "deepseek",     // 启用哪家;空 = providers 里的第一个
  "max_rounds": 30,         // 单次指令的工具调用轮次上限
  "system_prompt": "",      // 空 = 内置默认提示词
  "providers": {
    "deepseek": {
      "type": "openai_compatible",       // 协议类型,见下表
      "base_url": "https://api.deepseek.com/v1",
      "model": "deepseek-chat",
      "key_env": "EARTH_DEEPSEEK_KEY",   // 环境变量名(优先)
      "key_config": "deepseek",          // api_keys 段的键名
      "timeout_seconds": 60
    },
    "gemini": { "type": "gemini", "model": "gemini-2.5-flash", ... }
  }
}
```

| type | 协议 | 适用供应商 |
|------|------|-----------|
| openai_compatible | POST `{base_url}/chat/completions`，Bearer 鉴权，tools 函数调用 | DeepSeek、通义千问(DashScope 兼容模式)、智谱 GLM、Kimi、OpenAI 等——**换供应商只需改 base_url + model** |
| gemini | Google `generateContent` v1beta，key 拼 URL | Google Gemini |
| fake | 离线脚本回放（`script` 指向 JSON 数组，每项 `{"text":...}` / `{"calls":[{name,args}]}` / `{"error":...}` 按序出队） | 无网络/无 key 的自动化测试 |

凭证解析与数据源同规则：环境变量（`key_env`）优先，回退 `api_keys[key_config]`；
都没有时该供应商禁用（控制台告警一次），聊天面板显示"未启用"，不影响其它功能。

**接一个新供应商的步骤**（以通义千问为例，零代码）：

```jsonc
"providers": {
  "qwen": {
    "type": "openai_compatible",
    "base_url": "https://dashscope.aliyuncs.com/compatible-mode/v1",
    "model": "qwen-plus",
    "key_env": "EARTH_QWEN_KEY", "key_config": "qwen"
  }
},
"active": "qwen"
```

## 4. AI 助手（聊天面板 + 函数调用代理循环）

屏幕底部的"AI 助手"悬浮条：紧凑输入框 + 发送按钮，历史按需展开（默认折叠，
地图优先）。会话中：你=白、AI=绿、`[工具]` 调用记录=灰、错误=红。

**架构与线程模型**：

```
UI(draw 线程)                          主线程(FRAME drain)              worker 线程
─────────────                          ───────────────────              ──────────
submit(text) ──────────────┐
                           ▼
             AIChatCore: 记录 transcript/history
                           │ startWorkerRound ────────────→ provider->chat(历史+工具声明)
                           │                                   (HTTP POST,60s 超时,重试 3 次)
                           │ ◄──────── pending 结果(文本/错误/工具调用) ────────
                           ▼
             drainMainThread:
               - 文本/错误 → transcript,busy 清零,本轮结束
               - 工具调用 → 主线程执行(fly_to/set_layer/...)   ← 工具碰场景图,必须主线程
                          → 结果记入 history → 起下一轮 worker
```

要点：

- **函数调用配对**：`MODEL_CALL` 与 `TOOL_RESPONSE` 的 id 严格配对（OpenAI 的
  `tool_call_id` / Gemini 的 `functionCall.id`）；模型没给 id 时合成 `call_N`。
  历史裁剪的切点只落在用户消息边界，避免切在配对中间（两家 API 都会 400）。
- **轮次上限**：超过 `max_rounds` 后不再执行工具，补配对错误响应并强制一轮
  "无工具"请求，让模型基于已有数据收尾（不会话中断）。
- **工作区上下文**：每条用户指令前自动注入 `EARTH_CONTEXT_V1` 快照
  （相机经纬高 + 图层开关 + 数据源状态），供"当前位置/屏幕上"这类指代问题；
  快照过大会被丢弃并标注 `context_oversize`。
- **信任边界**：工具返回的文本/URL/metadata 视为不可信证据而非指令
  （防提示词注入），已写进默认系统提示词。

**内置工具**（`registerEarthAITools` 注册，`set_layer` 的描述会动态列出当前图层 id）：

| 工具 | 作用 |
|------|------|
| fly_to(lat, lon, alt_km) | 相机飞到指定经纬度/高度（默认 150km） |
| set_layer(id, enabled) | 开/关数据图层（与 UI 勾选同一条队列） |
| get_view_state() | 当前相机经纬高 + 全部图层开关状态 |
| get_data_summary(source?, sample?) | 数据源状态汇总；指定未开启的源会**自动懒开启**并提示加载中 |


## 5. UI 界面

| 界面元素 | 位置 | 功能 |
|----------|------|------|
| 图层 / Layers 面板 | 左上 | 分组列出数据源图层：图标（marker_shape）、名称、署名、健康状态（勾选=启用；0/1/2/3 = 未抓取/正常/失败/空）。启用开关经 LayerRegistry 排队，主线程下一帧生效 |
| 要素详情卡 | 点击场景标记弹出 | 标题、多行正文（时间/震级/级别等）、"打开"按钮（系统浏览器打开 `DataPoint.url`） |
| 环境 / 视角面板 | 左下 | 太阳方位角（也在主视图用 `,` `.` 调节）、海洋开关、视角归位 |
| AI 助手聊天条 | 底部居中 | AI 对话入口（见第 4 节）；AI 禁用时显示灰字提示 |
| 加载覆盖层 | 居中 | 地球根瓦片后台加载期间的提示 |

状态跨线程约定：UI（ImGui draw 线程）只写 `EnvironmentState` 的 atomic、
LayerRegistry 的待办队列与 `AIChatCore::submit`（内部加锁）；所有 GL/场景图
操作都在主线程（`EnvironmentUpdater`、`drainPending()`、`drainMainThread()`）执行。

## 6. 扩展一个新数据源（零侵入）

1. 新建 `my_source.cpp`，继承 `earthds::HttpJsonSource`，只实现
   `parsePoints(const picojson::value& root, std::vector<DataPoint>&)`；
2. 在文件里写一个注册函数：`DataSourceFactory::instance().registerType("my", creator)`，
   并在 `registerBuiltinSourceTypes()`（feed_sources.cpp）中调用；
3. 配置文件 `data_sources` 增加 `{"type": "my", "url": "...", ...}` 条目；
4. 把新文件加进 `CMakeLists.txt` 的源码列表。

不需要改动框架与 main 的任何代码。

## 7. 命令行参数

| 参数 | 说明 |
|------|------|
| `--config <file>` | 指定配置文件 |
| `--screen <n>` | 显示器编号 |
| `--resolution <w> <h>` | 窗口分辨率（默认 1920x1080） |
| `--folder <dir>` | 模型目录（默认 BASE_DIR/models） |
| `--no-wait` / `--thrown` | 城市瓦片不等待 / 漫游器允许惯性 |
| `--shot <file>` | （验证用）启动后在帧缓冲上抓一帧存 PNG，不依赖屏幕截图 |
| `--auto-exit` | （验证用）抓帧后自动退出，供脚本化冒烟测试 |
| `--ai-submit <text>` | （验证用）第 90 帧自动提交一条 AI 消息（配合 fake 供应商做无网络 e2e） |
| `--ai-transcript <file>` | （验证用）AI 回合结束（提交后 360 帧）把会话记录写盘并退出 |

## 8. 验证与常见问题

**无头/远程环境冒烟测试**（屏幕截图在断开会话中不可靠，直接读帧缓冲）：

```powershell
osgVerse_EarthExplorer.exe --shot D:/temp/shot.png --auto-exit
```

控制台预期输出：`[EarthConfig] 已加载配置 ...` → `[RealizeOperation] OpenGL ...` →
（网络不可达时）`Failed getting ...` / `No imagery for tile ...` →
`地球瓦片源不可用,使用兜底球体`。PNG 中应能看到星空背景、球体和图层面板。

- **窗口出现慢**：首次运行且图源不可达时需等瓦片请求超时（每次连接 8s 上限，
  根瓦片 4 个请求约 40s）；第二次起走 `earth_cache` 缓存。窗口本身在 2 秒内出现。
- **控制台中文乱码**：OSG 通知以 UTF-8 输出，GBK 控制台显示乱码不影响功能。
- **图层面板中文乱码**：未找到字体文件（检查 `ui.font` 配置与 `sdk/misc/` 下的字体）。
- **某个数据源不亮**：图层面板 0/1/2/3 状态 + 控制台告警；缺 key 的源会提示，
  环境变量或 `api_keys` 配好后下次刷新生效。
- **AI 面板显示"未启用"**：控制台搜 `[EarthConfig]`（缺 key）或 `[EarthAI]`
  （供应商不可用）告警；配好 `EARTH_DEEPSEEK_KEY` / `api_keys.deepseek` 后重启生效。
- **AI 回复"网络连接失败"**：供应商 API 不可达（重试 3 次仍无响应）；
  检查网络与 `base_url`。
- **AI e2e 自动化测试**：用 `type: "fake"` 供应商 + `--ai-submit`/`--ai-transcript`
  可在完全离线的环境验证整条代理循环（工具调用→主线程执行→最终文本）。
