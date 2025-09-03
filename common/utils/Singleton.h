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
