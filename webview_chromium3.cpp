/////////////////////////////////////////////////////////////////////////////
// Author: Steven Lamerton
// Copyright: (c) 2013 Steven Lamerton
// Licence: wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "webview_chromium.h"

#if CEF_API == 3

#include "webview_chromium.h"

#include <wx/webview.h>
#include <wx/filesys.h>
#include <wx/msw/private.h>



#ifdef __VISUALC__
#pragma warning(push)
#pragma warning(disable:4100)
#endif

#include <include/cef_app.h>
#include <include/cef_browser.h>

#ifdef __VISUALC__
#pragma warning(pop)
#endif

extern const char wxWebViewBackendChromium[] = "wxWebViewChromium";

wxIMPLEMENT_DYNAMIC_CLASS(wxWebViewChromium, wxWebView);

CefRefPtr<ClientHandler> g_clientHandler;

bool wxWebViewChromium::Create(wxWindow* parent,
           wxWindowID id,
           const wxString& url,
           const wxPoint& pos,
           const wxSize& size,
           long style,
           const wxString& name)
{
    if (!wxControl::Create(parent, id, pos, size, style,
                           wxDefaultValidator, name))
    {
        return false;
    }

    m_historyLoadingFromList = false;
    m_historyEnabled = true;
    m_historyPosition = -1;

    CefBrowserSettings browsersettings;
    CefWindowInfo info;

    g_clientHandler->SetWebView(this);

#ifdef __WXMSW__
    // Initialize window info to the defaults for a child window
    info.SetAsChild(GetHWND(), wxGetClientRect(this->GetHWND()));
#endif
    // Creat the new child browser window, we do this async as we use a multi
    // threaded message loop
    CefBrowserHost::CreateBrowser(info, static_cast<CefRefPtr<CefClient>>(g_clientHandler),
                                  url.ToStdString(), browsersettings);

    this->Bind(wxEVT_SIZE, &wxWebViewChromium::OnSize, this);

    return true;
}

wxWebViewChromium::~wxWebViewChromium()
{
    CefRefPtr<CefBrowser> browser = g_clientHandler->GetBrowser();
    if(browser.get()) {
        // Let the browser window know we are about to destroy it.
        browser->GetHost()->ParentWindowWillClose();
    }
}

void wxWebViewChromium::OnSize(wxSizeEvent& event)
{
    wxSize size = GetClientSize();
    wxPoint pos = GetPosition();

#ifdef __WXMSW__
    if(g_clientHandler && g_clientHandler->GetBrowser() && g_clientHandler->GetBrowser()->GetHost())
    {
        HWND handle = g_clientHandler->GetBrowser()->GetHost()->GetWindowHandle();

        if(handle)
        {
            HDWP hdwp = BeginDeferWindowPos(1);
            hdwp = DeferWindowPos(hdwp, handle, NULL, pos.x, pos.y,
                                  size.GetWidth(), size.GetHeight(), SWP_NOZORDER);
            EndDeferWindowPos(hdwp);
        }
    }
#endif

    event.Skip();
}

void* wxWebViewChromium::GetNativeBackend() const
{
    return g_clientHandler->GetBrowser();
}

bool wxWebViewChromium::CanGoForward() const
{ 
    if(m_historyEnabled)
        return m_historyPosition != static_cast<int>(m_historyList.size()) - 1;
    else
        return false;
}

bool wxWebViewChromium::CanGoBack() const
{ 
    if(m_historyEnabled)
        return m_historyPosition > 0;
    else
        return false;
}

void wxWebViewChromium::LoadHistoryItem(wxSharedPtr<wxWebViewHistoryItem> item)
{
    int pos = -1;
    for(unsigned int i = 0; i < m_historyList.size(); i++)
    {
        //We compare the actual pointers to find the correct item
        if(m_historyList[i].get() == item.get())
            pos = i;
    }
    wxASSERT_MSG(pos != static_cast<int>(m_historyList.size()),
                 "invalid history item");
    m_historyLoadingFromList = true;
    LoadURL(item->GetUrl());
    m_historyPosition = pos;
}

