#ifndef EARTH_AI_CHAT_H
#define EARTH_AI_CHAT_H
// AI 助手核心框架(移植自朋友版 ai_chat.h/.cpp 并重构为多供应商):
// - 一切能力皆 Tool:ToolRegistry 集中注册,函数调用在主线程 drain 执行(碰场景图安全);
// - LLMProvider 抽象 + 三种实现:
//     OpenAICompatibleProvider —— /chat/completions 协议,DeepSeek/通义/GLM/Kimi
//                                 等国内外模型只需换 base_url + model;
//     GeminiProvider           —— Google generateContent 协议;
//     FakeProvider             —— 离线脚本回放,无网络/无 key 也可验证整条代理循环;
// - AIChatCore:代理循环状态机。主线程 submit → worker 线程调 provider →
//   主线程 drainMainThread 执行工具 → 再起 worker……直到模型给出纯文本收尾。
// 本头文件只依赖 std + picojson,不含 OSG,便于独立测试。
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <picojson.h>

namespace earthai
{
    // ---------------- 工具注册表 ----------------
    struct Tool
    {
        std::string name, description;
        std::string parametersJson;   // JSON schema(object)字符串,直接嵌入各协议的声明段
        // 主线程执行(由 AIChatCore 的 drainMainThread 保证);返回值回传给模型
        std::function<picojson::value(const picojson::value& args)> execute;
    };

    class ToolRegistry
    {
    public:
        void add(const Tool& t) { _tools.push_back(t); }
        const std::vector<Tool>& tools() const { return _tools; }

        // Gemini functionDeclarations 数组 JSON(空表 → "[]")
        std::string buildGeminiDeclarationsJson() const;
        // OpenAI 兼容 tools 数组 JSON([{"type":"function","function":{...}}])
        std::string buildOpenAIToolsJson() const;

        bool dispatch(const std::string& name, const picojson::value& args, picojson::value& result);

    private:
        bool parseParameters(const Tool& t, picojson::value& out) const;

        std::vector<Tool> _tools;   // 只在主线程注册/枚举/dispatch,无需加锁
    };

    // ---------------- 中立会话历史(各 Provider 自行序列化) ----------------
    struct FunctionCall
    {
        std::string name;
        picojson::value args;
        std::string id;             // OpenAI tool_call_id / Gemini functionCall.id 配对用
    };
    struct LLMTurn { std::string text; std::vector<FunctionCall> calls; std::string error; };

    struct HistoryItem
    {
        enum Role { USER_TEXT, MODEL_TEXT, MODEL_CALL, TOOL_RESPONSE } role;
        std::string text;              // USER_TEXT / MODEL_TEXT 用
        std::string callName;          // MODEL_CALL / TOOL_RESPONSE 用
        std::string callId;            // 调用与响应严格配对(两家协议都要求)
        picojson::value callArgs;      // MODEL_CALL 用
        picojson::value toolResponse;  // TOOL_RESPONSE 用
        HistoryItem() : role(USER_TEXT) {}
    };

    // Provider 的输入:完整历史 + 工具声明(两种协议各一份,由 AIChatCore 在主线程
    // 预先算好——worker 线程绝不碰 ToolRegistry)+ 是否携带工具 + 系统提示词
    struct ChatRequest
    {
        std::vector<HistoryItem> history;
        bool withTools = true;
        std::string systemPrompt;
        std::string openAIToolsJson;        // OpenAI 兼容 tools 数组(withTools=false 时为空)
        std::string geminiDeclarationsJson; // Gemini functionDeclarations 数组(同上)
    };

    class LLMProvider
    {
    public:
        virtual ~LLMProvider() {}
        // 同步阻塞调用,由 AIChatCore 的工作线程执行,绝不在主线程调
        virtual LLMTurn chat(const ChatRequest& req) = 0;
        // 日志/UI 显示名(如 "deepseek-chat@api.deepseek.com"),不得包含 API Key
        virtual std::string describe() const = 0;
    };

    // ---------------- Provider 配置(由 EarthAppConfig 转换而来) ----------------
    struct AIProviderSetup
    {
        std::string name;             // 配置里的供应商标识,如 "deepseek"
        std::string type;             // openai_compatible / gemini / fake
        std::string baseUrl;          // openai_compatible 用(如 https://api.deepseek.com/v1)
        std::string model;            // 模型名(如 deepseek-chat / gemini-2.5-flash)
        std::string apiKey;           // 已解析的凭证(可为空 = 该 Provider 不可用)
        std::string scriptFile;       // fake 用:回放脚本 JSON 文件
        int timeoutSeconds = 60;
    };

