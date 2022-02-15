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

#include "catapult/validators/ValidationResult.h"
#include "src/cache/LockFundCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "src/config/LockFundConfiguration.h"
#include "src/validators/Validators.h"
#include "src/model/LockFundNotifications.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"
#include "tests/test/nodeps/KeyTestUtils.h"
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
		test::MutableBlockchainConfiguration mutableConfig;
		mutableConfig.Network.SetPluginConfiguration(pluginConfig);
		auto config = mutableConfig.ToConst();
		auto cache = test::CreateEmptyCatapultCache(config);
		auto delta = cache.createDelta();
		auto& accountCache = delta.sub<cache::AccountStateCache>();
		auto& lockFundCache = delta.sub<cache::LockFundCache>();
		accountCache.addAccount(keypair.publicKey(), Height(0), 2);
		auto account = accountCache.find(keypair.publicKey()).get();
		prepareAccount(account);
		modifyCache(lockFundCache);
		cache.commit(Height(1));

		// Act:
		auto result = test::ValidateNotification(*pValidator, notification, cache, config);

		// Assert:
		EXPECT_EQ(expectedResult, result);
	}

	struct DefaultRecordGroupGeneratorTraitsBase
	{
		std::optional<state::LockFundRecordMosaicMap> GenerateActiveRecord(uint32_t index)
		{
			return std::optional<state::LockFundRecordMosaicMap>({{MosaicId(72), Amount(200)}});
		}

		std::vector<state::LockFundRecordMosaicMap> GenerateInactiveRecords(uint32_t index)
		{
			return std::vector<state::LockFundRecordMosaicMap>({{{MosaicId(73), Amount(400)}}, {{MosaicId(73), Amount(400)}}});
		}
	};

	struct DefaultRecordGroupGeneratorHeightTraits : public DefaultRecordGroupGeneratorTraitsBase
	{
		Height GenerateIdentifier(uint32_t index)
		{
			return Height(index);
		}
	};

	struct DefaultRecordGroupGeneratorKeyTraits : public DefaultRecordGroupGeneratorTraitsBase
	{
		Key GenerateIdentifier(uint32_t index)
		{
			return GenerateRandomByteArray<Key>();
		}
	};

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
