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

#include "cached_account_repository.h"

#include "log/glogger.h"

#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <iomanip>
#include <sstream>

namespace
{

class CachedAccountRepositoryManager : public CoroManager
{
public:
    explicit CachedAccountRepositoryManager(int worker_count)
        : CoroManager(std::max(1, worker_count))
    {
        init();
    }
    ~CachedAccountRepositoryManager() override = default;

public:
    CoroResult* alloc() override
    {
        expand<AccountRepositoryOpResult>();
        return inner_alloc();
    }
};

} // namespace

namespace
{

std::string bytes_to_hex(const unsigned char* data, size_t len)
{
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for(size_t i = 0; i < len; ++i)
    {
        output << std::setw(2) << static_cast<int>(data[i]);
    }
    return output.str();
}

std::string make_password_hash(const std::string& password, const std::string& salt, int iterations)
{
    std::array<unsigned char, 32> digest{};
    const int rounds = std::max(1, iterations);
    const int rc = PKCS5_PBKDF2_HMAC(password.c_str(),
                                     static_cast<int>(password.size()),
                                     reinterpret_cast<const unsigned char*>(salt.data()),
                                     static_cast<int>(salt.size()),
                                     rounds,
                                     EVP_sha256(),
                                     static_cast<int>(digest.size()),
                                     digest.data());
    if(rc != 1)
    {
        return {};
    }
    return bytes_to_hex(digest.data(), digest.size());
}

std::string sha256_hex(const std::string& input)
{
    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_len = 0;

    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if(context == nullptr)
    {
        return {};
    }

    const bool ok = EVP_DigestInit_ex(context, EVP_sha256(), nullptr) == 1 &&
                    EVP_DigestUpdate(context, input.data(), input.size()) == 1 &&
                    EVP_DigestFinal_ex(context, digest.data(), &digest_len) == 1;
    EVP_MD_CTX_free(context);
    if(!ok)
    {
        return {};
    }
    return bytes_to_hex(digest.data(), digest_len);
}

bool verify_password_by_record(const std::string& password,
                               const AccountRecord& record,
                               int password_hash_iterations)
{
    const int normalized_iterations = std::max(1, password_hash_iterations);
    const auto expected_hash = make_password_hash(password, record.salt, normalized_iterations);
    if(!expected_hash.empty() && record.password_hash == expected_hash)
    {
        return true;
    }

    if(normalized_iterations != 20000)
    {
        const auto fast_hash = make_password_hash(password, record.salt, 20000);
        if(!fast_hash.empty() && record.password_hash == fast_hash)
        {
            return true;
        }
    }

    const auto legacy_hash = sha256_hex(record.salt + ":" + password);
    if(!legacy_hash.empty() && record.password_hash == legacy_hash)
    {
        return true;
    }

    return record.salt.empty() && record.password_hash == password;
}

} // namespace

class CachedAccountRepository::CachedAccountRepoManager : public CachedAccountRepositoryManager
{
public:
    explicit CachedAccountRepoManager(int worker_count)
        : CachedAccountRepositoryManager(worker_count)
    {
    }
};

CachedAccountRepository::CachedAccountRepository(std::shared_ptr<IAccountRepository> inner,
                                                 std::shared_ptr<IAccountCacheStore> cache_store,
                                                 int cache_ttl_sec,
                                                 int coro_workers,
                                                 int password_hash_iterations)
    : m_inner(std::move(inner)),
      m_cache_store(std::move(cache_store)),
      m_cache_ttl_sec(std::max(1, cache_ttl_sec)),
      m_coro_workers(std::max(1, coro_workers)),
      m_password_hash_iterations(std::max(1, password_hash_iterations))
{
    m_manager = std::make_unique<CachedAccountRepoManager>(m_coro_workers);
}

CachedAccountRepository::~CachedAccountRepository() = default;

bool CachedAccountRepository::ready() const
{
    return m_inner != nullptr && m_inner->ready();
}

void CachedAccountRepository::poll()
{
    if(m_manager)
    {
        m_manager->update();
    }
}

