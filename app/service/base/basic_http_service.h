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

#pragma once

#include <service.h>

#include "application_config.h"

#include <atomic>
#include <cstdint>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/thread.h>
#include <functional>
#include <mutex>
#include <unordered_set>
#include <string>
#include <thread>
#include <unordered_map>

namespace google::protobuf
{
class Message;
}

class BasicHttpService : public Service
{
public:
    using Handler = std::function<void(evhttp_request*)>;

public:
    BasicHttpService(std::string service_name, EndpointConfig endpoint, bool auto_assign_port = false);
    ~BasicHttpService() override;

public:
    const char* name() const override;
    bool start() override;
    bool stop() override;
    void update(std::chrono::milliseconds delta_time, std::chrono::milliseconds last_tick_time) override;

protected:
    bool register_handler(const std::string& path, Handler handler);
    const EndpointConfig& endpoint() const;
    static std::string read_request_body(evhttp_request* request);
    static bool write_protobuf_response(evhttp_request* request, const google::protobuf::Message& response, int http_status = 200);
    static std::string extract_authorization_token(evhttp_request* request);
    static bool is_protobuf_content_type(evhttp_request* request);
    static std::string make_endpoint_text(const EndpointConfig& endpoint);
    void retain_request(evhttp_request* request);
    void release_request(evhttp_request* request);
    virtual void on_event_loop_tick();

private:
    static void global_request_callback(evhttp_request* request, void* arg);
    void on_request(evhttp_request* request);
    void event_loop();

private:
    std::string m_service_name;
    EndpointConfig m_endpoint;
    bool m_auto_assign_port = false;
    std::unordered_map<std::string, Handler> m_handlers;

    event_base* m_event_base = nullptr;
    evhttp* m_evhttp = nullptr;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::unordered_set<evhttp_request*> m_owned_requests;
    std::mutex m_owned_mutex;
};
