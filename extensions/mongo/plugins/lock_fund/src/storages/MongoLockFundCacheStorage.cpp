/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/LockFundCache.h"
#include "MongoLockFundCacheStorage.h"
#include "src/mappers/LockFundRecordMapper.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "plugins/txes/lock_fund/src/cache/LockFundCache.h"
#include "plugins/txes/lock_fund/src/state/LockFundRecordGroup.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		template<int TIndex, typename TParam, typename TReturn>
		TReturn MapToMongoIdImpl(const TParam& id, std::integral_constant<int, TIndex> = {});
		template<>
		int64_t MapToMongoIdImpl<0>(const Height& id, std::integral_constant<int, 0>) {
			return mappers::ToInt64(id);
		}
		template<>
		bsoncxx::types::b_binary MapToMongoIdImpl<1>(const Key& id, std::integral_constant<int, 1>) {
			return mappers::ToBinary(id);
		}

		using FreeModelTypes = std::tuple<state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>, state::LockFundRecordGroup<state::LockFundKeyIndexDescriptor>>;

		template<int TIndex, typename TParam>
		bsoncxx::document::value MapToMongoDocumentImpl(const TParam& model, model::NetworkIdentifier networkIdentifier, std::integral_constant<int, TIndex> = {});
		template<>
		bsoncxx::document::value MapToMongoDocumentImpl<0, std::tuple_element<0, FreeModelTypes>::type>(const typename std::tuple_element<0, FreeModelTypes>::type& record, model::NetworkIdentifier networkIdentifier, std::integral_constant<int, 0>) {
			return plugins::ToDbModel<state::LockFundHeightIndexDescriptor>(record);
		}
		template<>
		bsoncxx::document::value MapToMongoDocumentImpl<1, std::tuple_element<1, FreeModelTypes>::type>(const typename std::tuple_element<1, FreeModelTypes>::type& record, model::NetworkIdentifier networkIdentifier, std::integral_constant<int, 1>) {
			return plugins::ToDbModel<state::LockFundKeyIndexDescriptor>(record);
		}
		struct LockFundCacheTraits {
			using CacheType = cache::LockFundCache;
			using CacheDeltaType = cache::LockFundCacheDelta;
			using KeyTypes = std::tuple<Height, Key>;
			using ModelTypes = FreeModelTypes;
			using MongoKeyTypes = std::tuple<int64_t, bsoncxx::types::b_binary>;
			using IdContainerType = std::tuple<std::unordered_set<Height, utils::BaseValueHasher<Height>>, std::unordered_set<Key, utils::ArrayHasher<Key>>>;
			using ElementContainerType = std::tuple<std::unordered_set<const state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>*>, std::unordered_set<const state::LockFundRecordGroup<state::LockFundKeyIndexDescriptor>*>>;

			static constexpr std::string_view  Collection_Names[] = {"lockFundHeightRecords", "lockFundKeyRecords"};
			static constexpr std::string_view Id_Property_Names[] = {"lockFundRecordGroup.identifier", "lockFundRecordGroup.identifier"};

			// TEMP: Verify we can remove index because disambiguation will not ever be needed!!
			template<int TIndex, typename TParam, typename TReturn>
			static TReturn GetId(const TParam& descriptor, std::integral_constant<int, TIndex> = {})
			{
				return descriptor.Identifier;
			}
			template<int TIndex, typename TParam, typename TReturn>
			static TReturn MapToMongoId(const TParam& id, std::integral_constant<int, TIndex> cnt = {})
			{
				return MapToMongoIdImpl<TIndex, TParam, TReturn>(id, cnt);
			}
			template<int TIndex, typename TParam>
			static bsoncxx::document::value MapToMongoDocument(const TParam& model, model::NetworkIdentifier networkIdentifier, std::integral_constant<int, TIndex> cnt = {} )
			{
				return MapToMongoDocumentImpl<TIndex, TParam>(model, networkIdentifier, cnt);
			}

		};
	}
	// SELF: MULTISET ONLY WORKS FOR OUTPUT, NOT INPUT
	DEFINE_MONGO_MULTISET_CACHE_STORAGE(LockFund, LockFundCacheTraits)
}}}