CoroAwaitable CachedAccountRepository::find_account(const std::string& account)
{
    auto* result = dynamic_cast<AccountRepositoryOpResult*>(m_manager->alloc());
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(AccountRepositoryOpType::find_account,
                 account,
                 "",
                 [this](AccountRepositoryOpResult* op) {
                     if(op == nullptr)
                     {
                        return;
                     }

                     if(m_inner == nullptr)
                     {
                         op->success = false;
                         op->error = "account_repository_unavailable";
                         return;
                     }

                     if(m_cache_store != nullptr)
                     {
                         AccountCacheOpResult cache_result;
                         bool cache_wait_ok = false;
                         {
                             std::lock_guard<std::mutex> wait_lock(m_wait_mutex);
                             cache_wait_ok = wait_account_cache_store_result(m_cache_store.get(),
                                                                             m_cache_store->get_account(op->request_account),
                                                                             &cache_result,
                                                                             1000);
                         }
                         if(cache_wait_ok &&
                            cache_result.success && cache_result.hit && cache_result.record.has_value())
                         {
                             op->success = true;
                             op->record = cache_result.record;
                             return;
                         }
                     }

                     AccountRepositoryOpResult inner_result;
                     bool ok = false;
                     {
                         std::lock_guard<std::mutex> wait_lock(m_wait_mutex);
                         ok = wait_account_repository_result(m_inner.get(),
                                                             m_inner->find_account(op->request_account),
                                                             &inner_result,
                                                             1500);
                     }
                     if(!ok)
                     {
                         op->success = false;
                         op->error = "account_repository_timeout";
                         return;
                     }

                     op->success = inner_result.success;
                     op->error = inner_result.error;
                     op->record = inner_result.record;
                     if(op->success && op->record.has_value() && m_cache_store != nullptr)
                     {
                         AccountCacheOpResult cache_put_result;
                         bool put_ok = false;
                         {
                             std::lock_guard<std::mutex> wait_lock(m_wait_mutex);
                             put_ok = wait_account_cache_store_result(m_cache_store.get(),
                                                                      m_cache_store->put_account(*op->record, m_cache_ttl_sec),
                                                                      &cache_put_result,
                                                                      1000);
                         }
                         if(!(put_ok && cache_put_result.success))
                         {
                             spdlog::warn("cache find_account put failed: {}",
                                          put_ok ? cache_put_result.error : "wait_timeout");
                         }
                     }
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable CachedAccountRepository::verify_password(const std::string& account, const std::string& password)
{
    auto* result = dynamic_cast<AccountRepositoryOpResult*>(m_manager->alloc());
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(AccountRepositoryOpType::verify_password,
                 account,
                 password,
                 [this](AccountRepositoryOpResult* op) {
                     if(op == nullptr)
                     {
                         return;
                     }

                     if(m_inner == nullptr)
                     {
                         op->success = false;
                         op->error = "account_repository_unavailable";
                         return;
                     }

                     if(m_cache_store != nullptr)
                     {
                         AccountCacheOpResult cache_result;
                         bool cache_wait_ok = false;
                         {
                             std::lock_guard<std::mutex> wait_lock(m_wait_mutex);
                             cache_wait_ok = wait_account_cache_store_result(m_cache_store.get(),
                                                                             m_cache_store->get_account(op->request_account),
                                                                             &cache_result,
                                                                             1000);
                         }
                         if(cache_wait_ok &&
                            cache_result.success && cache_result.hit && cache_result.record.has_value())
                         {
                             op->success = true;
                             op->record = cache_result.record;
                             op->password_ok = verify_password_by_record(op->request_password,
                                                                         *cache_result.record,
                                                                         m_password_hash_iterations);
                             if(op->password_ok)
                             {
                                 return;
                             }
                         }
                     }

                     AccountRepositoryOpResult inner_result;
                     bool ok = false;
                     {
                         std::lock_guard<std::mutex> wait_lock(m_wait_mutex);
                         ok = wait_account_repository_result(m_inner.get(),
                                                             m_inner->verify_password(op->request_account, op->request_password),
                                                             &inner_result,
                                                             1500);
                     }
                     if(!ok)
                     {
                         op->success = false;
                         op->error = "account_repository_timeout";
                         return;
                     }

                     op->success = inner_result.success;
                     op->error = inner_result.error;
                     op->password_ok = inner_result.password_ok;
                     op->record = inner_result.record;

                     if(op->success && op->record.has_value() && m_cache_store != nullptr)
                     {
                         AccountCacheOpResult cache_put_result;
                         bool put_ok = false;
                         {
                             std::lock_guard<std::mutex> wait_lock(m_wait_mutex);
                             put_ok = wait_account_cache_store_result(m_cache_store.get(),
                                                                      m_cache_store->put_account(*op->record, m_cache_ttl_sec),
                                                                      &cache_put_result,
                                                                      1000);
                         }
                         if(!(put_ok && cache_put_result.success))
                         {
                             spdlog::warn("cache verify_password put failed: {}",
                                          put_ok ? cache_put_result.error : "wait_timeout");
                         }
                     }
                 });
    return CoroAwaitable{m_manager.get(), result};
}

CoroAwaitable CachedAccountRepository::create_account(const std::string& account, const std::string& password)
{
    auto* result = dynamic_cast<AccountRepositoryOpResult*>(m_manager->alloc());
    if(result == nullptr)
    {
        return CoroAwaitable{m_manager.get(), nullptr};
    }

    result->init(AccountRepositoryOpType::create_account,
                 account,
                 password,
                 [this](AccountRepositoryOpResult* op) {
                     if(op == nullptr)
                     {
                         return;
                     }

                     if(m_inner == nullptr)
                     {
                         op->success = false;
                         op->error = "account_repository_unavailable";
                         return;
                     }

                     AccountRepositoryOpResult inner_result;
                     bool ok = false;
                     {
                         std::lock_guard<std::mutex> wait_lock(m_wait_mutex);
                         ok = wait_account_repository_result(m_inner.get(),
                                                             m_inner->create_account(op->request_account, op->request_password),
                                                             &inner_result,
                                                             1500);
                     }
                     if(!ok)
                     {
                         op->success = false;
                         op->error = "account_repository_timeout";
                         return;
                     }

                     op->success = inner_result.success;
                     op->error = inner_result.error;
                     op->create_ok = inner_result.create_ok;
                     op->record = inner_result.record;

                     if(m_cache_store != nullptr)
                     {
                         AccountCacheOpResult erase_result;
                         bool erase_ok = false;
                         {
                             std::lock_guard<std::mutex> wait_lock(m_wait_mutex);
                             erase_ok = wait_account_cache_store_result(m_cache_store.get(),
                                                                         m_cache_store->erase_account(op->request_account),
                                                                         &erase_result,
                                                                         1000);
                         }
                         if(!(erase_ok && erase_result.success))
                         {
                             spdlog::warn("cache create_account erase failed: {}",
                                          erase_ok ? erase_result.error : "wait_timeout");
                         }
                     }
                 });
    return CoroAwaitable{m_manager.get(), result};
}