    // 按配置实例化 Provider;type 未知或 fake 脚本缺失时返回 NULL(调用方禁用 AI 并告警)
    LLMProvider* createLLMProvider(const AIProviderSetup& p);

    // OpenAI 兼容 /chat/completions(DeepSeek / Qwen / GLM / Kimi / OpenAI 同构)
    class OpenAICompatibleProvider : public LLMProvider
    {
    public:
        OpenAICompatibleProvider(const AIProviderSetup& s) : _setup(s) {}
        virtual LLMTurn chat(const ChatRequest& req);
        virtual std::string describe() const;
    private:
        AIProviderSetup _setup;
    };

    // Google Gemini generateContent(v1beta REST)
    class GeminiProvider : public LLMProvider
    {
    public:
        GeminiProvider(const AIProviderSetup& s) : _setup(s) {}
        virtual LLMTurn chat(const ChatRequest& req);
        virtual std::string describe() const;
    private:
        AIProviderSetup _setup;
    };

    // 离线回放:EARTH_AI_FAKE=<json> 的数组,按序出队,每项 {"text":...}/{"calls":[...]}/{"error":...}
    class FakeProvider : public LLMProvider
    {
    public:
        bool loadFromFile(const std::string& path);
        bool loadFromString(const std::string& json);
        virtual LLMTurn chat(const ChatRequest& req);
        virtual std::string describe() const;
    private:
        std::vector<LLMTurn> _script; size_t _cursor = 0; std::mutex _mutex;
    };

    // ---------------- 会话记录(UI 读) ----------------
    struct ChatEntry { enum Kind { USER, ASSISTANT, TOOL_NOTE, ERR } kind; std::string text; };

    // ---------------- 代理循环状态机 ----------------
    class AIChatCore
    {
    public:
        AIChatCore(LLMProvider* p, ToolRegistry* reg);  // 不持有 provider/registry 所有权
        ~AIChatCore();  // join 残留 worker,避免退出崩溃

        void setMaxRounds(int n) { _maxRounds = (n > 0 ? n : 30); }
        void setSystemPrompt(const std::string& p) { _systemPrompt = p; }

        // UI(draw 线程)调用:提交一条用户消息;busy 则忽略并提示
        void submit(const std::string& userText);
        bool busy() const;

        // 主线程每帧调:执行排队的工具调用(工具必须主线程)、推进代理循环
        void drainMainThread();

        // UI 读(加锁快照)
        std::vector<ChatEntry> transcript() const;

        // 外部子系统把错误呈现到聊天记录(线程安全)
        void addErrorNote(const std::string& text);

        // 每次成功接受用户指令后、启动 worker 前,主线程读一次地球工作区快照(JSON)。
        // 快照会与用户原文一起发给模型(带信任边界),transcript 仍只显示用户原文。
        void setContextProvider(const std::function<std::string()>& provider);

        // 仅测试用:导出当前历史序列化后的 Gemini contents JSON,校验 call/response 配对
        std::string historyContentsForTest() const;

    private:
        void startWorkerRound();
        void joinWorkerIfAny();
        std::string buildContentsLocked() const;

        LLMProvider* _provider;
        ToolRegistry* _registry;
        int _maxRounds = 30;
        std::string _systemPrompt;

        mutable std::mutex _mutex;
        std::vector<ChatEntry> _transcript;
        std::vector<HistoryItem> _history;
        bool _busy = false;
        int _round = 0;
        int _callIdCounter = 0;                  // 模型没给 id 时合成 "call_N"
        bool _forceNoTools = false;              // 撞轮次上限后强制下一轮不带工具
        std::function<std::string()> _contextProvider;

        // worker 产出,主线程 drain 消费
        std::vector<ChatEntry> _pendingEntries;
        std::vector<FunctionCall> _pendingCalls;
        bool _hasPendingResult = false;
        std::string _pendingAssistantText;
        bool _hasPendingAssistantText = false;

        std::thread _worker;
        bool _workerJoinable = false;
    };
}
#endif
