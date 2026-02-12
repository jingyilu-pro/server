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

#include "client_worker.h"

#include "coromanager.h"
#include "cororesult.h"
#include "protocol/gateway.pb.h"

#include <event2/buffer.h>
#include <event2/event.h>
#include <event2/http.h>

#include <chrono>
#include <memory>
#include <thread>

struct HttpResponse
{
    int status_code = 0;
    std::string body;
    bool timed_out = false;
    bool ok = false;
};

namespace
{

int64_t duration_us(std::chrono::steady_clock::time_point begin)
{
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now - begin).count();
}

std::string endpoint_path(const char* path)
{
    return std::string(path == nullptr ? "/" : path);
}

struct HttpClientOpResult : public CoroResult
{
    EndpointConfig endpoint;
    std::string path;
    std::string request_body;
    int timeout_ms = 2000;
    std::vector<std::string> headers;
    HttpResponse response;
    std::string error;

    void clear() override
    {
        endpoint = {};
        path.clear();
        request_body.clear();
        timeout_ms = 2000;
        headers.clear();
        response = {};
        error.clear();
    }

    void worker() override
    {
        response = {};
        error.clear();

        event_base* base = event_base_new();
        if(base == nullptr)
        {
            error = "event_base_new_failed";
            return;
        }

        evhttp_connection* connection = evhttp_connection_base_new(base,
                                                                   nullptr,
                                                                   endpoint.host.c_str(),
                                                                   static_cast<ev_uint16_t>(endpoint.port));
        if(connection == nullptr)
        {
            event_base_free(base);
            error = "evhttp_connection_base_new_failed";
            return;
        }
        evhttp_connection_set_timeout(connection, std::max(1, timeout_ms / 1000));

        struct RequestState
        {
            HttpClientOpResult* result = nullptr;
        };

        RequestState state;
        state.result = this;

        auto* request = evhttp_request_new(
            [](evhttp_request* req, void* arg) {
                auto* req_state = static_cast<RequestState*>(arg);
                if(req_state == nullptr || req_state->result == nullptr)
                {
                    return;
                }

                if(req == nullptr)
                {
                    req_state->result->response.ok = false;
                    if(req_state->result->error.empty())
                    {
                        req_state->result->error = "http_request_null";
                    }
                    return;
                }

                auto* input = evhttp_request_get_input_buffer(req);
                if(input != nullptr)
                {
                    const size_t length = evbuffer_get_length(input);
                    req_state->result->response.body.resize(length);
                    if(length > 0)
                    {
                        evbuffer_copyout(input, req_state->result->response.body.data(), length);
                    }
                }

                req_state->result->response.status_code = evhttp_request_get_response_code(req);
                req_state->result->response.ok = req_state->result->response.status_code > 0;
            },
            &state);

        if(request == nullptr)
        {
            evhttp_connection_free(connection);
            event_base_free(base);
            error = "evhttp_request_new_failed";
            return;
        }

        evhttp_request_set_error_cb(
            request,
            [](evhttp_request_error err, void* arg) {
                auto* req_state = static_cast<RequestState*>(arg);
                if(req_state == nullptr || req_state->result == nullptr)
                {
                    return;
                }

                req_state->result->response.ok = false;
                req_state->result->response.timed_out = (err == EVREQ_HTTP_TIMEOUT);
                if(req_state->result->error.empty())
                {
                    switch(err)
                    {
                    case EVREQ_HTTP_TIMEOUT:
                        req_state->result->error = "http_timeout";
                        break;
                    case EVREQ_HTTP_EOF:
                        req_state->result->error = "http_eof";
                        break;
                    case EVREQ_HTTP_INVALID_HEADER:
                        req_state->result->error = "http_invalid_header";
                        break;
                    case EVREQ_HTTP_BUFFER_ERROR:
                        req_state->result->error = "http_buffer_error";
                        break;
                    case EVREQ_HTTP_REQUEST_CANCEL:
                        req_state->result->error = "http_request_cancel";
                        break;
                    case EVREQ_HTTP_DATA_TOO_LONG:
                        req_state->result->error = "http_data_too_long";
                        break;
                    default:
                        req_state->result->error = "http_unknown_error";
                        break;
                    }
                }
            });

        auto* output_headers = evhttp_request_get_output_headers(request);
        evhttp_add_header(output_headers, "Host", (endpoint.host + ":" + std::to_string(endpoint.port)).c_str());
        evhttp_add_header(output_headers, "Connection", "close");
        evhttp_add_header(output_headers, "Content-Type", "application/x-protobuf");
        for(const auto& header : headers)
        {
            const auto colon_pos = header.find(':');
            if(colon_pos == std::string::npos)
            {
                continue;
            }
            auto key = header.substr(0, colon_pos);
            auto value = header.substr(colon_pos + 1);
            while(!value.empty() && value.front() == ' ')
            {
                value.erase(value.begin());
            }
            if(!key.empty() && !value.empty())
            {
                evhttp_add_header(output_headers, key.c_str(), value.c_str());
            }
        }

        auto* output_buffer = evhttp_request_get_output_buffer(request);
        if(output_buffer != nullptr && !request_body.empty())
        {
            evbuffer_add(output_buffer, request_body.data(), request_body.size());
        }

        if(evhttp_make_request(connection, request, EVHTTP_REQ_POST, endpoint_path(path.c_str()).c_str()) != 0)
        {
            evhttp_request_free(request);
            evhttp_connection_free(connection);
            event_base_free(base);
            error = "evhttp_make_request_failed";
            return;
        }

        event_base_dispatch(base);

        if(!response.ok && error.empty() && response.status_code <= 0)
        {
            error = "http_request_failed";
        }

        evhttp_connection_free(connection);
        event_base_free(base);
    }
};

