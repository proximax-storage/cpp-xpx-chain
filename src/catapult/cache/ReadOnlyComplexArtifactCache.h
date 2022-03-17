/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ReadOnlyComplexCache.h"
#include "catapult/types.h"

namespace catapult { namespace cache {

	namespace {
		template<typename TCache, typename TCacheDelta, typename TDescriptor>
		struct ReadOnlyComplexArtifactCacheImpl {

			/// Creates a read-only overlay on top of \a cache.
			explicit ReadOnlyComplexArtifactCacheImpl(const TCache& cache)
					: m_pCache(&cache)
					, m_pCacheDelta(nullptr)
			{}

			/// Creates a read-only overlay on top of \a cache.
			explicit ReadOnlyComplexArtifactCacheImpl(const TCacheDelta& cache)
					: m_pCache(nullptr)
					, m_pCacheDelta(&cache)
			{}

			/// Find iterator returned by ReadOnlyArtifactCache::find.
			template<typename TCacheIterator, typename TCacheDeltaIterator>
			class ReadOnlyFindIterator {
			public:
				/// Creates an uninitialized iterator.
				ReadOnlyFindIterator() : m_hasCacheIter(false)
				{}

				/// Creates a find iterator around \a cacheIter.
				explicit ReadOnlyFindIterator(TCacheIterator&& cacheIter)
						: m_hasCacheIter(true)
						, m_cacheIter(std::move(cacheIter))
				{}

				/// Creates a find iterator around \a cacheDeltaIter.
				explicit ReadOnlyFindIterator(TCacheDeltaIterator&& cacheDeltaIter)
						: m_hasCacheIter(false)
						, m_cacheDeltaIter(std::move(cacheDeltaIter))
				{}

			public:
				/// Gets a const value.
				const typename TDescriptor::ValueType& get() const {
					return m_hasCacheIter ? m_cacheIter.get() : m_cacheDeltaIter.get();
				}

				/// Tries to get a const value.
				const typename TDescriptor::ValueType* tryGet() const {
					return m_hasCacheIter ? m_cacheIter.tryGet() : m_cacheDeltaIter.tryGet();
				}

			private:
				bool m_hasCacheIter;
				TCacheIterator m_cacheIter;
				TCacheDeltaIterator m_cacheDeltaIter;
			};


			auto find(typename TDescriptor::KeyType id) const {
				// note: having alias within function instead of at class scope allows forward declaration of caches
				using FindIterator = ReadOnlyFindIterator<decltype(m_pCache->find(id)), decltype(m_pCacheDelta->find(id))>;
				return m_pCache ? FindIterator(m_pCache->find(id)) : FindIterator(m_pCacheDelta->find(id));
			}

			/// Gets a value indicating whether or not an artifact with \a id is active at \a height.
			bool isActive(typename TDescriptor::KeyType id, Height height) const {
				return m_pCache ? m_pCache->isActive(id, height) : m_pCacheDelta->isActive(id, height);
			}

		protected:

			const TCache* m_pCache;
			const TCacheDelta* m_pCacheDelta;
		};
	}
	/// A read-only overlay on top of a cache that provides support for contains, get and isActive.
	/// Perf Note: Using complex cache will result in as many duplicated cache pointers as descriptors are provided!
	template<typename TCache, typename TCacheDelta, typename ...TDescriptors>
	class ReadOnlyComplexArtifactCache : public ReadOnlyComplexCache<TCache, TCacheDelta, TDescriptors...>,
	        public ReadOnlyComplexArtifactCacheImpl<TCache, TCacheDelta, TDescriptors>...{
	public:


	public:
		/// Creates a read-only overlay on top of \a cache.
		explicit ReadOnlyComplexArtifactCache(const TCache& cache)
				: ReadOnlyComplexCache<TCache, TCacheDelta, TDescriptors...>(cache)
				, ReadOnlyComplexArtifactCacheImpl<TCache, TCacheDelta, TDescriptors>(cache)...
		{}

		/// Creates a read-only overlay on top of \a cache.
		explicit ReadOnlyComplexArtifactCache(const TCacheDelta& cache)
				: ReadOnlyComplexCache<TCache, TCacheDelta, TDescriptors...>(cache)
				, ReadOnlyComplexArtifactCacheImpl<TCache, TCacheDelta, TDescriptors>(cache)...
		{}

	public:
		/// Finds the cache value identified by \a key.
		using ReadOnlyComplexArtifactCacheImpl<TCache, TCacheDelta, TDescriptors>::find...;
		using ReadOnlyComplexArtifactCacheImpl<TCache, TCacheDelta, TDescriptors>::isActive...;
	};
}}
