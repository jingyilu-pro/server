//
// Copyright (c) 2024-2025 JingyiLu jingyilupro@gmail.com
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#include "basic_http_service.h"

#include "http_code_message.h"

#include "log/glogger.h"

#include <algorithm>
#include <cctype>
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>
#include <event2/thread.h>
#include <event2/util.h>
#include <google/protobuf/message.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

namespace
{

uint16_t resolve_bound_port(evhttp_bound_socket* bound_socket)
{
    if(bound_socket == nullptr)
    {
        return 0;
    }

    const evutil_socket_t fd = evhttp_bound_socket_get_fd(bound_socket);
    if(fd < 0)
    {
        return 0;
    }

    sockaddr_storage address{};
    ev_socklen_t address_length = static_cast<ev_socklen_t>(sizeof(address));
    if(getsockname(fd, reinterpret_cast<sockaddr*>(&address), &address_length) != 0)
    {
        return 0;
    }

    if(address.ss_family == AF_INET)
    {
        const auto* ipv4 = reinterpret_cast<const sockaddr_in*>(&address);
        return ntohs(ipv4->sin_port);
    }
    if(address.ss_family == AF_INET6)
    {
        const auto* ipv6 = reinterpret_cast<const sockaddr_in6*>(&address);
        return ntohs(ipv6->sin6_port);
    }

    return 0;
}

} // namespace

BasicHttpService::BasicHttpService(std::string service_name, EndpointConfig endpoint, bool auto_assign_port)
    : m_service_name(std::move(service_name)),
      m_endpoint(std::move(endpoint)),
      m_auto_assign_port(auto_assign_port)
{
}

BasicHttpService::~BasicHttpService()
{
    stop();
}

const char* BasicHttpService::name() const
{
    return m_service_name.c_str();
}

bool BasicHttpService::start()
{
    if(m_running.load())
    {
        return true;
    }

    evthread_use_pthreads();
    m_event_base = event_base_new();
    if(m_event_base == nullptr)
    {
        spdlog::error("{} failed to create event base", m_service_name);
        return false;
    }

    m_evhttp = evhttp_new(m_event_base);
    if(m_evhttp == nullptr)
    {
        spdlog::error("{} failed to create evhttp", m_service_name);
        event_base_free(m_event_base);
        m_event_base = nullptr;
        return false;
    }

    evhttp_set_gencb(m_evhttp, &BasicHttpService::global_request_callback, this);

    if(m_auto_assign_port)
    {
        auto* bound_socket = evhttp_bind_socket_with_handle(m_evhttp, m_endpoint.host.c_str(), 0);
        if(bound_socket == nullptr)
        {
            spdlog::error("{} failed to bind endpoint {}", m_service_name, make_endpoint_text(m_endpoint));
            evhttp_free(m_evhttp);
            m_evhttp = nullptr;
            event_base_free(m_event_base);
            m_event_base = nullptr;
            return false;
        }

        const uint16_t bound_port = resolve_bound_port(bound_socket);
        if(bound_port == 0)
        {
            spdlog::error("{} failed to resolve auto-assigned port", m_service_name);
            evhttp_free(m_evhttp);
            m_evhttp = nullptr;
            event_base_free(m_event_base);
            m_event_base = nullptr;
            return false;
        }
        m_endpoint.port = bound_port;
    }
    else
    {
        const auto bind_result = evhttp_bind_socket(m_evhttp, m_endpoint.host.c_str(), static_cast<ev_uint16_t>(m_endpoint.port));
        if(bind_result != 0)
        {
            spdlog::error("{} failed to bind endpoint {}", m_service_name, make_endpoint_text(m_endpoint));
            evhttp_free(m_evhttp);
            m_evhttp = nullptr;
            event_base_free(m_event_base);
            m_event_base = nullptr;
            return false;
        }
    }

    m_running.store(true);
    m_thread = std::thread(&BasicHttpService::event_loop, this);
    spdlog::info("{} listening at {}", m_service_name, make_endpoint_text(m_endpoint));
    return true;
}

