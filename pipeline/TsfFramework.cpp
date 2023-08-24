// Copied and modified from Unreal5 engine source code
// Source/Runtime/ApplicationCore/Private/Windows/WindowsTextInputMethodSystem.cpp
#include <osg/Notify>
#include <string>
#include <set>
#include <vector>
#include <iostream>
#include "TsfFramework.h"
using namespace osgVerse;

class TSFActivationProxy
    : public ITfInputProcessorProfileActivationSink
    , public ITfActiveLanguageProfileNotifySink
{
public:
    TSFActivationProxy(TextInputMethodSystem* owner)
        : TSFProfileCookie(TF_INVALID_COOKIE), TSFLanguageCookie(TF_INVALID_COOKIE),
          _owner(owner), _referenceCount(1) {}
    virtual ~TSFActivationProxy() {}

    // IUnknown Interface
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override
    {
        *ppvObj = nullptr;
        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfInputProcessorProfileActivationSink))
        { *ppvObj = static_cast<ITfInputProcessorProfileActivationSink*>(this); }
        else if (IsEqualIID(riid, IID_ITfActiveLanguageProfileNotifySink))
        { *ppvObj = static_cast<ITfActiveLanguageProfileNotifySink*>(this); }

        // Add a reference if we're (conceptually) returning a reference to our self.
        if (*ppvObj) AddRef();
        return *ppvObj ? S_OK : E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() override
    { return ++_referenceCount; }

    STDMETHODIMP_(ULONG) Release() override
    { const ULONG c = --_referenceCount; if (!_referenceCount) delete this; return c; }

    // ITfInputProcessorProfileActivationSink Interface
    STDMETHODIMP OnActivated(DWORD dwProfileType, LANGID langid, __RPC__in REFCLSID clsid,
                             __RPC__in REFGUID catid, __RPC__in REFGUID guidProfile,
                             HKL hkl, DWORD dwFlags) override
    {
        const bool isEnabled = !!(dwFlags & TF_IPSINK_FLAG_ACTIVE);
        _owner->OnIMEActivationStateChanged(isEnabled);
        return S_OK;
    }
    
    // ITfActiveLanguageProfileNotifySink Interface
    STDMETHODIMP OnActivated(REFCLSID clsid, REFGUID guidProfile, BOOL activated) override
    {
        _owner->OnIMEActivationStateChanged(activated == TRUE);
        return S_OK;
    }

    DWORD TSFProfileCookie;
    DWORD TSFLanguageCookie;

private:
    TextInputMethodSystem* _owner;
    ULONG _referenceCount;
};

class TsfTextInputMethodSystem : public TextInputMethodSystem
{
public:
    virtual bool Initialize();
    virtual void Terminate();

protected:
    ComPtr<TSFActivationProxy> _tsfActivationProxy;
};

