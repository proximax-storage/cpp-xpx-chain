#ifndef CATAPULT_SERVER_ATOMICVECTOR_H
#define CATAPULT_SERVER_ATOMICVECTOR_H

#include <vector>
#include <mutex>

namespace catapult { namespace test {

	template<typename T>
	struct AtomicVector : public std::vector<T> {
		using base = std::vector<T>;
		using base::base; // inherit constructors

		template<typename... Args>
		auto push_back(Args&&... args) {
			std::lock_guard<std::mutex> lock(Mutex);
			return base::push_back(std::forward<decltype(args)...>(args...));
		}

		template<typename... Args>
		auto emplace_back(Args&&... args) {
			std::lock_guard<std::mutex> lock(Mutex);
			return base::emplace_back(std::forward<decltype(args)...>(args...));
		}

		template<typename... Args>
		auto emplace(Args&&... args) {
			std::lock_guard<std::mutex> lock(Mutex);
			return base::emplace(std::forward<decltype(args)...>(args...));
		}


		auto size() {
			std::lock_guard<std::mutex> lock(Mutex);
			return base::size();
		}

		std::mutex Mutex;
	};

}} // namespace catapult::test

#endif // CATAPULT_SERVER_ATOMICVECTOR_H