bool BasicHttpService::stop()
{
    if(!m_running.load())
    {
        return true;
    }

    m_running.store(false);
    if(m_event_base != nullptr)
    {
        event_base_loopbreak(m_event_base);
    }
    if(m_thread.joinable())
    {
        m_thread.join();
    }

    {
        std::lock_guard lock(m_owned_mutex);
        for(auto* request : m_owned_requests)
        {
            if(request != nullptr)
            {
                evhttp_request_free(request);
            }
        }
        m_owned_requests.clear();
    }

    if(m_evhttp != nullptr)
    {
        evhttp_free(m_evhttp);
        m_evhttp = nullptr;
    }
    if(m_event_base != nullptr)
    {
        event_base_free(m_event_base);
        m_event_base = nullptr;
    }

    spdlog::info("{} stopped", m_service_name);
    return true;
}

void BasicHttpService::update(std::chrono::milliseconds delta_time, std::chrono::milliseconds last_tick_time)
{
    (void)delta_time;
    (void)last_tick_time;
}

bool BasicHttpService::register_handler(const std::string& path, Handler handler)
{
    if(path.empty() || !handler)
    {
        return false;
    }
    m_handlers[path] = std::move(handler);
    return true;
}

const EndpointConfig& BasicHttpService::endpoint() const
{
    return m_endpoint;
}

void BasicHttpService::retain_request(evhttp_request* request)
{
    if(request == nullptr)
    {
        return;
    }

    evhttp_request_own(request);
    std::lock_guard lock(m_owned_mutex);
    m_owned_requests.insert(request);
}

void BasicHttpService::release_request(evhttp_request* request)
{
    if(request == nullptr)
    {
        return;
    }

    std::lock_guard lock(m_owned_mutex);
    const auto iter = m_owned_requests.find(request);
    if(iter == m_owned_requests.end())
    {
        return;
    }

    m_owned_requests.erase(iter);
    evhttp_request_free(request);
}

void BasicHttpService::on_event_loop_tick()
{
}

std::string BasicHttpService::read_request_body(evhttp_request* request)
{
    if(request == nullptr)
    {
        return {};
    }
    evbuffer* input = evhttp_request_get_input_buffer(request);
    if(input == nullptr)
    {
        return {};
    }
    const size_t length = evbuffer_get_length(input);
    std::string body;
    body.resize(length);
    if(length > 0)
    {
        evbuffer_copyout(input, body.data(), length);
    }
    return body;
}

bool BasicHttpService::write_protobuf_response(evhttp_request* request, const google::protobuf::Message& response, int http_status)
{
    if(request == nullptr)
    {
        return false;
    }

    std::string output_data;
    if(!response.SerializeToString(&output_data))
    {
        return false;
    }

    evbuffer* output = evbuffer_new();
    if(output == nullptr)
    {
        return false;
    }
    evbuffer_add(output, output_data.data(), output_data.size());

    auto* headers = evhttp_request_get_output_headers(request);
    if(headers != nullptr)
    {
        evhttp_add_header(headers, "Content-Type", "application/x-protobuf");

        bool client_wants_close = false;
        auto* input_headers = evhttp_request_get_input_headers(request);
        if(input_headers != nullptr)
        {
            if(const char* connection_header = evhttp_find_header(input_headers, "Connection");
               connection_header != nullptr)
            {
                std::string value = connection_header;
                std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
                    return static_cast<char>(std::tolower(ch));
                });
                client_wants_close = value.find("close") != std::string::npos;
            }
        }

        if(client_wants_close)
        {
            evhttp_add_header(headers, "Connection", "close");
        }
    }
    evhttp_send_reply(request, http_status, "OK", output);
    evbuffer_free(output);
    return true;
}

