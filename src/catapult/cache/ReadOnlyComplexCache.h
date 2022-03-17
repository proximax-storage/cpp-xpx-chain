/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <stddef.h>

namespace catapult { namespace cache {

	namespace {
		template<typename TCache, typename TCacheDelta, typename TDescriptor>
		class ReadOnlyComplexCacheImpl {
		public:

			/// Creates a read-only overlay on top of \a cache.
			explicit ReadOnlyComplexCacheImpl(const TCache& cache)
					: m_pCache(&cache)
					, m_pCacheDelta(nullptr)
			{}

			/// Creates a read-only overlay on top of \a cache.
			explicit ReadOnlyComplexCacheImpl(const TCacheDelta& cache)
					: m_pCache(nullptr)
					, m_pCacheDelta(&cache)
			{}

		public:
			/// Searches for the given \a key in the cache.
			/// Returns \c true if it is found or \c false otherwise.
			bool contains(const typename TDescriptor::KeyType& key) const {
				return m_pCache ? m_pCache->contains(key) : m_pCacheDelta->contains(key);
			}




		private:
			const TCache* m_pCache;
			const TCacheDelta* m_pCacheDelta;
		};
	}
	/// A read-only overlay on top of a cache that provides support for contains for multiple sets.
	template<typename TCache, typename TCacheDelta, typename ...TDescriptors>
	class ReadOnlyComplexCache : public ReadOnlyComplexCacheImpl<TCache, TCacheDelta, TDescriptors>... {
	public:
		/// Creates a read-only overlay on top of \a cache.
		explicit ReadOnlyComplexCache(const TCache& cache)
				: ReadOnlyComplexCacheImpl<TCache, TCacheDelta, TDescriptors>(cache)...
				, m_pCache(&cache)
				, m_pCacheDelta(nullptr)
		{}

		/// Creates a read-only overlay on top of \a cache.
		explicit ReadOnlyComplexCache(const TCacheDelta& cache)
				: ReadOnlyComplexCacheImpl<TCache, TCacheDelta, TDescriptors>(cache)...
				, m_pCache(nullptr)
				, m_pCacheDelta(&cache)
		{}

	public:
		/// Returns the number of elements in the cache.
		size_t size() const {
			return m_pCache ? m_pCache->size() : m_pCacheDelta->size();
		}

		using ReadOnlyComplexCacheImpl<TCache, TCacheDelta, TDescriptors>::contains...;

	private:
		const TCache* m_pCache;
		const TCacheDelta* m_pCacheDelta;

	};
}}
