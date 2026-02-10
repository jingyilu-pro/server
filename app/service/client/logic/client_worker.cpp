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

#include "protocol/gateway.pb.h"

#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <sstream>
#include <string>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace
{

struct HttpResponse
{
    int status_code = 0;
    std::string body;
    bool timed_out = false;
    bool ok = false;
};

int64_t duration_us(std::chrono::steady_clock::time_point begin)
{
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now - begin).count();
}

std::string endpoint_path(const char* path)
{
    return std::string(path == nullptr ? "/" : path);
}

bool wait_socket_ready(int fd, int timeout_ms, bool read_wait)
{
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    int result = read_wait ? select(fd + 1, &fds, nullptr, nullptr, &timeout)
                           : select(fd + 1, nullptr, &fds, nullptr, &timeout);
    return result > 0;
}

bool set_non_blocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if(flags < 0)
    {
        return false;
    }
    if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        return false;
    }
    return true;
}

bool set_blocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if(flags < 0)
    {
        return false;
    }
    if(fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) < 0)
    {
        return false;
    }
    return true;
}

bool connect_with_timeout(int fd, const sockaddr* addr, socklen_t addr_len, int timeout_ms, bool* timed_out)
{
    if(timed_out == nullptr)
    {
        return false;
    }
    *timed_out = false;

    if(!set_non_blocking(fd))
    {
        return false;
    }

    int result = connect(fd, addr, addr_len);
    if(result == 0)
    {
        set_blocking(fd);
        return true;
    }
    if(errno != EINPROGRESS)
    {
        return false;
    }

    if(!wait_socket_ready(fd, timeout_ms, false))
    {
        *timed_out = true;
        return false;
    }

    int so_error = 0;
    socklen_t so_error_len = sizeof(so_error);
    if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &so_error_len) != 0)
    {
        return false;
    }
    if(so_error != 0)
    {
        errno = so_error;
        return false;
    }

    set_blocking(fd);
    return true;
}

bool send_all(int fd, const std::string& data, int timeout_ms)
{
    size_t sent = 0;
    while(sent < data.size())
    {
        if(!wait_socket_ready(fd, timeout_ms, false))
        {
            return false;
        }

        ssize_t n = send(fd, data.data() + sent, data.size() - sent, 0);
        if(n <= 0)
        {
            return false;
        }
        sent += static_cast<size_t>(n);
    }
    return true;
}

bool recv_until_close(int fd, std::string* out, int timeout_ms, bool* timeout)
{
    if(out == nullptr || timeout == nullptr)
    {
        return false;
    }

    constexpr size_t kBufferSize = 8192;
    char buffer[kBufferSize];

    while(true)
    {
        if(!wait_socket_ready(fd, timeout_ms, true))
        {
            *timeout = true;
            return false;
        }

        ssize_t n = recv(fd, buffer, kBufferSize, 0);
        if(n < 0)
        {
            return false;
        }
        if(n == 0)
        {
            break;
        }
        out->append(buffer, static_cast<size_t>(n));
    }

    return true;
}

HttpResponse http_post(const EndpointConfig& endpoint,
                       const char* path,
                       const std::string& protobuf_body,
                       int timeout_ms,
                       const std::vector<std::string>& headers)
{
    HttpResponse response;

    int fd = -1;

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    const std::string port_text = std::to_string(endpoint.port);
    if(getaddrinfo(endpoint.host.c_str(), port_text.c_str(), &hints, &result) != 0)
    {
        return response;
    }

    bool connected = false;
    bool connect_timeout = false;
    for(addrinfo* current = result; current != nullptr; current = current->ai_next)
    {
        fd = socket(current->ai_family, current->ai_socktype, current->ai_protocol);
        if(fd < 0)
        {
            continue;
        }

        if(connect_with_timeout(fd, current->ai_addr, static_cast<socklen_t>(current->ai_addrlen), timeout_ms, &connect_timeout))
        {
            connected = true;
            break;
        }

        close(fd);
        fd = -1;
    }
    freeaddrinfo(result);

    if(!connected || fd < 0)
    {
        response.timed_out = connect_timeout;
        return response;
    }

    std::ostringstream request;
    request << "POST " << endpoint_path(path) << " HTTP/1.1\r\n";
    request << "Host: " << endpoint.host << ":" << endpoint.port << "\r\n";
    request << "Connection: close\r\n";
    request << "Content-Type: application/x-protobuf\r\n";
    for(const auto& header : headers)
    {
        request << header << "\r\n";
    }
    request << "Content-Length: " << protobuf_body.size() << "\r\n\r\n";
    request << protobuf_body;

    const auto request_text = request.str();
    if(!send_all(fd, request_text, timeout_ms))
    {
        close(fd);
        return response;
    }

    std::string raw_response;
    bool timeout = false;
    if(!recv_until_close(fd, &raw_response, timeout_ms, &timeout))
    {
        response.timed_out = timeout;
        close(fd);
        return response;
    }

    close(fd);

    auto head_end = raw_response.find("\r\n\r\n");
    if(head_end == std::string::npos)
    {
        return response;
    }

    auto status_end = raw_response.find("\r\n");
    if(status_end == std::string::npos)
    {
        return response;
    }

    auto status_line = raw_response.substr(0, status_end);
    {
        std::istringstream status_stream(status_line);
        std::string http_version;
        status_stream >> http_version;
        status_stream >> response.status_code;
    }

    response.body = raw_response.substr(head_end + 4);
    response.ok = (response.status_code > 0);
    return response;
}

} // namespace

WorkerCycleResult ClientWorker::run(ClientPressureTask* task) const
{
    WorkerCycleResult result;
    if(task == nullptr)
    {
        result.failure_reason = "null_task";
        return result;
    }

    auto begin = std::chrono::steady_clock::now();

    StageSample manager_sample;
    manager_sample.stage = StageType::manager;
    if(!do_manager_route(*task, &manager_sample))
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

bool ClientWorker::do_manager_route(const ClientPressureTask& task, StageSample* sample) const
{
    if(sample == nullptr)
    {
        return false;
    }

    gateway::RouteLoginRequest request;
    request.set_client_version("pressure_client_v1");

    std::string request_data;
    request.SerializeToString(&request_data);

    auto begin = std::chrono::steady_clock::now();
    auto response = http_post(task.manager_endpoint,
                              "/v1/route/login",
                              request_data,
                              task.request_timeout_ms,
                              {});

    sample->latency_us = duration_us(begin);
    sample->status_code = response.status_code;
    sample->timeout = response.timed_out;

    if(!response.ok)
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

    sample->success = true;
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
    auto response = http_post(task->login_endpoint,
                              "/v1/auth/login",
                              request_data,
                              task->request_timeout_ms,
                              {});

    sample->latency_us = duration_us(begin);
    sample->status_code = response.status_code;
    sample->timeout = response.timed_out;

    if(!response.ok)
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
        sample->error_reason = "login_code_" + std::to_string(login_response.code());
        return false;
    }

    task->jwt_token = login_response.jwt();
    task->has_token = !task->jwt_token.empty();
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
    auto response = http_post(task.game_endpoint,
                              "/v1/game/enter",
                              request_data,
                              task.request_timeout_ms,
                              headers);

    sample->latency_us = duration_us(begin);
    sample->status_code = response.status_code;
    sample->timeout = response.timed_out;

    if(!response.ok)
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