wxVector<wxSharedPtr<wxWebViewHistoryItem> > wxWebViewChromium::GetBackwardHistory()
{
    wxVector<wxSharedPtr<wxWebViewHistoryItem> > backhist;
    //As we don't have std::copy or an iterator constructor in the wxwidgets
    //native vector we construct it by hand
    for(int i = 0; i < m_historyPosition; i++)
    {
        backhist.push_back(m_historyList[i]);
    }
    return backhist;
}

wxVector<wxSharedPtr<wxWebViewHistoryItem> > wxWebViewChromium::GetForwardHistory()
{
    wxVector<wxSharedPtr<wxWebViewHistoryItem> > forwardhist;
    //As we don't have std::copy or an iterator constructor in the wxwidgets
    //native vector we construct it by hand
    for(int i = m_historyPosition + 1; i < static_cast<int>(m_historyList.size()); i++)
    {
        forwardhist.push_back(m_historyList[i]);
    }
    return forwardhist;
}

void wxWebViewChromium::GoBack()
{
    LoadHistoryItem(m_historyList[m_historyPosition - 1]);
}

void wxWebViewChromium::GoForward()
{
    LoadHistoryItem(m_historyList[m_historyPosition + 1]);
}

void wxWebViewChromium::LoadURL(const wxString& url)
{ 
    g_clientHandler->GetBrowser()->GetMainFrame()->LoadURL(url.ToStdString());
}

void wxWebViewChromium::ClearHistory()
{
    m_historyList.clear();
    m_historyPosition = -1;
}

void wxWebViewChromium::EnableHistory(bool enable)
{
    m_historyEnabled = enable;
}

void wxWebViewChromium::Stop()
{
    g_clientHandler->GetBrowser()->StopLoad();
}

void wxWebViewChromium::Reload(wxWebViewReloadFlags flags)
{
    if(flags == wxWEBVIEW_RELOAD_NO_CACHE)
    {
        g_clientHandler->GetBrowser()->ReloadIgnoreCache();
    }
    else
    {
        g_clientHandler->GetBrowser()->Reload();
    }
}

wxString wxWebViewChromium::GetPageSource() const
{
    return "";
}

wxString wxWebViewChromium::GetPageText() const 
{
    return  "";
}

wxString wxWebViewChromium::GetCurrentURL() const
{
    return g_clientHandler->GetBrowser()->GetMainFrame()->GetURL().ToString();
}

wxString wxWebViewChromium::GetCurrentTitle() const
{
    return m_title;
}

void wxWebViewChromium::Print()
{
    //m_browser->GetMainFrame()->Print();
}

void wxWebViewChromium::Cut()
{
    g_clientHandler->GetBrowser()->GetMainFrame()->Cut();
}

void wxWebViewChromium::Copy()
{
    g_clientHandler->GetBrowser()->GetMainFrame()->Copy();
}

void wxWebViewChromium::Paste()
{
    g_clientHandler->GetBrowser()->GetMainFrame()->Paste();
}

void wxWebViewChromium::Undo()
{
    g_clientHandler->GetBrowser()->GetMainFrame()->Undo();
}

void wxWebViewChromium::Redo()
{
    g_clientHandler->GetBrowser()->GetMainFrame()->Redo();
}

void wxWebViewChromium::SelectAll()
{
   g_clientHandler->GetBrowser()->GetMainFrame()->SelectAll();
}

void wxWebViewChromium::DeleteSelection()
{
    wxString jsdelete = "if (window.getSelection) { if (window.getSelection().deleteFromDocument) { window.getSelection().deleteFromDocument(); } }";
    RunScript(jsdelete);
}

void wxWebViewChromium::ClearSelection()
{
    wxString jsclear = "if (window.getSelection) { if (window.getSelection().empty) { window.getSelection().empty(); } }";
    RunScript(jsclear);
}

