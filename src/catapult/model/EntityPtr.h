#ifndef CATAPULT_SERVER_ENTITYPTR_H
#define CATAPULT_SERVER_ENTITYPTR_H

#include <memory>

namespace catapult { namespace model {

	/**
	 * Custom deleter type for Entity classes, allocated with MakeUniqueWithSize.
	 */
	template <typename T>
	struct UniqueEntityPtrDeleter {
		void operator()(T* t) {
			::operator delete(const_cast<void*>(reinterpret_cast<const void*>(t)));
		}

		// implicit cast operator, which enables implicit transformations from
		// unique_ptr<T, UniqueEntityPtrDeleter<T>> to unique_ptr<V, UniqueEntityPtrDeleter<V>>
		template <typename V>
		operator UniqueEntityPtrDeleter<V>(){
			return UniqueEntityPtrDeleter<V>();
		}
	};

	/**
	 * Alias to unique ptr with custom deleter.
	 */
	template <typename T>
	using UniqueEntityPtr = std::unique_ptr<T, UniqueEntityPtrDeleter<T>>;

}}

#endif //CATAPULT_SERVER_ENTITYPTR_H
