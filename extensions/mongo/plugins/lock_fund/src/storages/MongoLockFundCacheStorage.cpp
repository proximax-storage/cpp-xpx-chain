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
			static constexpr std::string_view Id_Property_Names[] = {"lockFundHeightRecords.identifier", "lockFundKeyRecords.identifier"};

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
