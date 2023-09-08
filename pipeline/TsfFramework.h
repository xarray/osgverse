#ifndef MANA_PP_TSF_FRAMEWORK_HPP
#define MANA_PP_TSF_FRAMEWORK_HPP

// Copied and modified from Unreal5 engine source code
// Source/Runtime/ApplicationCore/Private/Windows/WindowsTextInputMethodSystem
#include <windows.h>
#include <msctf.h>
#include <osg/Referenced>
#include <osg/Vec2>
#include <osg/ref_ptr>
#include <vector>
#include <map>

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

    class TextStoreACP;

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
        HWND GetWindow() const { return _window; }

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

    protected:
        HWND _window;
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
        TextInputMethodChangeNotifier(TextStoreACP* textStore);

        virtual void NotifyLayoutChanged(const LayoutChangeType ChangeType);
        virtual void NotifySelectionChanged();
        virtual void NotifyTextChanged(uint32 beginIndex, uint32 oldLength, uint32 newLength);
        virtual void CancelComposition();

    protected:
        const ComPtr<TextStoreACP> _textStore;
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

        std::map<TextInputMethodContext*, ComPtr<TextStoreACP>> _contextMap;
        osg::ref_ptr<TextInputMethodContext> _activeContext;
        TfClientId _tsfClientId;
        bool _tsfActivated;
    };

    class TextStoreACP : public ITextStoreACP, public ITfContextOwnerCompositionSink
    {
    public:
        TextStoreACP(TextInputMethodContext* context)
            : _tsfDocumentManager(NULL), _textContext(context), _referenceCount(1) {}
        virtual ~TextStoreACP() {}

        // IUnknown Interface Begin
        STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj)
        {
            *ppvObj = nullptr;
            if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITextStoreACP))
            { *ppvObj = static_cast<ITextStoreACP*>(this); }
            else if (IsEqualIID(riid, IID_ITfContextOwnerCompositionSink))
            { *ppvObj = static_cast<ITfContextOwnerCompositionSink*>(this); }

            if (*ppvObj) AddRef();
            return *ppvObj ? S_OK : E_NOINTERFACE;
        }

        STDMETHODIMP_(ULONG) AddRef(void) { return ++_referenceCount; }
        STDMETHODIMP_(ULONG) Release(void)
        {
            const ULONG localReferenceCount = --_referenceCount;
            if (_referenceCount == 0) delete this; return localReferenceCount;
        }
        // IUnknown Interface End

        // ITextStoreACP Interface Begin
        STDMETHODIMP AdviseSink(__RPC__in REFIID riid, __RPC__in_opt IUnknown* punk, DWORD dwMask)
        {
            if (!punk) return E_UNEXPECTED;
            if (!IsEqualIID(riid, IID_ITextStoreACPSink)) return E_INVALIDARG;

            // Install sink object if we have none.
            if (!_adviseSinkObject._textStoreACPSink)
            {
                if (FAILED(_adviseSinkObject._textStoreACPSink.FromQueryInterface(
                            IID_ITextStoreACPSink, punk))) return E_UNEXPECTED;
                if (!_adviseSinkObject._textStoreACPSink) return E_UNEXPECTED;
            }
            else
            {
                ComPtr<IUnknown> ourSinkId(nullptr), theirSinkId(nullptr);
                if (FAILED(ourSinkId.FromQueryInterface(IID_IUnknown, punk))) return E_UNEXPECTED;
                if (FAILED(theirSinkId.FromQueryInterface(IID_IUnknown, punk))) return E_UNEXPECTED;
                if (ourSinkId != theirSinkId) return E_FAIL;
            }

            // Update flags for what notifications we should broadcast back to TSF.
            _adviseSinkObject._sinkFlags = dwMask; return S_OK;
        }

        STDMETHODIMP UnadviseSink(__RPC__in_opt IUnknown* punk)
        {
            if (!punk) return E_INVALIDARG;
            if (!_adviseSinkObject._textStoreACPSink) return E_POINTER;

            ComPtr<IUnknown> ourSinkId(nullptr), theirSinkId(nullptr);
            if (FAILED(ourSinkId.FromQueryInterface(IID_IUnknown, punk))) return E_UNEXPECTED;
            if (FAILED(theirSinkId.FromQueryInterface(IID_IUnknown, punk))) return E_UNEXPECTED;
            if (ourSinkId != theirSinkId) return E_FAIL;
            _adviseSinkObject._textStoreACPSink.Reset(); return S_OK;
        }

        STDMETHODIMP RequestLock(DWORD dwLockFlags, HRESULT* phrSession)
        {
            if (!_adviseSinkObject._textStoreACPSink) return E_FAIL;
            if (!phrSession) return E_INVALIDARG;

            if (_lockManager.lockType == 0)
            {
                _lockManager.lockType = dwLockFlags & (~TS_LF_SYNC);
                *phrSession = _adviseSinkObject._textStoreACPSink->OnLockGranted(_lockManager.lockType);
                while (_lockManager.isPendingLockUpgrade)
                {
                    _lockManager.lockType = TS_LF_READWRITE;
                    _lockManager.isPendingLockUpgrade = false;
                    _adviseSinkObject._textStoreACPSink->OnLockGranted(_lockManager.lockType);
                }

                _lockManager.lockType = 0;
                return S_OK;
            }
            else if (!isFlaggedReadLocked(_lockManager.lockType) &&
                    !!isFlaggedReadWriteLocked(_lockManager.lockType) &&
                     !isFlaggedReadWriteLocked(dwLockFlags) && !(dwLockFlags & TS_LF_SYNC))
            { *phrSession = TS_S_ASYNC; _lockManager.isPendingLockUpgrade = true; }
            else
            { *phrSession = TS_E_SYNCHRONOUS; return E_FAIL; }
            return S_OK;
        }

        STDMETHODIMP GetStatus(__RPC__out TS_STATUS* pdcs)
        {
            if (!pdcs) return E_INVALIDARG;
            pdcs->dwDynamicFlags = _textContext->IsReadOnly() ? TS_SD_READONLY : 0;
            pdcs->dwStaticFlags = TS_SS_NOHIDDENTEXT; return S_OK;
        }

        STDMETHODIMP GetEndACP(__RPC__out LONG* pacp)
        {
            if (!isFlaggedReadLocked(_lockManager.lockType)) return TS_E_NOLOCK;
            *pacp = _textContext->GetTextLength(); return S_OK;
        }

        // Selection Methods
        STDMETHODIMP GetSelection(ULONG ulIndex, ULONG ulCount,
            __RPC__out_ecount_part(ulCount, *pcFetched) TS_SELECTION_ACP* pSelection,
            __RPC__out ULONG* pcFetched)
        {
            if (!isFlaggedReadLocked(_lockManager.lockType)) return TS_E_NOLOCK;
            if (ulIndex != TS_DEFAULT_SELECTION) return TS_E_NOSELECTION;

            ULONG selectionCount = osg::minimum(1ul, ulCount); *pcFetched = 0;
            for (ULONG i = 0; i < selectionCount; ++i)
            {
                uint32 selectionBeginIndex, selectionLength;
                TextInputMethodContext::CaretPosition caretPosition;
                _textContext->GetSelectionRange(selectionBeginIndex, selectionLength, caretPosition);

                pSelection[i].acpStart = selectionBeginIndex;
                pSelection[i].acpEnd = selectionBeginIndex + selectionLength;
                switch (caretPosition)
                {
                case TextInputMethodContext::CaretPosition::Beginning:
                    pSelection[i].style.ase = TS_AE_START; break;
                case TextInputMethodContext::CaretPosition::Ending:
                    pSelection[i].style.ase = TS_AE_END; break;
                }

                pSelection[i].style.fInterimChar = FALSE;
                ++(*pcFetched);
            }
            return S_OK;
        }

        STDMETHODIMP SetSelection(ULONG ulCount,
            __RPC__in_ecount_full(ulCount) const TS_SELECTION_ACP* pSelection)
        {
            if (!isFlaggedReadWriteLocked(_lockManager.lockType)) return TS_E_NOLOCK;
            const LONG stringLength = _textContext->GetTextLength();
            unsigned int selectionCount = osg::minimum(1ul, ulCount);
            for (unsigned int i = 0; i < selectionCount; ++i)
            {
                if (pSelection[i].acpStart < 0 || pSelection[i].acpStart > stringLength ||
                    pSelection[i].acpEnd < 0 || pSelection[i].acpEnd > stringLength)
                { return TF_E_INVALIDPOS; }
            }

            for (unsigned int i = 0; i < selectionCount; ++i)
            {
                uint32 selectionBeginIndex = pSelection[i].acpStart;
                uint32 selectionLength = pSelection[i].acpEnd - pSelection[i].acpStart;
                TextInputMethodContext::CaretPosition caretPosition =
                    TextInputMethodContext::CaretPosition::Ending;
                _textContext->SetSelectionRange(selectionBeginIndex, selectionLength, caretPosition);
            }
            return S_OK;
        }

        // Attributes Methods
        STDMETHODIMP RequestSupportedAttrs(DWORD dwFlags, ULONG cFilterAttrs,
            __RPC__in_ecount_full_opt(cFilterAttrs) const TS_ATTRID* paFilterAttrs)
        {
            for (ULONG i = 0; i < cFilterAttrs; ++i)
            {
                auto predicate = [&](const SupportedAttribute& attribute) -> bool
                { return IsEqualGUID(*attribute.id, paFilterAttrs[i]) == TRUE; };

                std::vector<SupportedAttribute>::iterator itr =
                    std::find_if(_supportedAttributes.begin(), _supportedAttributes.end(), predicate);
                if (itr != _supportedAttributes.end())
                {
                    size_t index = std::distance(_supportedAttributes.begin(), itr);
                    _supportedAttributes[index].wantsDefault = true;
                }
            }
            return S_OK;
        }

        STDMETHODIMP RequestAttrsAtPosition(LONG acpPos, ULONG cFilterAttrs,
            __RPC__in_ecount_full_opt(cFilterAttrs) const TS_ATTRID* paFilterAttrs, DWORD dwFlags)
        {
            for (ULONG i = 0; i < cFilterAttrs; ++i)
            {
                auto predicate = [&](const SupportedAttribute& attribute) -> bool
                { return IsEqualGUID(*attribute.id, paFilterAttrs[i]) == TRUE; };

                std::vector<SupportedAttribute>::iterator itr =
                    std::find_if(_supportedAttributes.begin(), _supportedAttributes.end(), predicate);
                if (itr != _supportedAttributes.end())
                {
                    size_t index = std::distance(_supportedAttributes.begin(), itr);
                    _supportedAttributes[index].wantsDefault = true;
                }
            }
            return S_OK;
        }

        STDMETHODIMP RequestAttrsTransitioningAtPosition(LONG acpPos, ULONG cFilterAttrs,
            __RPC__in_ecount_full_opt(cFilterAttrs) const TS_ATTRID* paFilterAttrs, DWORD dwFlags)
        { return E_NOTIMPL; }
        STDMETHODIMP FindNextAttrTransition(LONG acpStart, LONG acpHalt, ULONG cFilterAttrs,
            __RPC__in_ecount_full_opt(cFilterAttrs) const TS_ATTRID* paFilterAttrs,
            DWORD dwFlags, __RPC__out LONG* pacpNext, __RPC__out BOOL* pfFound,
            __RPC__out LONG* plFoundOffset) { return E_NOTIMPL; }

        STDMETHODIMP RetrieveRequestedAttrs(ULONG ulCount,
            __RPC__out_ecount_part(ulCount, *pcFetched) TS_ATTRVAL* paAttrVals,
            __RPC__out ULONG* pcFetched)
        {
            *pcFetched = 0;
            for (int32 i = 0; i < _supportedAttributes.size() && *pcFetched < ulCount; ++i)
            {
                if (_supportedAttributes[i].wantsDefault)
                {
                    paAttrVals[i].idAttr = *(_supportedAttributes[i].id);
                    VariantCopy(&(paAttrVals[*pcFetched].varValue),
                                &(_supportedAttributes[i].defaultValue));
                    paAttrVals[i].dwOverlapId = 0;
                }
                ++(*pcFetched);
            }
            return S_OK;
        }

        // View Methods
        STDMETHODIMP GetActiveView(__RPC__out TsViewCookie* pvcView)
        { *pvcView = 0; return S_OK; }

        STDMETHODIMP GetACPFromPoint(TsViewCookie vcView, __RPC__in const POINT* pt,
                                     DWORD dwFlags, __RPC__out LONG* pacp)
        {
            if (!isFlaggedReadLocked(_lockManager.lockType)) return TS_E_NOLOCK;
            const osg::Vec2 point(pt->x, pt->y);
            *pacp = _textContext->GetCharacterIndexFromPoint(point); return S_OK;
        }

        STDMETHODIMP GetTextExt(TsViewCookie vcView, LONG acpStart, LONG acpEnd,
                                __RPC__out RECT* prc, __RPC__out BOOL* pfClipped)
        {
            if (!isFlaggedReadLocked(_lockManager.lockType)) return TS_E_NOLOCK;
            if (acpStart == acpEnd) return E_INVALIDARG;

            const LONG stringLength = _textContext->GetTextLength();
            if (acpStart < 0 || acpStart > stringLength ||
                (acpEnd != -1 && (acpEnd < 0 || acpEnd > stringLength)))
            { return TS_E_INVALIDPOS;  }

            osg::Vec2 position, size;
            *pfClipped = _textContext->GetTextBounds(acpStart, acpEnd - acpStart, position, size);
            prc->left = (LONG)position.x(); prc->top = (LONG)position.y();
            prc->right = (LONG)(position.x() + size.x());
            prc->bottom = (LONG)(position.y() + size.y());
            return S_OK;
        }

        STDMETHODIMP GetScreenExt(TsViewCookie vcView, __RPC__out RECT* prc)
        {
            osg::Vec2 position, size;
            if (vcView != 0) return E_INVALIDARG;
            _textContext->GetScreenBounds(position, size);
            prc->left = (LONG)position.x(); prc->top = (LONG)position.y();
            prc->right = (LONG)(position.x() + size.x());
            prc->bottom = (LONG)(position.y() + size.y());
            return S_OK;
        }

        STDMETHODIMP GetWnd(TsViewCookie vcView, __RPC__deref_out_opt HWND* phwnd)
        { *phwnd = _textContext->GetWindow(); return S_OK; }

        // Plain Character Methods
        STDMETHODIMP GetText(LONG acpStart, LONG acpEnd,
            __RPC__out_ecount_part(cchPlainReq, *pcchPlainOut) WCHAR* pchPlain,
            ULONG cchPlainReq, __RPC__out ULONG* pcchPlainOut,
            __RPC__out_ecount_part(ulRunInfoReq, *pulRunInfoOut) TS_RUNINFO* prgRunInfo,
            ULONG ulRunInfoReq, __RPC__out ULONG* pulRunInfoOut, __RPC__out LONG* pacpNext)
        {
            if (!isFlaggedReadLocked(_lockManager.lockType)) return TS_E_NOLOCK;
            const LONG stringLength = _textContext->GetTextLength();

            // Validate range.
            if (acpStart < 0 || acpStart > stringLength ||
                (acpEnd != -1 && (acpEnd < 0 || acpEnd > stringLength))) return TF_E_INVALIDPOS;
            const uint32 beginIndex = acpStart;
            const uint32 length = (acpEnd == -1 ? stringLength : acpEnd) - beginIndex;
            *pcchPlainOut = 0; // No characters yet copied to buffer.

            // Write characters to buffer only if there is a buffer with allocated size.
            if (pchPlain && cchPlainReq > 0)
            {
                std::string stringInRange;
                _textContext->GetTextInRange(beginIndex, length, stringInRange);
                for (uint32 i = 0; i < length && *pcchPlainOut < cchPlainReq; ++i)
                {
                    pchPlain[i] = stringInRange[i];
                    ++(*pcchPlainOut);
                }
            }

            *pulRunInfoOut = 0; // No runs yet copied to buffer.
            if (prgRunInfo && ulRunInfoReq > 0)
            {
                prgRunInfo[0].uCount = osg::minimum(static_cast<uint32>(ulRunInfoReq), length);
                prgRunInfo[0].type = TS_RT_PLAIN; ++(*pulRunInfoOut);
            }
            *pacpNext = beginIndex + length;
            return S_OK;
        }

        STDMETHODIMP QueryInsert(LONG acpInsertStart, LONG acpInsertEnd, ULONG cch,
            __RPC__out LONG* pacpResultStart, __RPC__out LONG* pacpResultEnd);
        STDMETHODIMP InsertTextAtSelection(DWORD dwFlags, __RPC__in_ecount_full(cch) const WCHAR* pchText,
            ULONG cch, __RPC__out LONG* pacpStart, __RPC__out LONG* pacpEnd,
            __RPC__out TS_TEXTCHANGE* pChange);
        STDMETHODIMP SetText(DWORD dwFlags, LONG acpStart, LONG acpEnd,
            __RPC__in_ecount_full(cch) const WCHAR* pchText, ULONG cch,
            __RPC__out TS_TEXTCHANGE* pChange);

        // Embedded Character Methods
        STDMETHODIMP GetEmbedded(LONG acpPos, __RPC__in REFGUID rguidService,
            __RPC__in REFIID riid, __RPC__deref_out_opt IUnknown** ppunk);
        STDMETHODIMP GetFormattedText(LONG acpStart, LONG acpEnd,
            __RPC__deref_out_opt IDataObject** ppDataObject);
        STDMETHODIMP QueryInsertEmbedded(__RPC__in const GUID* pguidService,
            __RPC__in const FORMATETC* pFormatEtc, __RPC__out BOOL* pfInsertable);
        STDMETHODIMP InsertEmbedded(DWORD dwFlags, LONG acpStart, LONG acpEnd,
            __RPC__in_opt IDataObject* pDataObject, __RPC__out TS_TEXTCHANGE* pChange);
        STDMETHODIMP InsertEmbeddedAtSelection(DWORD dwFlags, __RPC__in_opt IDataObject* pDataObject,
            __RPC__out LONG* pacpStart, __RPC__out LONG* pacpEnd, __RPC__out TS_TEXTCHANGE* pChange);
        // ITextStoreACP Interface End

        // ITfContextOwnerCompositionSink Interface Begin
        STDMETHODIMP OnStartComposition(__RPC__in_opt ITfCompositionView* pComposition,
                                        __RPC__out BOOL* pfOk)
        {
            if (!_composition._tsfCompositionView)
            {
                _composition._tsfCompositionView = pComposition;
                if (!_textContext->IsComposing()) _textContext->BeginComposition(); *pfOk = TRUE;
            }
            else *pfOk = FALSE;
            return S_OK;
        }

        STDMETHODIMP OnUpdateComposition(__RPC__in_opt ITfCompositionView* pComposition,
                                         __RPC__in_opt ITfRange* pRangeNew)
        {
            if (!_composition._tsfCompositionView) return E_UNEXPECTED;
            if (pComposition != _composition._tsfCompositionView) return E_UNEXPECTED;

            if (pRangeNew)
            {
                ComPtr<ITfRangeACP> rangeACP;
                rangeACP.FromQueryInterface(IID_ITfRangeACP, pRangeNew);

                LONG beginIndex = 0, length = 0;
                const auto result = rangeACP->GetExtent(&(beginIndex), &(length));
                if (FAILED(result)) return E_FAIL;
                else _textContext->UpdateCompositionRange(beginIndex, length);
            }
            return S_OK;
        }

        STDMETHODIMP OnEndComposition(__RPC__in_opt ITfCompositionView* pComposition)
        {
            if (!_composition._tsfCompositionView) return E_UNEXPECTED;
            if (pComposition != _composition._tsfCompositionView) return E_UNEXPECTED;
            _composition._tsfCompositionView.Reset();
            _textContext->EndComposition(); return S_OK;
        }
        // ITfContextOwnerCompositionSink Interface End

        struct AdviseSinkObject
        {
            AdviseSinkObject() :_textStoreACPSink(nullptr), _sinkFlags(0) {}
            ComPtr<ITextStoreACPSink> _textStoreACPSink;
            ComPtr<ITextStoreACPServices> _textStoreACPServices;
            DWORD _sinkFlags;
        } _adviseSinkObject;

        struct Composition
        {
            Composition() : _tsfCompositionView(nullptr) {}
            ComPtr<ITfCompositionView> _tsfCompositionView;
        } _composition;

        ComPtr<ITfDocumentMgr> _tsfDocumentManager;
        ComPtr<ITfContext> _tsfContext;
        ComPtr<ITfContextOwnerCompositionServices> _tsfContextOwnerCompositionServices;
        TfEditCookie _tsfEditCookie;

    protected:
        struct SupportedAttribute
        {
            SupportedAttribute(const TS_ATTRID* const inId) : wantsDefault(false), id(inId)
            { VariantInit(&(defaultValue)); }

            bool wantsDefault;
            const TS_ATTRID* const id;
            VARIANT defaultValue;
        };

        struct LockManager
        {
            LockManager() : lockType(0), isPendingLockUpgrade(false) {}
            DWORD lockType; bool isPendingLockUpgrade;
        } _lockManager;

        static bool isFlaggedReadLocked(const DWORD flags)
        { return (flags & TS_LF_READ) == TS_LF_READ; }

        static bool isFlaggedReadWriteLocked(const DWORD flags)
        { return (flags & TS_LF_READWRITE) == TS_LF_READWRITE; }

        std::vector<SupportedAttribute> _supportedAttributes;
        osg::ref_ptr<TextInputMethodContext> _textContext;
        ULONG _referenceCount;
    };

}

#endif
