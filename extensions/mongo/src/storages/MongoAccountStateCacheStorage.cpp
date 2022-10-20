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
#include "catapult/utils/ConfigurationUtils.h"
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

			static auto MapAdditionalToMongoDocument(const state::AccountState& accountState, const MosaicId& harvestingMosaicId, const Height& height, const Height& refHeight) {
				return mappers::ToDbModel(state::StakingRecord(accountState, harvestingMosaicId, height, refHeight));
			}

			static void PrepareStorage(MongoDatabase& mongoDatabase){
				if(mongoDatabase.HasCollection(Additional_Collection_Name))
					return;

				bool success = true;
				try {
					auto address_index = document{} << "stakingAccount.address" << 1 << finalize;
					auto height_index = document{} << "stakingAccount.refHeight" << 1 << finalize;
					mongocxx::options::index address_index_options {};
					address_index_options.name("Address");
					mongocxx::options::index height_index_options {};
					height_index_options.name("RefHeight");
					auto collection = mongoDatabase.CreateCollection(Additional_Collection_Name);

					// Ensure collection exists
					auto result = collection.create_index(std::move(address_index), address_index_options);
					auto view = result.view();
					if (view.empty() || view.find("name") == view.end()) {
						CATAPULT_THROW_RUNTIME_ERROR("Unable to create address index for staking records collection.");
					}
					result = collection.create_index(std::move(height_index), height_index_options);
					view = result.view();
					if (view.empty() || view.find("name") == view.end()) {
						CATAPULT_THROW_RUNTIME_ERROR("Unable to create refHeight index for staking records collection.");
					}
				}
				catch (mongocxx::exception e) {
					CATAPULT_THROW_RUNTIME_ERROR_1("Unable to prepare collection for staking records", std::string(e.what()));
				}
				if(!success)
					CATAPULT_THROW_RUNTIME_ERROR("Unable to prepare collection for staking records.");
			}

			static void OnRemove(StorageCallbackContext& context, const std::unordered_set<const ModelType*>& elements){
				// Accounts never get removed!
				auto interval = context.ConfigHolder.Config(context.CurrentHeight).Network.DockStakeRewardInterval.unwrap();
				if(interval == 0) return;
				auto deleteResults = context.BulkWriter.bulkDelete(Additional_Collection_Name, elements,
				   [refHeight = utils::GetClosestHeight(context.CurrentHeight.unwrap(), interval)](const ModelType* model){
					   return CreateAdditionalFilterByKey(model, refHeight);
				   }).get();
			}

			static void OnUpsert(StorageCallbackContext& context, const std::unordered_set<const ModelType*>& elements){
				auto interval = context.ConfigHolder.Config(context.CurrentHeight).Network.DockStakeRewardInterval.unwrap();
				if(interval == 0) return;
				auto lastCalculatedHeight = utils::GetClosestHeight(context.CurrentHeight.unwrap(), interval);
				const auto harvestingMosaicId = context.ConfigHolder.Config(context.CurrentHeight).Immutable.HarvestingMosaicId;
				auto createDocument = [harvestingMosaicId, currentHeight = context.CurrentHeight, refHeight = lastCalculatedHeight](const auto* pModel, auto) {
				  return MapAdditionalToMongoDocument(*pModel, harvestingMosaicId, currentHeight, refHeight);
				};
				std::unordered_set<const ModelType*> filteredInsertSet;
				for(auto element : elements)
				{
					if(element->GetVersion() < 2 || element->IsLocked() || !element->Balances.changes().hasChange<state::AccountBalancesTrackables::Locked>())
						continue;
					auto balance = element->Balances.getLocked(harvestingMosaicId);
					filteredInsertSet.insert(element);
				}
				auto upsertResults = context.BulkWriter.bulkUpsert(Additional_Collection_Name, filteredInsertSet, createDocument, [refHeight = lastCalculatedHeight](const ModelType* model){
					   return CreateAdditionalFilterByKey(model, refHeight);
				   }).get();
				auto aggregateResult = BulkWriteResult::Aggregate(thread::get_all(std::move(upsertResults)));
				context.ErrorPolicy.checkUpserted(filteredInsertSet.size(), aggregateResult, "modified and added elements to secondary channel");
			}

			static bsoncxx::document::value CreateAdditionalFilterByKey(const ModelType* model, Height refHeight) {
				using namespace bsoncxx::builder::stream;
				return document() << std::string(Id_Additional_Property_Name) << mappers::ToBinary(model->Address)
								  << "stakingAccount.refHeight" << mappers::ToInt64(refHeight)<< finalize;
			}

		};
	}

	DEFINE_MONGO_FLAT_CACHE_STORAGE(AccountState, AccountStateCacheTraits)
}}}
