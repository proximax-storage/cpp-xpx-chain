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
#include "CacheChangesSerializer.h"
#include "CacheChangesStorage.h"
#include "catapult/utils/Functional.h"

namespace catapult { namespace cache {

	/// CacheChangesStorage implementation that wraps a cache and associated storage traits.
	template<typename TCache, typename TStorageTraits>
	class CacheChangesStorageAdapter : public CacheChangesStorage {
	public:
		/// Creates an adapter around \a cache.
		explicit CacheChangesStorageAdapter(TCache& cache) : m_cache(cache)
		{}

	public:
		void saveAll(const CacheChanges& changes, io::OutputStream& output) const override {
			WriteCacheChanges<TStorageTraits>(output, changes.sub<TCache>());
		}

		std::unique_ptr<const MemoryCacheChanges> loadAll(io::InputStream& input) const override {
			auto pMemoryCacheChanges = std::make_unique<MemoryCacheChangesT<typename TCache::CacheValueType>>();
			ReadCacheChanges<TStorageTraits>(input, *pMemoryCacheChanges);
			return std::move(pMemoryCacheChanges);
		}

		void apply(const CacheChanges& changes, const Height& height) const override {
			auto delta = m_cache.createDelta(height);

			auto subCacheChanges = changes.sub<TCache>();
			for (const auto* pAdded : subCacheChanges.addedElements()) {
				TStorageTraits::Purge(*pAdded, *delta);
				TStorageTraits::LoadInto(*pAdded, *delta);
			}

			for (const auto* pModified : subCacheChanges.modifiedElements()) {
				TStorageTraits::Purge(*pModified, *delta);
				TStorageTraits::LoadInto(*pModified, *delta);
			}

			for (const auto* pRemoved : subCacheChanges.removedElements())
				TStorageTraits::Purge(*pRemoved, *delta);

			m_cache.commit();
		}

	private:
		TCache& m_cache;
	};

	template<typename TCache, typename TStorageTraits>
	class MultisetCacheChangesStorageAdapter : public CacheChangesStorage {
	public:
		/// Creates an adapter around \a cache.
		explicit MultisetCacheChangesStorageAdapter(TCache& cache) : m_cache(cache)
		{}

	public:
		void saveAll(const CacheChanges& changes, io::OutputStream& output) const override {
			WriteCacheChanges<TStorageTraits>(output, changes.sub<TCache>());
		}

		std::unique_ptr<const MemoryCacheChanges> loadAll(io::InputStream& input) const override {
			auto pMemoryCacheChanges = std::make_unique<MemoryCacheChangesT<typename TCache::CacheValueType>>();
			ReadCacheChanges<TStorageTraits>(input, *pMemoryCacheChanges);
			return std::move(pMemoryCacheChanges);
		}

		void apply(const CacheChanges& changes, const Height& height) const override {
			auto delta = m_cache.createDelta(height);

			auto subCacheChanges = changes.sub<TCache>();
			utils::for_sequence(std::make_index_sequence<std::tuple_size_v<typename TCache::CacheValueTypes>>{}, [&](auto i){
				for (const auto* pAdded : std::get<i>(subCacheChanges.addedElements())) {
					TStorageTraits::Purge<i>(*pAdded, *delta);
					TStorageTraits::LoadInto<i>(*pAdded, *delta);
				}

				for (const auto* pModified : std::get<i>(subCacheChanges.modifiedElements())) {
					TStorageTraits::Purge<i>(*pModified, *delta);
					TStorageTraits::LoadInto<i>(*pModified, *delta);
				}

				for (const auto* pRemoved : std::get<i>(subCacheChanges.removedElements()))
					TStorageTraits::Purge<i>(*pRemoved, *delta);
			});
			m_cache.commit();
		}

	private:
		TCache& m_cache;
	};
}}
