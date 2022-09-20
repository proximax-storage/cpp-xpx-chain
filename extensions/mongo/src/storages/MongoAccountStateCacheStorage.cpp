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

#include "catapult/state/StakingRecord.h"
#include "MongoAccountStateCacheStorage.h"
#include "mongo/src/mappers/AccountStateMapper.h"
#include "mongo/src/mappers/StakingRecordMapper.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/thread/FutureUtils.h"
using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace storages {

	namespace {

		struct AccountStateCacheTraits {
			using CacheType = cache::AccountStateCache;
			using CacheDeltaType = cache::AccountStateCacheDelta;
			using KeyType = Address;
			using ModelType = state::AccountState;

			static constexpr auto Collection_Name = "accounts";
			static constexpr auto Id_Property_Name = "account.address";
			static constexpr auto Additional_Collection_Name = "staking_accounts";
			static constexpr auto Id_Additional_Property_Name = "stakingAccount.address";
			static auto GetId(const ModelType& accountState) {
				return accountState.Address;
			}

			static auto MapToMongoId(const Address& address) {
				return mappers::ToBinary(address);
			}

			static auto MapToMongoDocument(const state::AccountState& accountState, model::NetworkIdentifier) {
				return mappers::ToDbModel(accountState);
			}

			static auto MapAdditionalToMongoDocument(const state::AccountState& accountState, const MosaicId& harvestingMosaicId, const Height& height) {
				return mappers::ToDbModel(state::StakingRecord(accountState, harvestingMosaicId, height));
			}

			static void RemoveCallback(StorageCallbackContext& context, const std::unordered_set<const ModelType*>& elements){
				// Accounts never get removed!
				auto deleteResults = context.BulkWriter.bulkDelete(Additional_Collection_Name, elements, CreateFilterByKey).get();
			}

			static void InsertCallback(StorageCallbackContext& context, const std::unordered_set<const ModelType*>& elements){
				const auto harvestingMosaicId = context.ConfigHolder.Config(context.CurrentHeight).Immutable.HarvestingMosaicId;
				auto createDocument = [harvestingMosaicId, currentHeight = context.CurrentHeight](const auto* pModel, auto) {
				  return MapAdditionalToMongoDocument(*pModel, harvestingMosaicId, currentHeight);
				};
				std::unordered_set<const ModelType*> filteredInsertSet;
				std::unordered_set<const ModelType*> filteredRemoveSet;
				for(auto element : elements)
				{
					if(element->GetVersion() < 2 || element->IsLocked())
						continue;
					auto balance = element->Balances.getLocked(harvestingMosaicId);
					if(balance > Amount(0))
						filteredInsertSet.insert(element);
					else
						filteredRemoveSet.insert(element);
				}
				auto upsertResults = context.BulkWriter.bulkUpsert(Additional_Collection_Name, filteredInsertSet, createDocument, CreateFilterByKey).get();
				auto aggregateResult = BulkWriteResult::Aggregate(thread::get_all(std::move(upsertResults)));
				context.ErrorPolicy.checkUpserted(filteredInsertSet.size(), aggregateResult, "modified and added elements to secondary channel");
				auto deleteResults = context.BulkWriter.bulkDelete(Additional_Collection_Name, filteredRemoveSet, CreateFilterByKey).get();
			}

			static bsoncxx::document::value CreateFilterByKey(const ModelType* model) {
				using namespace bsoncxx::builder::stream;
				return document() << std::string(Id_Additional_Property_Name) << mappers::ToBinary(model->Address) << finalize;
			}

		};
	}

	DEFINE_MONGO_FLAT_CACHE_STORAGE(AccountState, AccountStateCacheTraits)
}}}
