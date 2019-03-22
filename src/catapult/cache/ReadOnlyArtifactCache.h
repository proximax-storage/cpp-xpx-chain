/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include "ReadOnlySimpleCache.h"
#include "catapult/types.h"

namespace catapult { namespace cache {

	/// A read-only overlay on top of a cache that provides support for contains, get and isActive.
	template<typename TCache, typename TCacheDelta, typename TKey, typename TValue>
	class ReadOnlyArtifactCache : public ReadOnlySimpleCache<TCache, TCacheDelta, TKey> {
	public:
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
			const TValue& get() const {
				return m_hasCacheIter ? m_cacheIter.get() : m_cacheDeltaIter.get();
			}

			/// Tries to get a const value.
			const TValue* tryGet() const {
				return m_hasCacheIter ? m_cacheIter.tryGet() : m_cacheDeltaIter.tryGet();
			}

		private:
			bool m_hasCacheIter;
			TCacheIterator m_cacheIter;
			TCacheDeltaIterator m_cacheDeltaIter;
		};

	public:
		/// Creates a read-only overlay on top of \a cache.
		explicit ReadOnlyArtifactCache(const TCache& cache)
				: ReadOnlySimpleCache<TCache, TCacheDelta, TKey>(cache)
				, m_pCache(&cache)
				, m_pCacheDelta(nullptr)
		{}

		/// Creates a read-only overlay on top of \a cache.
		explicit ReadOnlyArtifactCache(const TCacheDelta& cache)
				: ReadOnlySimpleCache<TCache, TCacheDelta, TKey>(cache)
				, m_pCache(nullptr)
				, m_pCacheDelta(&cache)
		{}

	public:
		/// Finds the cache value identified by \a key.
		auto find(TKey id) const {
			// note: having alias within function instead of at class scope allows forward declaration of caches
			using FindIterator = ReadOnlyFindIterator<decltype(m_pCache->find(id)), decltype(m_pCacheDelta->find(id))>;
			return m_pCache ? FindIterator(m_pCache->find(id)) : FindIterator(m_pCacheDelta->find(id));
		}

		/// Gets a value indicating whether or not an artifact with \a id is active at \a height.
		bool isActive(TKey id, Height height) const {
			return m_pCache ? m_pCache->isActive(id, height) : m_pCacheDelta->isActive(id, height);
		}

	private:
		const TCache* m_pCache;
		const TCacheDelta* m_pCacheDelta;
	};
}}
