#include "core/managers/connect_manager.h"

#ifdef _WIN32
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif // #ifdef _WIN32

namespace mngr
{

#ifdef _WIN32
void ConnectManager::send_https(std::shared_ptr<RequestData> req)
{
    const auto reply = [req](const char* err)
    {
        req->status = HTTP_RESP_FAIL;
        LogErrorS("HTTP-Req-Ext#" << req->req_id << ". Failed: " << err << ", err " << LogApiERR);
    };

    if (req->port != INTERNET_DEFAULT_HTTPS_PORT)
        return reply("invalid request");

    using InternetHandle = std::unique_ptr<void, decltype(&::WinHttpCloseHandle)>;

    InternetHandle hsession(::WinHttpOpen(L"Graph-N/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0), ::WinHttpCloseHandle);
    if (!hsession)
        return reply("WinHttpOpen failed");

    const auto server = tools::str::utf8_to_wide(req->server);
    InternetHandle hconnect(::WinHttpConnect(hsession.get(), server.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0), ::WinHttpCloseHandle);
    if (!hconnect)
        return reply("WinHttpConnect failed");

    const auto url = tools::str::utf8_to_wide(req->url);
    const wchar_t* method = (req->type == RequestData::Type::PostRawPayload) ? L"POST" : L"GET";
    InternetHandle hrequest(::WinHttpOpenRequest(hconnect.get(), method, url.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE), ::WinHttpCloseHandle);
    if (!hrequest)
        return reply("WinHttpOpenRequest failed");

    const int32_t timeout_ms = g::app_global_stop_all_threads.stop_requested() ? 1000 : 5000;
    ::WinHttpSetTimeouts(hrequest.get(), timeout_ms, timeout_ms, timeout_ms, timeout_ms);

    if (RequestData::Type::PostRawPayload == req->type)
    {
        const std::wstring header = L"Content-Type: application/json\r\n";
        ::WinHttpAddRequestHeaders(hrequest.get(), header.c_str(), DWORD(-1), WINHTTP_ADDREQ_FLAG_ADD);
    }

    LPVOID payload_ptr = req->post_payload.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)req->post_payload.data();
    DWORD payload_size = cast::to<DWORD>(req->post_payload.size());
    BOOL results = ::WinHttpSendRequest(hrequest.get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0, payload_ptr, payload_size, payload_size, 0);

    if (results)
        results = ::WinHttpReceiveResponse(hrequest.get(), NULL);
    if (!results)
        return reply("WinHttpReceiveResponse/Send failed");

    DWORD status_code = 0;
    DWORD size = sizeof(status_code);
    ::WinHttpQueryHeaders(hrequest.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status_code, &size, WINHTTP_NO_HEADER_INDEX);
    req->status = cast::to_i32(status_code);

    DWORD available = 0;
    do
    {
        if (!::WinHttpQueryDataAvailable(hrequest.get(), &available))
            return reply("WinHttpQueryDataAvailable failed");

        if (0 == available)
            break;

        std::vector<char> buffer(available);
        DWORD downloaded = 0;
        if (!::WinHttpReadData(hrequest.get(), buffer.data(), available, &downloaded))
            return reply("WinHttpReadData failed");

        req->data.append(buffer.data(), downloaded);
    }
    while (available > 0);

    Log("HTTP-Req-Ext#" << req->req_id << ". Finished. Result: " << cast::to_u32(status_code) << ". Body: " << req->data.size() << " bytes");
}

#else
void ConnectManager::send_https(std::shared_ptr<RequestData> req)
{
    if (req->port != INTERNET_DEFAULT_HTTPS_PORT)
    {
        req->status = HTTP_RESP_FAIL;
        LogError("HTTP-Req-Ext#" << req->req_id << ". Failed: invalid request", );
        return;
    }
    req->status = HTTP_RESP_FAIL;
    Log("HTTP-Req-Ext#" << req->req_id << ". Failed: not implemented");
}
#endif // #ifdef _WIN32

} // namespace mngr