bool TsfTextInputMethodSystem::Initialize()
{
    // Get input processor profiles
    ITfInputProcessorProfiles* rawPointerTSFInputProcessorProfiles = NULL;
    HRESULT r = ::CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr,
                                   CLSCTX_INPROC_SERVER, IID_ITfInputProcessorProfiles,
                                   reinterpret_cast<void**>(&(rawPointerTSFInputProcessorProfiles)));
    if (FAILED(r))
    {
        OSG_WARN << "[TextInputMethodSystem] Failed creating the TSF input processor profiles.\n";
        return false;
    }
    else
        _tsfInputProcessorProfiles.Attach(rawPointerTSFInputProcessorProfiles);

    // Get input processor profile manager from profiles
    r = _tsfInputProcessorProfileManager.FromQueryInterface(
            IID_ITfInputProcessorProfileMgr, _tsfInputProcessorProfiles);
    if (FAILED(r))
    {
        OSG_WARN << "[TextInputMethodSystem] Failed acquiring TSF input processor profile manager.\n";
        _tsfInputProcessorProfiles.Reset(); return false;
    }

    // Create thread manager
    ITfThreadMgr* rawPointerTSFThreadManager = NULL;
    r = ::CoCreateInstance(CLSID_TF_ThreadMgr, NULL, CLSCTX_INPROC_SERVER, IID_ITfThreadMgr,
                           reinterpret_cast<void**>(&(rawPointerTSFThreadManager)));
    if (FAILED(r))
    {
        OSG_WARN << "[TextInputMethodSystem] Failed creating the TSF thread manager.\n";
        _tsfInputProcessorProfiles.Reset(); _tsfInputProcessorProfileManager.Reset();
        return false;
    }
    else
        _tsfThreadManager.Attach(rawPointerTSFThreadManager);

    // Activate thread manager
    r = _tsfThreadManager->Activate(&(_tsfClientId));
    if (FAILED(r))
    {
        OSG_WARN << "[TextInputMethodSystem] Failed while activating the TSF thread manager\n";
        _tsfInputProcessorProfiles.Reset(); _tsfInputProcessorProfileManager.Reset();
        _tsfThreadManager.Reset(); return false;
    }

    // Get source from thread manager, needed to install profile processor related sinks
    ComPtr<ITfSource> tsfSource;
    r = tsfSource.FromQueryInterface(IID_ITfSource, _tsfThreadManager);
    if (FAILED(r))
    {
        OSG_WARN << "[TextInputMethodSystem] Failed while acquiring the TSF source from thread manager\n";
        _tsfInputProcessorProfiles.Reset(); _tsfInputProcessorProfileManager.Reset();
        _tsfThreadManager.Reset(); return false;
    }

    _tsfActivationProxy = new TSFActivationProxy(this);
    r = tsfSource->AdviseSink(IID_ITfInputProcessorProfileActivationSink,
            static_cast<ITfInputProcessorProfileActivationSink*>(_tsfActivationProxy),
            &(_tsfActivationProxy->TSFProfileCookie));
    if (FAILED(r))
    {
        OSG_WARN << "[TextInputMethodSystem] Failed while advising the profile notification sink to source\n";
        _tsfInputProcessorProfiles.Reset(); _tsfInputProcessorProfileManager.Reset();
        _tsfThreadManager.Reset(); _tsfActivationProxy.Reset(); return false;
    }

    // Disabled Document Manager
    r = _tsfThreadManager->CreateDocumentMgr(&(_tsfDisabledDocumentManager));
    if (FAILED(r))
    {
        OSG_WARN << "[TextInputMethodSystem] Failed while creating TSF document manager\n";
        _tsfInputProcessorProfiles.Reset(); _tsfInputProcessorProfileManager.Reset();
        _tsfThreadManager.Reset(); _tsfActivationProxy.Reset(); return false;
    }

    // Default the focus to the disabled document manager.
    r = _tsfThreadManager->SetFocus(_tsfDisabledDocumentManager);
    if (FAILED(r))
    {
        OSG_WARN << "[TextInputMethodSystem] Failed while activating TSF document manager\n";
        _tsfInputProcessorProfiles.Reset(); _tsfInputProcessorProfileManager.Reset();
        _tsfThreadManager.Reset(); _tsfActivationProxy.Reset(); return false;
    }

    // Query TSF-based input methods first, which will include physical keyboards and TSF-based IMEs
    std::vector<std::wstring> availableInputMethods;
    std::set<HKL> processedKeyboardLayouts;
    ComPtr<IEnumTfInputProcessorProfiles> tsfEnumInputProcessorProfiles;
    if (SUCCEEDED(_tsfInputProcessorProfileManager->EnumProfiles(0, &tsfEnumInputProcessorProfiles)))
    {
        ULONG fetchedTSFProfilesCount = 0;
        TF_INPUTPROCESSORPROFILE tsfProfiles[32];
        while (SUCCEEDED(tsfEnumInputProcessorProfiles->Next(32, tsfProfiles, &fetchedTSFProfilesCount))
            && fetchedTSFProfilesCount > 0)
        {
            for (ULONG index = 0; index < fetchedTSFProfilesCount; ++index)
            {
                const TF_INPUTPROCESSORPROFILE& tsfProfile = tsfProfiles[index];
                if (tsfProfile.dwProfileType == TF_PROFILETYPE_KEYBOARDLAYOUT)
                    processedKeyboardLayouts.insert(tsfProfile.hkl);
                if (tsfProfile.dwFlags & TF_IPP_FLAG_ENABLED)
                {
                    LCID lcid = MAKELCID(tsfProfile.langid, SORT_DEFAULT);
                    const int32 neededSize = ::GetLocaleInfoW(lcid, LOCALE_SLANGUAGE, nullptr, 0);
                    std::vector<wchar_t> lcidInfo(neededSize);
                    ::GetLocaleInfoW(lcid, LOCALE_SLANGUAGE, lcidInfo.data(), neededSize);

                    std::wstring inputMethodDesc(lcidInfo.begin(), lcidInfo.end());
                    if (tsfProfile.dwProfileType == TF_PROFILETYPE_KEYBOARDLAYOUT)
                        inputMethodDesc += L" (Keyboard)";
                    else if (tsfProfile.dwProfileType == TF_PROFILETYPE_INPUTPROCESSOR)
                    {
                        BSTR description;
                        if (SUCCEEDED(_tsfInputProcessorProfiles->GetLanguageProfileDescription(
                                tsfProfile.clsid, tsfProfile.langid, tsfProfile.guidProfile, &description)))
                        {
                            inputMethodDesc += L": " + std::wstring(description);
                            ::SysFreeString(description);
                        }
                        inputMethodDesc += L" (TSF IME)";
                    }
                    availableInputMethods.push_back(inputMethodDesc);
                }
            }
        }
    }

    // Detect whether we have an IME active
    TF_INPUTPROCESSORPROFILE tsfProfile;
    if (SUCCEEDED(_tsfInputProcessorProfileManager->GetActiveProfile(GUID_TFCAT_TIP_KEYBOARD, &tsfProfile))
        && tsfProfile.dwProfileType == TF_PROFILETYPE_INPUTPROCESSOR)
    { _tsfActivated = true; }

    std::wcout << L"Available Input Methods: " << std::endl;
    for (size_t i = 0; i < availableInputMethods.size(); ++i)
        std::wcout << L"    " << availableInputMethods[i] << std::endl;
    return true;
}