void wxWebViewChromium::RunScript(const wxString& javascript)
{
    g_clientHandler->GetBrowser()->GetMainFrame()->ExecuteJavaScript(javascript.ToStdString(),
                                                                     "", 0);
}

bool wxWebViewChromium::IsBusy() const
{
    if(g_clientHandler->GetBrowser())
        return g_clientHandler->GetBrowser()->IsLoading();
    else
        return false;
}

void wxWebViewChromium::SetEditable(bool enable)
{
    wxString mode = enable ? "\"on\"" : "\"off\"";
    RunScript("document.designMode = " + mode); 
}

void wxWebViewChromium::DoSetPage(const wxString& html, const wxString& baseUrl)
{
    g_clientHandler->GetBrowser()->GetMainFrame()->LoadString(html.ToStdString(),
                                                              baseUrl.ToStdString());
}

wxWebViewZoom wxWebViewChromium::GetZoom() const
{
    float zoom = 0.0f;
    //In cef this must be called on the UI thread so is more complex than this
    //float zoom = g_clientHandler->GetBrowser()->GetHost()->GetZoomLevel();

    // arbitrary way to map float zoom to our common zoom enum
    if (zoom <= -0.75f)
    {
        return wxWEBVIEW_ZOOM_TINY;
    }
    else if (zoom > -0.75 && zoom <= -0.25)
    {
        return wxWEBVIEW_ZOOM_SMALL;
    }
    else if (zoom > -0.25 && zoom <= 0.25)
    {
        return wxWEBVIEW_ZOOM_MEDIUM;
    }
    else if (zoom > 0.25 && zoom <= 0.75)
    {
        return wxWEBVIEW_ZOOM_LARGE;
    }
    else if (zoom > 0.75)
    {
        return wxWEBVIEW_ZOOM_LARGEST;
    }

    // to shut up compilers, this can never be reached logically
    wxASSERT(false);
    return wxWEBVIEW_ZOOM_MEDIUM;
}


void wxWebViewChromium::SetZoom(wxWebViewZoom zoom)
{
    double mapzoom;
    // arbitrary way to map our common zoom enum to float zoom
    switch (zoom)
    {
        case wxWEBVIEW_ZOOM_TINY:
            mapzoom = -1.0;
            break;

        case wxWEBVIEW_ZOOM_SMALL:
            mapzoom = -0.5;
            break;

        case wxWEBVIEW_ZOOM_MEDIUM:
            mapzoom = 0.0;
            break;

        case wxWEBVIEW_ZOOM_LARGE:
            mapzoom = 0.5;
            break;

        case wxWEBVIEW_ZOOM_LARGEST:
            mapzoom = 1.0;
            break;

        default:
            wxASSERT(false);
    }
    g_clientHandler->GetBrowser()->GetHost()->SetZoomLevel(mapzoom);
}

void wxWebViewChromium::SetZoomType(wxWebViewZoomType type)
{
    // there is only one supported zoom type at the moment so this setter
    // does nothing beyond checking sanity
    wxASSERT(type == wxWEBVIEW_ZOOM_TYPE_LAYOUT);
}

wxWebViewZoomType wxWebViewChromium::GetZoomType() const
{
    return wxWEBVIEW_ZOOM_TYPE_LAYOUT;
}

bool wxWebViewChromium::CanSetZoomType(wxWebViewZoomType type) const
{
    return type == wxWEBVIEW_ZOOM_TYPE_LAYOUT;
}

void wxWebViewChromium::RegisterHandler(wxSharedPtr<wxWebViewHandler> handler)
{
    // We currently don't support custom scheme handlers
}

bool wxWebViewChromium::StartUp(int &code)
{
    if(!g_clientHandler)
        g_clientHandler = new ClientHandler;

    CefMainArgs args(wxGetInstance()); 

    code = CefExecuteProcess(args, NULL);
    if(code >= 0)
        return false;

    CefSettings settings;
    // We use a multithreaded message loop so we don't have to integrate
    // with the wx message loop
    settings.multi_threaded_message_loop = true;

    return CefInitialize(args, settings, NULL);
}

