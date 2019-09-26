#ifndef CATAPULT_SERVER_ATOMICVECTOR_H
#define CATAPULT_SERVER_ATOMICVECTOR_H

#include <vector>
#include <mutex>

namespace catapult { namespace test {

	template<typename T>
	struct AtomicVector : public std::vector<T> {
		using base = std::vector<T>;
		using base::base; // inherit constructors
		using base::begin;
		using base::end;

		AtomicVector(const AtomicVector& v) : base(v)
		{}

		AtomicVector(AtomicVector&& v) : base(v)
		{}

		// we don't care about overloads, just throw all args to the underlying method
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

		auto size() const {
			std::lock_guard<std::mutex> lock(Mutex);
			return base::size();
		}

		mutable std::mutex Mutex;
	};

	template<typename T>
	bool operator==(const AtomicVector<T>& lhs, const AtomicVector<T>& rhs) {
		if (lhs.size() != rhs.size())
			return false;

		std::lock_guard<std::mutex> lock1(lhs.Mutex);
		std::lock_guard<std::mutex> lock2(rhs.Mutex);

		return std::equal(lhs.begin(), lhs.end(), rhs.begin());
	}

}} // namespace catapult::test

#endif // CATAPULT_SERVER_ATOMICVECTOR_H