void TsfTextInputMethodSystem::Terminate()
{
    // Get source from thread manager, needed to uninstall profile processor related sinks
    ComPtr<ITfSource> tsfSource;
    HRESULT r = tsfSource.FromQueryInterface(IID_ITfSource, _tsfThreadManager);
    if (tsfSource && _tsfActivationProxy)
    {
        // Uninstall language notification sink
        if (_tsfActivationProxy->TSFLanguageCookie != TF_INVALID_COOKIE)
            r = tsfSource->UnadviseSink(_tsfActivationProxy->TSFLanguageCookie);

        // Uninstall profile notification sink
        if (_tsfActivationProxy->TSFProfileCookie != TF_INVALID_COOKIE)
            r = tsfSource->UnadviseSink(_tsfActivationProxy->TSFProfileCookie);
    }

    _tsfActivationProxy.Reset();
    _tsfThreadManager->Deactivate();
    _tsfThreadManager.Reset();
    _tsfDisabledDocumentManager.Reset();
    _tsfInputProcessorProfiles.Reset();
    _tsfInputProcessorProfileManager.Reset();
}

void TextInputMethodSystem::OnIMEActivationStateChanged(bool isEnabled)
{
    _tsfActivated = false;
    if (isEnabled)
    {
        TF_INPUTPROCESSORPROFILE tsfProfile;
        if (SUCCEEDED(_tsfInputProcessorProfileManager->GetActiveProfile(GUID_TFCAT_TIP_KEYBOARD, &tsfProfile))
            && tsfProfile.dwProfileType == TF_PROFILETYPE_INPUTPROCESSOR)
        { _tsfActivated = true; }
    }
}

void TextInputMethodSystem::ApplyDefaults(HWND window)
{
    ITfDocumentMgr* tsfDocumentManagerToSet = nullptr;
    if (_activeContext.valid())
    {
        const HRESULT r = _tsfThreadManager->GetFocus(&tsfDocumentManagerToSet);
        if (FAILED(r))
        {
            OSG_NOTICE << "[TextInputMethodSystem] Getting the active TSF document manager failed, "
                       << "will fallback to using the disabled document manager\n";
            tsfDocumentManagerToSet = nullptr;
        }
    }

    // TSF Implementation
    if (tsfDocumentManagerToSet)
        _tsfThreadManager->SetFocus(tsfDocumentManagerToSet);
    else
    {
        ITfDocumentMgr* unused = NULL;
        _tsfThreadManager->AssociateFocus(window, _tsfDisabledDocumentManager, &unused);
    }
}