class HttpClientManager : public CoroManager
{
public:
    explicit HttpClientManager(int worker_count)
        : CoroManager(worker_count)
    {
        CoroManager::init();
    }

    ~HttpClientManager() override = default;

public:
    CoroResult* alloc() override
    {
        expand<HttpClientOpResult>();
        return inner_alloc();
    }
};

struct HttpWaitState
{
    bool done = false;
    bool has_result = false;
    bool success = false;
    HttpResponse response;
};

coro_task_t capture_http_result(CoroAwaitable awaitable,
                                std::shared_ptr<HttpWaitState> state)
{
    auto* result = dynamic_cast<HttpClientOpResult*>(co_await awaitable);
    if(!state)
    {
        co_return;
    }

    if(result != nullptr)
    {
        state->response = result->response;
        state->success = result->response.ok && result->error.empty();
        state->has_result = true;
    }
    state->done = true;
}

} // namespace

class ClientWorker::HttpClientManager : public ::HttpClientManager
{
public:
    explicit HttpClientManager(int worker_count)
        : ::HttpClientManager(worker_count)
    {
    }
};

ClientWorker::ClientWorker(int http_coro_workers)
    : m_http_coro_workers(std::max(1, http_coro_workers))
{
    m_http_manager = std::make_unique<HttpClientManager>(m_http_coro_workers);
}

ClientWorker::~ClientWorker() = default;