void wxWebViewChromium::Shutdown()
{
    CefShutdown();
}

// CefDisplayHandler methods
void ClientHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading,
                          bool canGoBack, bool canGoForward)
{}

void ClientHandler::OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                     const CefString& url)
{}

void ClientHandler::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title)
{
    m_webview->m_title = title.ToString();
    wxString target = browser->GetMainFrame()->GetName().ToString();

    wxWebViewEvent event(wxEVT_COMMAND_WEBVIEW_TITLE_CHANGED, m_webview->GetId(), "", target);
    event.SetString(title.ToString());
    event.SetEventObject(m_webview);

    m_webview->HandleWindowEvent(event);
}

bool ClientHandler::OnConsoleMessage(CefRefPtr<CefBrowser> browser, const CefString& message,
                      const CefString& source, int line)
{
    return false;
}

// CefLifeSpanHandler methods
bool ClientHandler::OnBeforePopup(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame,
                             const CefString& target_url,
                             const CefString& target_frame_name,
                             const CefPopupFeatures& popupFeatures,
                             CefWindowInfo& windowInfo,
                             CefRefPtr<CefClient>& client,
                             CefBrowserSettings& settings,
                             bool* no_javascript_access)
{
    return false;
}

void ClientHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
    if(!m_browser.get())
    {
        m_browser = browser;
        m_browserId = browser->GetIdentifier();
    }
}
bool ClientHandler::DoClose(CefRefPtr<CefBrowser> browser)
{
    return false;
}

void ClientHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
    if(browser->GetIdentifier() == m_browserId)
    {
        m_browser = NULL;
    }
}

// CefLoadHandler methods
void ClientHandler::OnLoadStart(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame)
{
    wxString url = frame->GetURL().ToString();
    wxString target = frame->GetName().ToString();

    wxWebViewEvent event(wxEVT_COMMAND_WEBVIEW_NAVIGATING, m_webview->GetId(), url, target);
    event.SetEventObject(m_webview);

    m_webview->HandleWindowEvent(event);

    if (!event.IsAllowed())
    {
        // We do not yet have support for vetoing pages
    }
}

void ClientHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame,
                              int httpStatusCode)
{
    wxString url = frame->GetURL().ToString();
    wxString target = frame->GetName().ToString();

    wxWebViewEvent event(wxEVT_COMMAND_WEBVIEW_NAVIGATED, m_webview->GetId(), url, target);
    event.SetEventObject(m_webview);

    m_webview->HandleWindowEvent(event);

    if(frame->IsMain())
    {
        //As we are complete we also add to the history list, but not if the
        //page is not the main page, ie it is a subframe
        if(m_webview->m_historyEnabled && !m_webview->m_historyLoadingFromList)
        {
            //If we are not at the end of the list, then erase everything
            //between us and the end before adding the new page
            if(m_webview->m_historyPosition != static_cast<int>(m_webview->m_historyList.size()) - 1)
            {
                m_webview->m_historyList.erase(m_webview->m_historyList.begin() + m_webview->m_historyPosition + 1,
                                               m_webview->m_historyList.end());
            }
            wxSharedPtr<wxWebViewHistoryItem> item(new wxWebViewHistoryItem(url, m_webview->GetCurrentTitle()));
            m_webview->m_historyList.push_back(item);
            m_webview->m_historyPosition++;
        }
        //Reset as we are done now
        m_webview->m_historyLoadingFromList = false;

        wxWebViewEvent levent(wxEVT_COMMAND_WEBVIEW_LOADED, m_webview->GetId(), url, target);
        levent.SetEventObject(m_webview);

        m_webview->HandleWindowEvent(levent);
    }
}

void ClientHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                ErrorCode errorCode,
                                const CefString& errorText,
                                const CefString& failedUrl)
{
    //We define a macro for convenience
    #define ERROR_TYPE_CASE(error, wxtype) case(error): \
    type = wxtype;\
    break

    wxWebViewNavigationError type = wxWEBVIEW_NAV_ERR_OTHER;
    switch (errorCode)
    {
        ERROR_TYPE_CASE(ERR_FAILED, wxWEBVIEW_NAV_ERR_OTHER);
        ERROR_TYPE_CASE(ERR_ABORTED, wxWEBVIEW_NAV_ERR_USER_CANCELLED);
        ERROR_TYPE_CASE(ERR_INVALID_ARGUMENT, wxWEBVIEW_NAV_ERR_OTHER);
        ERROR_TYPE_CASE(ERR_INVALID_HANDLE, wxWEBVIEW_NAV_ERR_OTHER);
        ERROR_TYPE_CASE(ERR_FILE_NOT_FOUND, wxWEBVIEW_NAV_ERR_NOT_FOUND);
        ERROR_TYPE_CASE(ERR_TIMED_OUT, wxWEBVIEW_NAV_ERR_CONNECTION);
        ERROR_TYPE_CASE(ERR_FILE_TOO_BIG, wxWEBVIEW_NAV_ERR_OTHER);
        ERROR_TYPE_CASE(ERR_UNEXPECTED, wxWEBVIEW_NAV_ERR_OTHER);
        ERROR_TYPE_CASE(ERR_ACCESS_DENIED, wxWEBVIEW_NAV_ERR_AUTH);
        ERROR_TYPE_CASE(ERR_NOT_IMPLEMENTED, wxWEBVIEW_NAV_ERR_OTHER);
        ERROR_TYPE_CASE(ERR_CONNECTION_CLOSED, wxWEBVIEW_NAV_ERR_CONNECTION);
        ERROR_TYPE_CASE(ERR_CONNECTION_RESET, wxWEBVIEW_NAV_ERR_CONNECTION);
        ERROR_TYPE_CASE(ERR_CONNECTION_REFUSED, wxWEBVIEW_NAV_ERR_CONNECTION);
        ERROR_TYPE_CASE(ERR_CONNECTION_ABORTED, wxWEBVIEW_NAV_ERR_CONNECTION);
        ERROR_TYPE_CASE(ERR_CONNECTION_FAILED, wxWEBVIEW_NAV_ERR_CONNECTION);
        ERROR_TYPE_CASE(ERR_NAME_NOT_RESOLVED, wxWEBVIEW_NAV_ERR_CONNECTION);
        ERROR_TYPE_CASE(ERR_INTERNET_DISCONNECTED, wxWEBVIEW_NAV_ERR_CONNECTION);
        ERROR_TYPE_CASE(ERR_SSL_PROTOCOL_ERROR, wxWEBVIEW_NAV_ERR_SECURITY);
        ERROR_TYPE_CASE(ERR_ADDRESS_INVALID, wxWEBVIEW_NAV_ERR_REQUEST);
        ERROR_TYPE_CASE(ERR_ADDRESS_UNREACHABLE, wxWEBVIEW_NAV_ERR_CONNECTION);
        ERROR_TYPE_CASE(ERR_SSL_CLIENT_AUTH_CERT_NEEDED, wxWEBVIEW_NAV_ERR_AUTH);
        ERROR_TYPE_CASE(ERR_TUNNEL_CONNECTION_FAILED, wxWEBVIEW_NAV_ERR_CONNECTION);
        ERROR_TYPE_CASE(ERR_NO_SSL_VERSIONS_ENABLED, wxWEBVIEW_NAV_ERR_SECURITY);
        ERROR_TYPE_CASE(ERR_SSL_VERSION_OR_CIPHER_MISMATCH, wxWEBVIEW_NAV_ERR_SECURITY);
        ERROR_TYPE_CASE(ERR_SSL_RENEGOTIATION_REQUESTED, wxWEBVIEW_NAV_ERR_REQUEST);
        ERROR_TYPE_CASE(ERR_CERT_COMMON_NAME_INVALID, wxWEBVIEW_NAV_ERR_CERTIFICATE);
        ERROR_TYPE_CASE(ERR_CERT_DATE_INVALID, wxWEBVIEW_NAV_ERR_CERTIFICATE);
        ERROR_TYPE_CASE(ERR_CERT_AUTHORITY_INVALID, wxWEBVIEW_NAV_ERR_CERTIFICATE);
        ERROR_TYPE_CASE(ERR_CERT_CONTAINS_ERRORS, wxWEBVIEW_NAV_ERR_CERTIFICATE);
        ERROR_TYPE_CASE(ERR_CERT_NO_REVOCATION_MECHANISM, wxWEBVIEW_NAV_ERR_CERTIFICATE);
        ERROR_TYPE_CASE(ERR_CERT_UNABLE_TO_CHECK_REVOCATION, wxWEBVIEW_NAV_ERR_CERTIFICATE);
        ERROR_TYPE_CASE(ERR_CERT_REVOKED, wxWEBVIEW_NAV_ERR_CERTIFICATE);
        ERROR_TYPE_CASE(ERR_CERT_INVALID, wxWEBVIEW_NAV_ERR_CERTIFICATE);
        ERROR_TYPE_CASE(ERR_CERT_END, wxWEBVIEW_NAV_ERR_CERTIFICATE);
        ERROR_TYPE_CASE(ERR_INVALID_URL, wxWEBVIEW_NAV_ERR_REQUEST);
        ERROR_TYPE_CASE(ERR_DISALLOWED_URL_SCHEME, wxWEBVIEW_NAV_ERR_REQUEST);
        ERROR_TYPE_CASE(ERR_UNKNOWN_URL_SCHEME, wxWEBVIEW_NAV_ERR_REQUEST);
        ERROR_TYPE_CASE(ERR_TOO_MANY_REDIRECTS, wxWEBVIEW_NAV_ERR_OTHER);
        ERROR_TYPE_CASE(ERR_UNSAFE_REDIRECT, wxWEBVIEW_NAV_ERR_SECURITY);
        ERROR_TYPE_CASE(ERR_UNSAFE_PORT, wxWEBVIEW_NAV_ERR_SECURITY);
        ERROR_TYPE_CASE(ERR_INVALID_RESPONSE, wxWEBVIEW_NAV_ERR_OTHER);
        ERROR_TYPE_CASE(ERR_INVALID_CHUNKED_ENCODING, wxWEBVIEW_NAV_ERR_OTHER);
        ERROR_TYPE_CASE(ERR_METHOD_NOT_SUPPORTED, wxWEBVIEW_NAV_ERR_OTHER);
        ERROR_TYPE_CASE(ERR_UNEXPECTED_PROXY_AUTH, wxWEBVIEW_NAV_ERR_OTHER);
        ERROR_TYPE_CASE(ERR_EMPTY_RESPONSE, wxWEBVIEW_NAV_ERR_OTHER);
        ERROR_TYPE_CASE(ERR_RESPONSE_HEADERS_TOO_BIG, wxWEBVIEW_NAV_ERR_OTHER);
        ERROR_TYPE_CASE(ERR_CACHE_MISS, wxWEBVIEW_NAV_ERR_OTHER);
        ERROR_TYPE_CASE(ERR_INSECURE_RESPONSE, wxWEBVIEW_NAV_ERR_SECURITY);
    }

    wxString url = failedUrl.ToString();
    wxString target = frame->GetName().ToString();
    wxWebViewEvent event(wxEVT_COMMAND_WEBVIEW_ERROR, m_webview->GetId(), url, target);
    event.SetEventObject(m_webview);
    event.SetInt(type);
    event.SetString(errorText.ToString());

    m_webview->HandleWindowEvent(event);
}

void ClientHandler::OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser,
                                              TerminationStatus status)
{}

#endif