std::string BasicHttpService::extract_authorization_token(evhttp_request* request)
{
    if(request == nullptr)
    {
        return {};
    }

    auto* headers = evhttp_request_get_input_headers(request);
    if(headers == nullptr)
    {
        return {};
    }

    const char* authorization = evhttp_find_header(headers, "Authorization");
    if(authorization == nullptr)
    {
        return {};
    }

    std::string token = authorization;
    while(!token.empty() && std::isspace(static_cast<unsigned char>(token.front())) != 0)
    {
        token.erase(token.begin());
    }
    while(!token.empty() && std::isspace(static_cast<unsigned char>(token.back())) != 0)
    {
        token.pop_back();
    }

    const std::string bearer_scheme = "bearer";
    if(token.size() > bearer_scheme.size())
    {
        std::string scheme = token.substr(0, bearer_scheme.size());
        std::transform(scheme.begin(), scheme.end(), scheme.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });

        if(scheme == bearer_scheme && std::isspace(static_cast<unsigned char>(token[bearer_scheme.size()])) != 0)
        {
            std::size_t value_offset = bearer_scheme.size();
            while(value_offset < token.size() &&
                  std::isspace(static_cast<unsigned char>(token[value_offset])) != 0)
            {
                ++value_offset;
            }
            token = token.substr(value_offset);
        }
    }

    return token;
}

bool BasicHttpService::is_protobuf_content_type(evhttp_request* request)
{
    if(request == nullptr)
    {
        return false;
    }

    auto* headers = evhttp_request_get_input_headers(request);
    if(headers == nullptr)
    {
        return false;
    }

    const char* content_type = evhttp_find_header(headers, "Content-Type");
    if(content_type == nullptr)
    {
        return false;
    }

    std::string normalized = content_type;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    const auto semicolon_pos = normalized.find(';');
    if(semicolon_pos != std::string::npos)
    {
        normalized = normalized.substr(0, semicolon_pos);
    }

    while(!normalized.empty() && std::isspace(static_cast<unsigned char>(normalized.back())) != 0)
    {
        normalized.pop_back();
    }
    while(!normalized.empty() && std::isspace(static_cast<unsigned char>(normalized.front())) != 0)
    {
        normalized.erase(normalized.begin());
    }

    return normalized == "application/x-protobuf" || normalized == "application/octet-stream";
}

std::string BasicHttpService::make_endpoint_text(const EndpointConfig& endpoint)
{
    return endpoint.host + ":" + std::to_string(endpoint.port);
}

void BasicHttpService::global_request_callback(evhttp_request* request, void* arg)
{
    auto* self = static_cast<BasicHttpService*>(arg);
    if(self == nullptr)
    {
        evhttp_send_error(request,
                          http_code_message::transport::status::kInternalServerError,
                          http_code_message::transport::message::kInternalError);
        return;
    }
    self->on_request(request);
}

void BasicHttpService::on_request(evhttp_request* request)
{
    if(request == nullptr)
    {
        return;
    }

    const auto command = evhttp_request_get_command(request);
    if(command != EVHTTP_REQ_POST)
    {
        evhttp_send_error(request,
                          http_code_message::transport::status::kMethodNotAllowed,
                          http_code_message::transport::message::kMethodNotAllowed);
        return;
    }

    if(!is_protobuf_content_type(request))
    {
        evhttp_send_error(request,
                          http_code_message::transport::status::kUnsupportedMediaType,
                          http_code_message::transport::message::kUnsupportedMediaType);
        return;
    }

    const char* raw_uri = evhttp_request_get_uri(request);
    if(raw_uri == nullptr)
    {
        evhttp_send_error(request,
                          http_code_message::transport::status::kBadRequest,
                          http_code_message::transport::message::kInvalidUri);
        return;
    }

    std::string uri = raw_uri;
    auto query_pos = uri.find('?');
    if(query_pos != std::string::npos)
    {
        uri = uri.substr(0, query_pos);
    }

    auto it = m_handlers.find(uri);
    if(it == m_handlers.end())
    {
        evhttp_send_error(request,
                          http_code_message::transport::status::kNotFound,
                          http_code_message::transport::message::kNotFound);
        return;
    }

    it->second(request);
}

void BasicHttpService::event_loop()
{
    if(m_event_base == nullptr)
    {
        return;
    }

    while(m_running.load())
    {
        on_event_loop_tick();
        const auto code = event_base_loop(m_event_base, EVLOOP_ONCE | EVLOOP_NONBLOCK);
        if(code != 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}
