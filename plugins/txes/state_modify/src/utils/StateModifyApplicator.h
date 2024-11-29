/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "boost/core/span.hpp"
#include "catapult/cache/CacheConstants.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "src/cache/NamespaceCache.h"
#include "src/cache/MetadataV1Cache.h"

namespace catapult { namespace utils {

	namespace detail {
		template<typename TCache, typename TSerializer>
		struct CacheConversionFunctions {
			static void Insert(typename TCache::CacheDeltaType& delta, const typename TCache::CacheValueType& value) {
				delta.insert(value);
			}
			static void ApplyRecord(
					cache::CatapultCacheDelta& cacheDelta,
					const boost::span<const uint8_t>& key,
					const boost::span<const uint8_t>& value) {
				auto& localCacheDelta = cacheDelta.sub<TCache>();
				auto stateObject = TCache::Descriptor::Serializer::DeserializeValue(value);
				auto iterator = localCacheDelta.find(TCache::Descriptor::GetKeyFromValue(stateObject));
				if (iterator.tryGet() != nullptr) {
					iterator.get() = stateObject;
				} else {
					Insert(localCacheDelta, stateObject);
				}
			}
		};

		template<>
		void CacheConversionFunctions<cache::AccountStateCache>::Insert(
				cache::AccountStateCacheDelta& delta,
				const state::AccountState& value) {
			delta.addAccount(value);
		}

		template<>
		void CacheConversionFunctions<cache::NamespaceCache, cache::RootNamespaceHistoryPrimarySerializer>::ApplyRecord(
				cache::CatapultCacheDelta& cacheDelta,
				const boost::span<const uint8_t>& key,
				const boost::span<const uint8_t>& value) {
			auto& localCacheDelta = cacheDelta.sub<cache::NamespaceCache>();
			auto stateObject = cache::RootNamespaceHistoryPrimarySerializer::DeserializeValue(value);
			auto iterator = localCacheDelta.mfind(stateObject.id(), Tag<cache::NamespaceCacheTypes::PrimaryTypes::BaseSetDeltaType>());
			if (iterator.tryGet() != nullptr) {
				iterator.get() = stateObject;
			} else {
				localCacheDelta.insertRaw(stateObject);
			}
		}
		template<>
		void CacheConversionFunctions<cache::NamespaceCache, cache::NamespaceFlatMapTypesSerializer>::ApplyRecord(
				cache::CatapultCacheDelta& cacheDelta,
				const boost::span<const uint8_t>& key,
				const boost::span<const uint8_t>& value) {
			auto& localCacheDelta = cacheDelta.sub<cache::NamespaceCache>();
			auto stateObject = cache::NamespaceFlatMapTypesSerializer::DeserializeValue(value);
			auto iterator = localCacheDelta.mfind(stateObject.id(), Tag<cache::NamespaceCacheTypes::FlatMapTypes::BaseSetDeltaType>());
			if (iterator.tryGet() != nullptr) {
				iterator.get() = stateObject;
			} else {
				localCacheDelta.insertRaw(stateObject);
			}
		}
		template<>
		void CacheConversionFunctions<cache::NamespaceCache, cache::NamespaceHeightGroupingSerializer>::ApplyRecord(
				cache::CatapultCacheDelta& cacheDelta,
				const boost::span<const uint8_t>& key,
				const boost::span<const uint8_t>& value) {
			auto& localCacheDelta = cacheDelta.sub<cache::NamespaceCache>();
			auto stateObject = cache::NamespaceHeightGroupingSerializer::DeserializeValue(value);
			auto height = stateObject.key();
			auto iterator = localCacheDelta.mfind(height);
			if (iterator.tryGet() != nullptr) {
				iterator.get() = stateObject;
			} else {
				localCacheDelta.insertRaw(stateObject);
			}
		}
		template<>
		void CacheConversionFunctions<cache::MetadataV1Cache, state::MetadataV1Serializer>::ApplyRecord(
				cache::CatapultCacheDelta& cacheDelta,
				const boost::span<const uint8_t>& key,
				const boost::span<const uint8_t>& value) {
			auto& localCacheDelta = cacheDelta.sub<cache::MetadataV1Cache>();
			auto stateObject = cache::MetadataV1PrimarySerializer::DeserializeValue(value);
			auto iterator = localCacheDelta.mfind(stateObject.metadataId());
			if (iterator.tryGet() != nullptr) {
				iterator.get() = stateObject;
			} else {
				localCacheDelta.insert(stateObject);
			}
		}
	}
	class StateModifyApplicator {
	public:
		StateModifyApplicator(cache::CatapultCacheDelta& delta) : cache(delta) {

		}
	public:
		template<typename TCache, typename TSerializer = typename TCache::Descriptor::Serializer>
		void ApplyRecord(const boost::span<const uint8_t>& key, const boost::span<const uint8_t>& value){
			detail::CacheConversionFunctions<TCache, TSerializer>::ApplyRecord(cache, key, value);
		}

	private:
		// Catapult cache reference
		cache::CatapultCacheDelta& cache;

	};
}}