bool ClientWorker::run_http_awaitable(CoroAwaitable awaitable, HttpResponse* out_response, int timeout_ms) const
{
    if(out_response == nullptr)
    {
        return false;
    }
    *out_response = {};

    auto wait_state = std::make_shared<HttpWaitState>();
    capture_http_result(awaitable, wait_state);

    if(timeout_ms <= 0)
    {
        timeout_ms = 2000;
    }

    auto begin = std::chrono::steady_clock::now();
    while(std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::steady_clock::now() - begin)
              .count() <= timeout_ms)
    {
        if(m_http_manager)
        {
            m_http_manager->update();
        }

        if(wait_state->done)
        {
            if(wait_state->has_result)
            {
                *out_response = wait_state->response;
                return wait_state->success;
            }
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    out_response->timed_out = true;
    return false;
}

CoroAwaitable ClientWorker::http_post_async(const EndpointConfig& endpoint,
                                            const char* path,
                                            const std::string& protobuf_body,
                                            int timeout_ms,
                                            const std::vector<std::string>& headers) const
{
    if(!m_http_manager)
    {
        return CoroAwaitable{nullptr, nullptr};
    }

    auto* result = dynamic_cast<HttpClientOpResult*>(m_http_manager->alloc());
    if(result == nullptr)
    {
        return CoroAwaitable{m_http_manager.get(), nullptr};
    }

    result->endpoint = endpoint;
    result->path = path == nullptr ? "/" : path;
    result->request_body = protobuf_body;
    result->timeout_ms = timeout_ms;
    result->headers = headers;

    return CoroAwaitable{m_http_manager.get(), result};
}

WorkerCycleResult ClientWorker::run(ClientPressureTask* task) const
{
    WorkerCycleResult result;
    if(task == nullptr)
    {
        result.failure_reason = "null_task";
        return result;
    }

    auto begin = std::chrono::steady_clock::now();
    const auto scenario = task->scenario.empty() ? std::string("full_chain") : task->scenario;

    if(scenario == "manager_only")
    {
        StageSample manager_sample;
        manager_sample.stage = StageType::manager;
        if(!do_manager_route(task, &manager_sample))
        {
            result.stages.push_back(manager_sample);
            result.failure_reason = manager_sample.error_reason.empty() ? "manager_failed" : manager_sample.error_reason;
            result.timeout = manager_sample.timeout;
            result.chain_latency_us = duration_us(begin);
            return result;
        }
        result.stages.push_back(manager_sample);
        result.success = true;
        result.chain_latency_us = duration_us(begin);
        return result;
    }

    if(scenario == "login_only")
    {
        StageSample login_sample;
        login_sample.stage = StageType::login;
        if(!do_login(task, &login_sample))
        {
            result.stages.push_back(login_sample);
            result.failure_reason = login_sample.error_reason.empty() ? "login_failed" : login_sample.error_reason;
            result.timeout = login_sample.timeout;
            result.chain_latency_us = duration_us(begin);
            return result;
        }
        result.stages.push_back(login_sample);
        result.success = true;
        result.chain_latency_us = duration_us(begin);
        return result;
    }

    if(scenario == "game_only")
    {
        if(!task->has_token)
        {
            StageSample login_sample;
            login_sample.stage = StageType::login;
            if(!do_login(task, &login_sample))
            {
                result.stages.push_back(login_sample);
                result.failure_reason = login_sample.error_reason.empty() ? "login_failed" : login_sample.error_reason;
                result.timeout = login_sample.timeout;
                result.chain_latency_us = duration_us(begin);
                return result;
            }
            result.stages.push_back(login_sample);
        }

        StageSample game_sample;
        game_sample.stage = StageType::game;
        if(!do_enter_game(*task, &game_sample))
        {
            result.stages.push_back(game_sample);
            result.failure_reason = game_sample.error_reason.empty() ? "game_enter_failed" : game_sample.error_reason;
            result.timeout = game_sample.timeout;
            result.chain_latency_us = duration_us(begin);
            return result;
        }
        result.stages.push_back(game_sample);
        result.success = true;
        result.chain_latency_us = duration_us(begin);
        return result;
    }

    StageSample manager_sample;
    manager_sample.stage = StageType::manager;
    if(!do_manager_route(task, &manager_sample))
    {
        result.stages.push_back(manager_sample);
        result.failure_reason = manager_sample.error_reason.empty() ? "manager_failed" : manager_sample.error_reason;
        result.timeout = manager_sample.timeout;
        result.chain_latency_us = duration_us(begin);
        return result;
    }
    result.stages.push_back(manager_sample);

    if(!task->has_token || task->auto_relogin)
    {
        StageSample login_sample;
        login_sample.stage = StageType::login;
        if(!do_login(task, &login_sample))
        {
            result.stages.push_back(login_sample);
            result.failure_reason = login_sample.error_reason.empty() ? "login_failed" : login_sample.error_reason;
            result.timeout = login_sample.timeout;
            result.chain_latency_us = duration_us(begin);
            return result;
        }
        result.stages.push_back(login_sample);
    }

    StageSample game_sample;
    game_sample.stage = StageType::game;
    if(!do_enter_game(*task, &game_sample))
    {
        result.stages.push_back(game_sample);
        result.failure_reason = game_sample.error_reason.empty() ? "game_enter_failed" : game_sample.error_reason;
        result.timeout = game_sample.timeout;
        result.chain_latency_us = duration_us(begin);
        return result;
    }
    result.stages.push_back(game_sample);

    result.success = true;
    result.chain_latency_us = duration_us(begin);
    return result;
}

bool ClientWorker::do_manager_route(ClientPressureTask* task, StageSample* sample) const
{
    if(sample == nullptr || task == nullptr)
    {
        return false;
    }

    gateway::RouteLoginRequest request;
    request.set_client_version("pressure_client_v1");

    std::string request_data;
    request.SerializeToString(&request_data);

    auto begin = std::chrono::steady_clock::now();
    HttpResponse response;
    const bool http_ok = run_http_awaitable(http_post_async(task->manager_endpoint,
                                                            "/v1/route/login",
                                                            request_data,
                                                            task->request_timeout_ms,
                                                            {}),
                                            &response,
                                            task->request_timeout_ms);

    sample->latency_us = duration_us(begin);
    sample->status_code = response.status_code;
    sample->timeout = response.timed_out;

    if(!http_ok)
    {
        sample->error_reason = response.timed_out ? "manager_timeout" : "manager_network_error";
        return false;
    }
    if(response.status_code != 200)
    {
        sample->error_reason = "manager_http_status_" + std::to_string(response.status_code);
        return false;
    }

    gateway::RouteLoginResponse route_response;
    if(!route_response.ParseFromString(response.body))
    {
        sample->error_reason = "manager_parse_error";
        return false;
    }
    if(route_response.code() != 0)
    {
        sample->error_reason = "manager_code_" + std::to_string(route_response.code());
        return false;
    }

    if(route_response.has_login_endpoint() && route_response.login_endpoint().port() > 0)
    {
        task->login_endpoint.host = route_response.login_endpoint().host();
        task->login_endpoint.port = static_cast<uint16_t>(route_response.login_endpoint().port());
    }
    if(route_response.has_game_endpoint() && route_response.game_endpoint().port() > 0)
    {
        task->game_endpoint.host = route_response.game_endpoint().host();
        task->game_endpoint.port = static_cast<uint16_t>(route_response.game_endpoint().port());
    }

    sample->success = true;
    return true;
}

bool ClientWorker::do_register(ClientPressureTask* task, std::string* error_reason) const
{
    if(task == nullptr)
    {
        if(error_reason != nullptr)
        {
            *error_reason = "register_null_task";
        }
        return false;
    }

    gateway::AuthRegisterRequest request;
    request.set_account(task->account);
    request.set_password("pressure_password");

    std::string request_data;
    request.SerializeToString(&request_data);

    HttpResponse response;
    const bool http_ok = run_http_awaitable(http_post_async(task->login_endpoint,
                                                            "/v1/auth/register",
                                                            request_data,
                                                            task->request_timeout_ms,
                                                            {}),
                                            &response,
                                            task->request_timeout_ms);

    if(!http_ok)
    {
        if(error_reason != nullptr)
        {
            *error_reason = response.timed_out ? "register_timeout" : "register_network_error";
        }
        return false;
    }
    if(response.status_code != 200)
    {
        if(error_reason != nullptr)
        {
            *error_reason = "register_http_status_" + std::to_string(response.status_code);
        }
        return false;
    }

    gateway::AuthRegisterResponse register_response;
    if(!register_response.ParseFromString(response.body))
    {
        if(error_reason != nullptr)
        {
            *error_reason = "register_parse_error";
        }
        return false;
    }

    if(register_response.code() != 0 && register_response.code() != 40001)
    {
        if(error_reason != nullptr)
        {
            *error_reason = "register_code_" + std::to_string(register_response.code());
        }
        return false;
    }

    return true;
}

bool ClientWorker::do_login(ClientPressureTask* task, StageSample* sample) const
{
    if(sample == nullptr || task == nullptr)
    {
        return false;
    }

    gateway::AuthLoginRequest request;
    request.set_account(task->account);
    request.set_password("pressure_password");

    std::string request_data;
    request.SerializeToString(&request_data);

    auto begin = std::chrono::steady_clock::now();
    HttpResponse response;
    bool http_ok = run_http_awaitable(http_post_async(task->login_endpoint,
                                                      "/v1/auth/login",
                                                      request_data,
                                                      task->request_timeout_ms,
                                                      {}),
                                      &response,
                                      task->request_timeout_ms);

    sample->latency_us = duration_us(begin);
    sample->status_code = response.status_code;
    sample->timeout = response.timed_out;

    if(!http_ok)
    {
        sample->error_reason = response.timed_out ? "login_timeout" : "login_network_error";
        return false;
    }
    if(response.status_code != 200)
    {
        sample->error_reason = "login_http_status_" + std::to_string(response.status_code);
        return false;
    }

    gateway::AuthLoginResponse login_response;
    if(!login_response.ParseFromString(response.body))
    {
        sample->error_reason = "login_parse_error";
        return false;
    }
    if(login_response.code() != 0)
    {
        if(login_response.code() == 40101)
        {
            std::string register_error;
            if(do_register(task, &register_error))
            {
                http_ok = run_http_awaitable(http_post_async(task->login_endpoint,
                                                             "/v1/auth/login",
                                                             request_data,
                                                             task->request_timeout_ms,
                                                             {}),
                                             &response,
                                             task->request_timeout_ms);
                sample->status_code = response.status_code;
                sample->timeout = response.timed_out;
                sample->latency_us = duration_us(begin);

                if(!http_ok)
                {
                    sample->error_reason = response.timed_out ? "login_timeout" : "login_network_error";
                    return false;
                }
                if(response.status_code != 200)
                {
                    sample->error_reason = "login_http_status_" + std::to_string(response.status_code);
                    return false;
                }

                if(!login_response.ParseFromString(response.body))
                {
                    sample->error_reason = "login_parse_error";
                    return false;
                }
                if(login_response.code() != 0)
                {
                    sample->error_reason = "login_code_" + std::to_string(login_response.code());
                    return false;
                }
            }
            else
            {
                sample->error_reason = register_error.empty() ? "register_failed" : register_error;
                return false;
            }
        }
        else
        {
            sample->error_reason = "login_code_" + std::to_string(login_response.code());
            return false;
        }
    }

    task->jwt_token = login_response.jwt();
    task->has_token = !task->jwt_token.empty();
    if(login_response.has_game_endpoint() && login_response.game_endpoint().port() > 0)
    {
        task->game_endpoint.host = login_response.game_endpoint().host();
        task->game_endpoint.port = static_cast<uint16_t>(login_response.game_endpoint().port());
    }
    if(!task->has_token)
    {
        sample->error_reason = "login_token_empty";
        return false;
    }

    sample->success = true;
    return true;
}

bool ClientWorker::do_enter_game(const ClientPressureTask& task, StageSample* sample) const
{
    if(sample == nullptr)
    {
        return false;
    }

    gateway::GameEnterRequest request;
    request.set_account(task.account);

    std::string request_data;
    request.SerializeToString(&request_data);

    std::vector<std::string> headers;
    if(task.has_token)
    {
        headers.emplace_back("Authorization: Bearer " + task.jwt_token);
    }

    auto begin = std::chrono::steady_clock::now();
    HttpResponse response;
    const bool http_ok = run_http_awaitable(http_post_async(task.game_endpoint,
                                                            "/v1/game/enter",
                                                            request_data,
                                                            task.request_timeout_ms,
                                                            headers),
                                            &response,
                                            task.request_timeout_ms);

    sample->latency_us = duration_us(begin);
    sample->status_code = response.status_code;
    sample->timeout = response.timed_out;

    if(!http_ok)
    {
        sample->error_reason = response.timed_out ? "game_timeout" : "game_network_error";
        return false;
    }
    if(response.status_code != 200)
    {
        sample->error_reason = "game_http_status_" + std::to_string(response.status_code);
        return false;
    }

    gateway::GameEnterResponse game_response;
    if(!game_response.ParseFromString(response.body))
    {
        sample->error_reason = "game_parse_error";
        return false;
    }
    if(game_response.code() != 0)
    {
        sample->error_reason = "game_code_" + std::to_string(game_response.code());
        return false;
    }

    sample->success = true;
    return true;
}
