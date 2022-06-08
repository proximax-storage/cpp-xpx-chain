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

#include "plugins/txes/aggregate/src/model/AggregateTransaction.h"
#include "src/validators/Validators.h"
#include "plugins/txes/aggregate/src/model/AggregateEntityType.h"
#include "tests/test/AggregateTransactionPluginTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS AggregateHashPresentValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(AggregateHashPresent,)

	namespace {
		auto CreateNotification(const Hash256& transactionHash, model::EntityType transactionType) {
			return model::TransactionNotification<1>(Key(), transactionHash, transactionType, Timestamp());
		}

		auto CreateCache(const Hash256& transactionHash, Height expirationHeight, state::LockStatus status = state::LockStatus::Unused) {
			auto cache = test::HashLockInfoCacheFactory::Create();
			auto cacheDelta = cache.createDelta();
			auto& lockInfoCacheDelta = cacheDelta.sub<cache::HashLockInfoCache>();
			auto lockInfo = test::BasicHashLockInfoTestTraits::CreateLockInfo(expirationHeight);
			lockInfo.Hash = transactionHash;
			lockInfo.Status = status;
			lockInfoCacheDelta.insert(lockInfo);
			cache.commit(Height());
			return cache;
		}

		auto RunValidator(
				cache::CatapultCache& cache,
				const Hash256& transactionHash,
				model::EntityType transactionType,
				Height notificationHeight) {
			// Arrange:
			auto pValidator = CreateAggregateHashPresentValidator();

			// Act:
			return test::ValidateNotification<model::TransactionNotification<1>>(
					*pValidator,
					CreateNotification(transactionHash, transactionType),
					cache,
					config::BlockchainConfiguration::Uninitialized(),
					notificationHeight);
		}

	}

#define TRAITS_BASED_TEST(TEST_NAME) \
    template<typename TDescriptor>                                 \
	void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_v1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<model::AggregateTransactionRawDescriptor>(); } \
	TEST(TEST_CLASS, TEST_NAME##_v2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<model::AggregateTransactionExtendedDescriptor>(); } \
    template<typename TDescriptor>                                 \
	void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	TRAITS_BASED_TEST(SuccessForNonAggregateBondedTransactionType) {
		// Arrange:
		auto cache = CreateCache(test::GenerateRandomByteArray<Hash256>(), Height());

		// Act:
		auto result = RunValidator(cache, test::GenerateRandomByteArray<Hash256>(), TDescriptor::CompleteType, Height());

		// Assert:
		EXPECT_EQ(ValidationResult::Success, result);
	}

	TRAITS_BASED_TEST(SuccessWhenLockInfoMeetsAllConditions) {
		// Arrange:
		auto hash = test::GenerateRandomByteArray<Hash256>();
		auto cache = CreateCache(hash, Height(123));

		// Act:
		auto result = RunValidator(cache, hash, TDescriptor::BondedType, Height(120));

		// Assert:
		EXPECT_EQ(ValidationResult::Success, result);
	}

	TRAITS_BASED_TEST(QFailureForUnknownTransactionHash) {
		// Arrange:
		auto cache = CreateCache(test::GenerateRandomByteArray<Hash256>(), Height());

		// Act:
		auto result = RunValidator(cache, test::GenerateRandomByteArray<Hash256>(), TDescriptor::BondedType, Height());

		// Assert:
		EXPECT_EQ(Failure_LockHash_Hash_Does_Not_Exist, result);
	}

	TRAITS_BASED_TEST(FailureWhenLockInfoIsExpired_AtExpirationHeight) {
		// Arrange:
		auto hash = test::GenerateRandomByteArray<Hash256>();
		auto cache = CreateCache(hash, Height(123));

		// Act:
		auto result = RunValidator(cache, hash, TDescriptor::BondedType, Height(123));

		// Assert:
		EXPECT_EQ(Failure_LockHash_Inactive_Hash, result);
	}

	TRAITS_BASED_TEST(FailureWhenLockInfoIsExpired_AfterExpirationHeight) {
		// Arrange:
		auto hash = test::GenerateRandomByteArray<Hash256>();
		auto cache = CreateCache(hash, Height(123));

		// Act:
		auto result = RunValidator(cache, hash, TDescriptor::BondedType, Height(150));

		// Assert:
		EXPECT_EQ(Failure_LockHash_Inactive_Hash, result);
	}

	TRAITS_BASED_TEST(FailureWhenTransactionHashIsAlreadyUsed) {
		// Arrange:
		auto hash = test::GenerateRandomByteArray<Hash256>();
		auto cache = CreateCache(hash, Height(123), state::LockStatus::Used);

		// Act:
		auto result = RunValidator(cache, hash, TDescriptor::BondedType, Height());

		// Assert:
		EXPECT_EQ(Failure_LockHash_Inactive_Hash, result);
	}
#undef TRAITS_BASED_TEST
}}
