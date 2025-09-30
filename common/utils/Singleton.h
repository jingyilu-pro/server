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



#include <mutex>


static std::mutex singleton_mutex;

template < typename T>
class Singleton
{
public:
	Singleton(const Singleton&) = delete;
	const Singleton & operator= (const Singleton &) = delete;
	Singleton(void) = default;
	~Singleton(void) = default;

	static inline T* instance(void) {
        if(!singleton_) {
			std::unique_lock<std::mutex> lk(singleton_mutex);
			if(!singleton_) {
            	singleton_ = new T();
			}
        }
		return singleton_;
	}

private:
	static T* singleton_;
};

template <typename T>
T* Singleton<T>::singleton_ = nullptr;
