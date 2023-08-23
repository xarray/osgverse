#ifndef MANA_PP_TSF_FRAMEWORK_HPP
#define MANA_PP_TSF_FRAMEWORK_HPP

// Copied and modified from Unreal5 engine source code
// Source/Runtime/ApplicationCore/Private/Windows/WindowsTextInputMethodSystem
#include <windows.h>
#include <msctf.h>
#include <osg/Referenced>
#include <osg/Vec2>
#include <osg/ref_ptr>

#ifndef uint32
typedef unsigned int uint32;
typedef int int32;
#endif

template<typename T> class ComPtr
{
public:
    typedef T PointerType;
    ComPtr() : ptr(NULL) {}
    ComPtr(PointerType* const p) : ptr(p) { if (ptr) ptr->AddRef(); }
    ComPtr(const ComPtr<PointerType>& o) : ptr(o.ptr) { if (ptr) ptr->AddRef(); }
    ComPtr(ComPtr<PointerType>&& o) : ptr(o.ptr) { o.ptr = NULL; }
    ~ComPtr() { if (ptr) ptr->Release(); }

    ComPtr<PointerType>& operator=(PointerType* const o)
    { if (ptr != o) { if (o) o->AddRef(); if (ptr) ptr->Release(); ptr = o; } return *this; }

    ComPtr<PointerType>& operator=(const ComPtr<PointerType>& o)
    { if (ptr != o.ptr) { if (o.ptr) o->AddRef(); if (ptr) ptr->Release(); ptr = o.ptr; } return *this; }

    ComPtr<PointerType>& operator=(ComPtr<PointerType>&& o)
    { if (ptr != o.ptr) { if (ptr) ptr->Release(); ptr = o.ptr; o.ptr = NULL; } return *this; }

    FORCEINLINE PointerType** operator&() { return &(ptr); }
    FORCEINLINE PointerType* operator->() const { return ptr; }
    FORCEINLINE bool operator==(PointerType* const o) const { return ptr == o; }
    FORCEINLINE bool operator!=(PointerType* const o) const { return ptr != o; }
    FORCEINLINE operator PointerType* () const { return ptr; }

    void Attach(PointerType* Object) { if (ptr) ptr->Release(); ptr = Object; }
    void Detach() { ptr = NULL; }
    void Reset() { if (ptr) { ptr->Release(); ptr = NULL; } }
    FORCEINLINE PointerType* Get() const { return ptr; }
    FORCEINLINE const bool IsValid() const { return (ptr != NULL); }

    HRESULT FromQueryInterface(REFIID riid, IUnknown* unknown)
    {
        if (ptr) { ptr->Release(); ptr = NULL; }
        return unknown->QueryInterface(riid, reinterpret_cast<void**>(&(ptr)));
    }

private:
    PointerType* ptr;
};

namespace osgVerse
{

    /**
     * Editable texts should implement this class and maintain an object of this type after registering it.
     * Methods of this class are called by the system to query contextual information about the state
     * of the editable text.
     * This information is used by the text input method system to provide appropriate processed input.
     * Methods of this class are also called by the system to provide processed text input.
     */
    class TextInputMethodContext : public osg::Referenced
    {
    public:
        enum class CaretPosition { Beginning, Ending };

        virtual bool IsComposing();
        virtual bool IsReadOnly();
        virtual uint32 GetTextLength();

        virtual void GetSelectionRange(uint32& beginIndex, uint32& length, CaretPosition& caretPosition);
        virtual void SetSelectionRange(uint32 beginIndex, uint32 length, CaretPosition caretPosition);

        virtual void GetTextInRange(uint32 beginIndex, uint32 length, std::string& outString);
        virtual void SetTextInRange(uint32 beginIndex, uint32 length, const std::string& string);

        virtual int32 GetCharacterIndexFromPoint(const osg::Vec2& point);
        virtual bool GetTextBounds(uint32 beginIndex, uint32 length, osg::Vec2& outPosition, osg::Vec2& outSize);
        virtual void GetScreenBounds(osg::Vec2& outPosition, osg::Vec2& outSize);

        virtual void BeginComposition();
        virtual void UpdateCompositionRange(int32 beginIndex, uint32 length);
        virtual void EndComposition();
    };

    /**
     * Platform owners implement this class to react to changes in the view/model of editable text widgets.
     * Methods of this class should be called by the user to notify the system of changes not caused by system calls to
     * methods of a ITextInputMethodContext implementation.
     */
    class TextInputMethodChangeNotifier : public osg::Referenced
    {
    public:
        enum class LayoutChangeType { Created, Changed, Destroyed };
        virtual void NotifyLayoutChanged(const LayoutChangeType ChangeType);
        virtual void NotifySelectionChanged();
        virtual void NotifyTextChanged(uint32 beginIndex, uint32 oldLength, uint32 newLength);
        virtual void CancelComposition();
    };

    /**
     * Platform owners implement this class to interface with the platform's input method system.
     */
    class TextInputMethodSystem : public osg::Referenced
    {
    public:
        TextInputMethodSystem() : _tsfClientId(-1), _tsfActivated(false) {}
        virtual bool Initialize() = 0;
        virtual void Terminate() = 0;
        void OnIMEActivationStateChanged(bool isEnabled);

        virtual void ApplyDefaults(HWND window);
        virtual TextInputMethodChangeNotifier* RegisterContext(TextInputMethodContext* context);
        virtual void UnregisterContext(TextInputMethodContext* context);

        virtual void ActivateContext(TextInputMethodContext* context);
        virtual void DeactivateContext(TextInputMethodContext* context);
        virtual bool IsActiveContext(TextInputMethodContext* context) const;

    protected:
        ComPtr<ITfInputProcessorProfiles> _tsfInputProcessorProfiles;
        ComPtr<ITfInputProcessorProfileMgr> _tsfInputProcessorProfileManager;
        ComPtr<ITfThreadMgr> _tsfThreadManager;
        ComPtr<ITfDocumentMgr> _tsfDisabledDocumentManager;

        osg::ref_ptr<TextInputMethodContext> _activeContext;
        TfClientId _tsfClientId;
        bool _tsfActivated;
    };

}

#endif
