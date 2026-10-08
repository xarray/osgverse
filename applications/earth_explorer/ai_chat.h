#ifndef EARTH_AI_CHAT_H
#define EARTH_AI_CHAT_H
// AI assistant core framework (ported from a friend's ai_chat.h/.cpp and refactored for multiple providers):
// - Every capability is a Tool: ToolRegistry registers them centrally, and function calls are
//   drained and executed on the main thread (safe to touch the scene graph);
// - LLMProvider abstraction + three implementations:
//     OpenAICompatibleProvider —— /chat/completions protocol; DeepSeek/Tongyi/GLM/Kimi
//                                 and other domestic/overseas models only need base_url + model swapped;
//     GeminiProvider           —— Google generateContent protocol;
//     FakeProvider             —— offline script playback; verifies the whole agent loop without network/key;
// - AIChatCore: the agent loop state machine. Main thread submit → worker thread calls provider →
//   main thread drainMainThread executes tools → start worker again... until the model finishes with plain text.
// This header depends only on std + picojson and does not include OSG, so it can be tested standalone.
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <picojson.h>

namespace earthai
{
    // ---------------- Tool registry ----------------
    struct Tool
    {
        std::string name, description;
        std::string parametersJson;   // JSON schema (object) string, embedded directly into each protocol's declaration section
        // Executed on the main thread (guaranteed by AIChatCore::drainMainThread); the return value is sent back to the model
        std::function<picojson::value(const picojson::value& args)> execute;
    };

    class ToolRegistry
    {
    public:
        void add(const Tool& t) { _tools.push_back(t); }
        const std::vector<Tool>& tools() const { return _tools; }

        // Gemini functionDeclarations array JSON (empty list → "[]")
        std::string buildGeminiDeclarationsJson() const;
        // OpenAI-compatible tools array JSON ([{"type":"function","function":{...}}])
        std::string buildOpenAIToolsJson() const;

        bool dispatch(const std::string& name, const picojson::value& args, picojson::value& result);

    private:
        bool parseParameters(const Tool& t, picojson::value& out) const;

        std::vector<Tool> _tools;   // registered/enumerated/dispatched only on the main thread, no locking needed
    };

    // ---------------- Neutral conversation history (each Provider serializes it itself) ----------------
    struct FunctionCall
    {
        std::string name;
        picojson::value args;
        std::string id;             // used to pair OpenAI tool_call_id / Gemini functionCall.id
    };
    struct LLMTurn { std::string text; std::vector<FunctionCall> calls; std::string error; };

    struct HistoryItem
    {
        enum Role { USER_TEXT, MODEL_TEXT, MODEL_CALL, TOOL_RESPONSE } role;
        std::string text;              // for USER_TEXT / MODEL_TEXT
        std::string callName;          // for MODEL_CALL / TOOL_RESPONSE
        std::string callId;            // strictly pairs calls with responses (both protocols require it)
        picojson::value callArgs;      // for MODEL_CALL
        picojson::value toolResponse;  // for TOOL_RESPONSE
        HistoryItem() : role(USER_TEXT) {}
    };

    // Provider input: full history + tool declarations (one copy per protocol, precomputed by
    // AIChatCore on the main thread — the worker thread never touches ToolRegistry)
    // + whether tools are carried + the system prompt
    struct ChatRequest
    {
        std::vector<HistoryItem> history;
        bool withTools = true;
        std::string systemPrompt;
        std::string openAIToolsJson;        // OpenAI-compatible tools array (empty when withTools=false)
        std::string geminiDeclarationsJson; // Gemini functionDeclarations array (same as above)
    };

    class LLMProvider
    {
    public:
        virtual ~LLMProvider() {}
        // Synchronous blocking call, executed by AIChatCore's worker thread, never called on the main thread
        virtual LLMTurn chat(const ChatRequest& req) = 0;
        // Display name for logs/UI (e.g. "deepseek-chat@api.deepseek.com"); must not contain the API Key
        virtual std::string describe() const = 0;
    };

    // ---------------- Provider configuration (converted from EarthAppConfig) ----------------
    struct AIProviderSetup
    {
        std::string name;             // provider identifier from config, e.g. "deepseek"
        std::string type;             // openai_compatible / gemini / fake
        std::string baseUrl;          // for openai_compatible (e.g. https://api.deepseek.com/v1)
        std::string model;            // model name (e.g. deepseek-chat / gemini-2.5-flash)
        std::string apiKey;           // resolved credential (may be empty = this Provider is unavailable)
        std::string scriptFile;       // for fake: playback script JSON file
        int timeoutSeconds = 60;
    };

    // Instantiate a Provider from config; returns NULL when the type is unknown or the fake script is missing (caller disables AI and warns)
    LLMProvider* createLLMProvider(const AIProviderSetup& p);

    // OpenAI-compatible /chat/completions (DeepSeek / Qwen / GLM / Kimi / OpenAI are isomorphic)
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

    // Offline playback: array from EARTH_AI_FAKE=<json>, dequeued in order, each item {"text":...}/{"calls":[...]}/{"error":...}
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

    // ---------------- Chat transcript (read by UI) ----------------
    struct ChatEntry { enum Kind { USER, ASSISTANT, TOOL_NOTE, ERR } kind; std::string text; };

    // ---------------- Agent loop state machine ----------------
    class AIChatCore
    {
    public:
        AIChatCore(LLMProvider* p, ToolRegistry* reg);  // does not own provider/registry
        ~AIChatCore();  // join any remaining worker to avoid a crash on exit

        void setMaxRounds(int n) { _maxRounds = (n > 0 ? n : 30); }
        void setSystemPrompt(const std::string& p) { _systemPrompt = p; }

        // Called by UI (draw thread): submit a user message; if busy, ignore and notify
        void submit(const std::string& userText);
        bool busy() const;

        // Called every frame on the main thread: execute queued tool calls (tools must run on the main thread), advance the agent loop
        void drainMainThread();

        // Read by UI (locked snapshot)
        std::vector<ChatEntry> transcript() const;

        // External subsystems present errors into the chat transcript (thread-safe)
        void addErrorNote(const std::string& text);

        // After each successfully accepted user instruction, before starting the worker, the main
        // thread reads the earth workspace snapshot (JSON) once.
        // The snapshot is sent to the model together with the raw user text (with a trust boundary),
        // while the transcript still shows only the raw user text.
        void setContextProvider(const std::function<std::string()>& provider);

        // Test-only: export the serialized Gemini contents JSON of the current history to verify call/response pairing
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
        int _callIdCounter = 0;                  // synthesize "call_N" when the model gives no id
        bool _forceNoTools = false;              // force the next round to carry no tools after hitting the round limit
        std::function<std::string()> _contextProvider;

        // worker output, consumed by the main thread drain
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
