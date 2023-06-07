/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
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

#include "src/validators/Validators.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"
#include "src/catapult/cache_core/AccountStateCache.h"

namespace catapult { namespace validators {

#define TEST_CLASS AddressInteractionViabilityValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(AddressInteractionViability,)

	namespace {
		// endregion

		// region test utils

		model::UnresolvedAddressSet UnresolveXorAll(const std::vector<Address>& addresses) {
			model::UnresolvedAddressSet unresolvedAddresses;
			for (const auto& address : addresses)
				unresolvedAddresses.insert(test::UnresolveXor(address));

			return unresolvedAddresses;
		}


		void PopulateCache(
				cache::CatapultCache& cache,
				const std::vector<Address>& normalAccounts,
				const std::vector<Address>& lockedAccounts) {
			auto delta = cache.createDelta();
			auto& accountCacheDelta = delta.sub<cache::AccountStateCache>();
			for (const auto& address : normalAccounts) {
				accountCacheDelta.addAccount(address, Height(1));
			}
			for (const auto& address : lockedAccounts) {
				accountCacheDelta.addAccount(address, Height(1));
				auto& account = accountCacheDelta.find(address).get();
				account.AccountType = state::AccountType::Locked;
			}

			cache.commit(Height(1));
		}

		void AssertValidationResult(
				ValidationResult expectedResult,
				const std::vector<Address>& normalAccounts,
				const std::vector<Address>& lockedAccounts,
				const model::UnresolvedAddressSet& participantsByAddress) {
			// Arrange:
			auto cache = test::CoreSystemCacheFactory::Create();
			PopulateCache(cache, normalAccounts, lockedAccounts);
			auto pValidator = CreateAddressInteractionViabilityValidator();
			auto entityType = static_cast<model::EntityType>(0x4123);
			auto source = test::GenerateRandomByteArray<Key>();
			auto notification = model::AddressInteractionNotification<1>(source, entityType, participantsByAddress);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}

		// endregion
	}


	TEST(TEST_CLASS, FailureWhenOneOfParticipantsIsLocked) {
		auto okAddresses = test::GenerateRandomDataVector<Address>(3);
		auto lockedAddresses = test::GenerateRandomDataVector<Address>(1);
		auto participants = std::vector<Address>{okAddresses[0], okAddresses[1], lockedAddresses[0], okAddresses[2]};
		AssertValidationResult(Failure_Core_Participant_Is_Locked, okAddresses, lockedAddresses, UnresolveXorAll(participants));
	}

	TEST(TEST_CLASS, SuccessWhenNoneOfParticipantsIsLocked) {
		auto okAddresses = test::GenerateRandomDataVector<Address>(4);
		auto participants = std::vector<Address>{okAddresses[0], okAddresses[1], okAddresses[2], okAddresses[3]};
		AssertValidationResult(ValidationResult::Success, okAddresses, {}, UnresolveXorAll(participants));
	}

}}
