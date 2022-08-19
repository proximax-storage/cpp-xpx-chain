/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/cache_core/AccountStateCache.h"
#include "src/cache/LockFundCache.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "src/validators/Validators.h"
#include "catapult/validators/ValidationResult.h"
#include "src/config/LockFundConfiguration.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"
#include "LockFundCacheFactory.h"

namespace catapult { namespace test {

	constexpr auto Success_Result = validators::ValidationResult::Success;
	template<typename TNotification, typename TValidator, typename TPrepareFunc, typename TModifier = void(cache::LockFundCacheDelta&)>
	void AssertValidationResult(validators::ValidationResult expectedResult,
								TNotification& notification,
								std::unique_ptr<TValidator> pValidator,
								const BlockDuration& unlockCooldown,
								const uint16_t& maxMosaicsSize,
								const crypto::KeyPair& keypair,
								TPrepareFunc prepareAccount,
								TModifier modifyCache = [](cache::LockFundCacheDelta&){}) {
		// Arrange:

		auto pluginConfig = config::LockFundConfiguration::Uninitialized();
		pluginConfig.MaxMosaicsSize = maxMosaicsSize;
		pluginConfig.MinRequestUnlockCooldown = unlockCooldown;
		pluginConfig.MaxUnlockRequests = 5;
		test::MutableBlockchainConfiguration mutableConfig;
		mutableConfig.Network.SetPluginConfiguration(pluginConfig);
		auto config = mutableConfig.ToConst();
		auto cache = test::CreateEmptyCatapultCache<LockFundCacheFactory>(config);
		auto delta = cache.createDelta();
		auto& accountCache = delta.sub<cache::AccountStateCache>();
		auto& lockFundCache = delta.sub<cache::LockFundCache>();
		accountCache.addAccount(keypair.publicKey(), Height(0), 2);
		auto& account = accountCache.find(keypair.publicKey()).get();
		prepareAccount(account);
		modifyCache(lockFundCache);
		cache.commit(Height(0));

		auto cacheView = cache.createView();
		auto readOnlyCache = cacheView.toReadOnly();
		model::ResolverContext resolverContext;
		auto context = validators::ValidatorContext(config, Height(100), Timestamp(8888), resolverContext, readOnlyCache);

		// Act:
		auto result = test::ValidateNotification(*pValidator, notification, context);

		// Assert:
		EXPECT_EQ(expectedResult, result);
	}

	template<typename TIdentifier>
	void AssertEqual(state::LockFundRecordGroup<TIdentifier> originalRecord, state::LockFundRecordGroup<TIdentifier> loadedRecord)
	{
		EXPECT_EQ(originalRecord.Identifier, loadedRecord.Identifier);
		EXPECT_EQ(originalRecord.LockFundRecords.size(), loadedRecord.LockFundRecords.size());
		for(auto &pair : loadedRecord.LockFundRecords)
		{
			auto deserializedPair = originalRecord.LockFundRecords.find(pair.first);
			EXPECT_EQ(pair.first, deserializedPair->first);
			EXPECT_EQ(pair.second.Size(), deserializedPair->second.Size());
			EXPECT_EQ(pair.second.Active(), deserializedPair->second.Active());
			for(auto mosaic : pair.second.Get())
			{
				EXPECT_EQ(deserializedPair->second.Get().find(mosaic.first)->second, mosaic.second);
			}
			for(auto inactiveRecord  : pair.second.InactiveRecords)
			{
				int i = 0;
				for(auto mosaic : inactiveRecord)
				{
					EXPECT_EQ(deserializedPair->second.InactiveRecords[i].find(mosaic.first)->second, mosaic.second);
					i++;
				}
			}
		}
	}

	void AssertEqual(state::LockFundRecord originalRecord, state::LockFundRecord loadedRecord);

	std::unordered_map<Key, state::LockFundRecordGroup<state::LockFundKeyIndexDescriptor>, utils::ArrayHasher<Key>> DeriveKeyRecordsFromHeightRecord(state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor> record);

	struct DefaultRecordGroupGeneratorTraitsBase
	{
		static std::optional<state::LockFundRecordMosaicMap> GenerateActiveRecord(uint32_t index)
		{
			return std::optional<state::LockFundRecordMosaicMap>({{MosaicId(72), Amount(200)}});
		}

		static std::vector<state::LockFundRecordMosaicMap> GenerateInactiveRecords(uint32_t index)
		{
			return std::vector<state::LockFundRecordMosaicMap>({{{MosaicId(73), Amount(400)}}, {{MosaicId(73), Amount(400)}}});
		}
	};

	template<typename TIdentifier>
	struct DefaultRecordGroupGeneratorTraits;
	template<>
	struct DefaultRecordGroupGeneratorTraits<state::LockFundKeyIndexDescriptor> : public DefaultRecordGroupGeneratorTraitsBase
	{
		static typename state::LockFundKeyIndexDescriptor::ValueIdentifier GenerateIdentifier(uint32_t index)
		{
			return Height(index);
		}
	};
	template<>
	struct DefaultRecordGroupGeneratorTraits<state::LockFundHeightIndexDescriptor> : public DefaultRecordGroupGeneratorTraitsBase
	{
		static typename state::LockFundHeightIndexDescriptor::ValueIdentifier GenerateIdentifier(uint32_t index)
		{
			return GenerateRandomByteArray<Key>();
		}
	};
	std::shared_ptr<config::BlockchainConfigurationHolder> CreateLockFundConfigHolder();
	template<typename TIdentifier, typename TGeneratorTraits>
	state::LockFundRecordGroup<TIdentifier> GenerateRecordGroup(typename TIdentifier::KeyType identifier, uint32_t numRecords)
	{
		state::LockFundRecordGroup<TIdentifier> lockFundRecordGroup(identifier, {});
		for (int i=0; i<numRecords; ++i)
		{
			std::optional<state::LockFundRecordMosaicMap> mosaics = TGeneratorTraits::GenerateActiveRecord(i);
			std::vector<state::LockFundRecordMosaicMap> inactiveMosaics =  TGeneratorTraits::GenerateInactiveRecords(i);
			state::LockFundRecord record;
			if(mosaics.has_value())
				record.Set(mosaics.value());
			if(!inactiveMosaics.empty())
				record.InactiveRecords = std::move(inactiveMosaics);
			lockFundRecordGroup.LockFundRecords.insert(std::pair(TGeneratorTraits::GenerateIdentifier(i), std::move(record)));
		}
		return lockFundRecordGroup;
	}



}}
