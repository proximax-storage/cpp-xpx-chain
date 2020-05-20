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

#include "src/validators/Validators.h"
#include "tests/test/PropertyCacheTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS MosaicRecipientValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(MosaicRecipient,)
	DEFINE_COMMON_VALIDATOR_TESTS(LevyRecipient,)
	
	namespace {
		model::ResolverContext CreateResolverContext(MosaicId mosaicId, Address recipient) {
			return model::ResolverContext(
				[](const auto&) { return MosaicId(1); },
				[](const auto&) { return Address(); },
				[](const auto&) { return Amount(1000); },
				[mosaicId](const auto&) { return MosaicId(mosaicId);; },
				[recipient](const auto&) { return recipient; });
		}
		
		template<typename TOperationTraits>
		void PopulateCache(cache::CatapultCache& cache, const Address& accountAddress, const std::vector<MosaicId>& mosaicIds) {
			auto delta = cache.createDelta();
			auto& propertyCacheDelta = delta.sub<cache::PropertyCache>();
			propertyCacheDelta.insert(state::AccountProperties(accountAddress));
			auto& accountProperties = propertyCacheDelta.find(accountAddress).get();
			auto& accountProperty = accountProperties.property(model::PropertyType::MosaicId);
			for (auto mosaicId : mosaicIds)
				TOperationTraits::Add(accountProperty, state::ToVector(mosaicId));

			cache.commit(Height(1));
		}

		template<typename TOperationTraits>
		void AssertValidationResult(
				ValidationResult expectedResult,
				const Address& accountAddress,
				const std::vector<MosaicId>& mosaicIds,
				const UnresolvedAddress& recipient,
				UnresolvedMosaicId mosaicId) {
			// Arrange:
			auto cache = test::PropertyCacheFactory::Create();
			PopulateCache<TOperationTraits>(cache, accountAddress, mosaicIds);
			auto pValidator = CreateMosaicRecipientValidator();
			auto sender = test::GenerateRandomByteArray<Key>();
			auto notification = model::BalanceTransferNotification<1>(sender, recipient, mosaicId, Amount(123));

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
		
		template<typename TOperationTraits>
		void AssertLevyValidationResult(
			ValidationResult expectedResult,
			const Address& accountAddress,
			const std::vector<MosaicId>& mosaicIds,
			const Address& recipient,
			MosaicId mosaicId) {
			// Arrange:
			auto cache = test::PropertyCacheFactory::Create();
			
			PopulateCache<TOperationTraits>(cache, accountAddress, mosaicIds);
			auto pValidator = CreateLevyRecipientValidator();
			auto sender = test::GenerateRandomByteArray<Key>();
			
			auto notification = model::LevyTransferNotification<1>(sender, UnresolvedLevyAddress(),
				UnresolvedLevyMosaicId(444), UnresolvedAmount(100));
			
			auto resolverContext = CreateResolverContext(mosaicId, recipient);
			
			auto cacheView = cache.createView();
			auto readOnlyCache = cacheView.toReadOnly();
			auto validatorContext = ValidatorContext(config::BlockchainConfiguration::Uninitialized(),Height(1),
				Timestamp(0), resolverContext, readOnlyCache);
			
			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, validatorContext);
			
			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	// region failure

	TEST(TEST_CLASS, FailureWhenRecipientIsKnownAndMosaicIdIsNotContainedInValues_Allow) {
		// Arrange:
		auto accountAddress = test::GenerateRandomByteArray<Address>();

		// Act:
		AssertValidationResult<test::AllowTraits>(
				Failure_Property_Mosaic_Transfer_Not_Allowed,
				accountAddress,
				test::GenerateRandomDataVector<MosaicId>(3),
				test::UnresolveXor(accountAddress),
				test::GenerateRandomValue<UnresolvedMosaicId>());
	}

	TEST(TEST_CLASS, FailureWhenRecipientIsKnownAndMosaicIdIsContainedInValues_Block) {
		// Arrange:
		auto accountAddress = test::GenerateRandomByteArray<Address>();
		auto values = test::GenerateRandomDataVector<MosaicId>(3);

		// Act:
		AssertValidationResult<test::BlockTraits>(
				Failure_Property_Mosaic_Transfer_Not_Allowed,
				accountAddress,
				values,
				test::UnresolveXor(accountAddress),
				test::UnresolveXor(values[1]));
	}
	
	TEST(TEST_CLASS, FailureWhenLevyRecipientIsKnownAndMosaicIdIsNotContainedInValues_Allow) {
		// Arrange:
		auto accountAddress = test::GenerateRandomByteArray<Address>();
		
		// Act:
		AssertLevyValidationResult<test::AllowTraits>(
			Failure_Property_Mosaic_Transfer_Not_Allowed,
			accountAddress,
			test::GenerateRandomDataVector<MosaicId>(3),
			accountAddress,
			MosaicId(1));
	}
		
	TEST(TEST_CLASS, FailureWhenLevyRecipientIsKnownAndMosaicIdIsContainedInValues_Block) {
		// Arrange:
		auto accountAddress = test::GenerateRandomByteArray<Address>();
		auto values = test::GenerateRandomDataVector<MosaicId>(3);
		
		// Act:
		AssertLevyValidationResult<test::BlockTraits>(
			Failure_Property_Mosaic_Transfer_Not_Allowed,
			accountAddress,
			values,
			accountAddress,
			values[1]);
	}
	// endregion

	// region success

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Allow) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<test::AllowTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Block) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<test::BlockTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	TRAITS_BASED_TEST(SuccessWhenRecipientIsNotKnown) {
		// Arrange:
		auto accountAddress = test::GenerateRandomByteArray<Address>();

		// Act:
		AssertValidationResult<TTraits>(
				ValidationResult::Success,
				accountAddress,
				test::GenerateRandomDataVector<MosaicId>(3),
				test::UnresolveXor(test::GenerateRandomByteArray<Address>()),
				test::GenerateRandomValue<UnresolvedMosaicId>());
	}

	TRAITS_BASED_TEST(SuccessWhenRecipientIsKnownButPropertyHasNoValues) {
		// Arrange:
		auto accountAddress = test::GenerateRandomByteArray<Address>();

		// Act:
		AssertValidationResult<TTraits>(
				ValidationResult::Success,
				accountAddress,
				test::GenerateRandomDataVector<MosaicId>(0),
				test::UnresolveXor(accountAddress),
				test::GenerateRandomValue<UnresolvedMosaicId>());
	}
	
	TRAITS_BASED_TEST(SuccessWhenLevyRecipientIsNotKnown) {
		// Arrange:
		auto accountAddress = test::GenerateRandomByteArray<Address>();
		
		// Act:
		AssertLevyValidationResult<TTraits>(
			ValidationResult::Success,
			accountAddress,
			test::GenerateRandomDataVector<MosaicId>(3),
			test::GenerateRandomByteArray<Address>(),
			MosaicId());
	}
	
	TRAITS_BASED_TEST(SuccessWhenLevyRecipientIsKnownButPropertyHasNoValues) {
		// Arrange:
		auto accountAddress = test::GenerateRandomByteArray<Address>();
		
		// Act:
		AssertLevyValidationResult<TTraits>(
			ValidationResult::Success,
			accountAddress,
			test::GenerateRandomDataVector<MosaicId>(0),
			accountAddress,
			MosaicId());
	}
	
	namespace {
		template<typename TOperationTraits>
		void AssertSuccess(const std::vector<MosaicId>& mosaicIds, UnresolvedMosaicId transferredMosaicId) {
			// Arrange:
			auto accountAddress = test::GenerateRandomByteArray<Address>();

			// Act:
			AssertValidationResult<TOperationTraits>(
					ValidationResult::Success,
					accountAddress,
					mosaicIds,
					test::UnresolveXor(accountAddress),
					transferredMosaicId);
		}
		
		template<typename TOperationTraits>
		void AssertSuccessLevy(const std::vector<MosaicId>& mosaicIds, MosaicId transferredMosaicId) {
			// Arrange:
			auto accountAddress = test::GenerateRandomByteArray<Address>();
			
			// Act:
			AssertLevyValidationResult<TOperationTraits>(
				ValidationResult::Success,
				accountAddress,
				mosaicIds,
				accountAddress,
				transferredMosaicId);
		}
	}

	TEST(TEST_CLASS, SuccessWhenAllConditionsAreMet_Allow) {
		// Arrange:
		auto mosaicIds = test::GenerateRandomDataVector<MosaicId>(3);

		// Act:
		AssertSuccess<test::AllowTraits>(mosaicIds, test::UnresolveXor(mosaicIds[1]));
	}

	TEST(TEST_CLASS, SuccessWhenAllConditionsAreMet_Block) {
		// Act:
		AssertSuccess<test::BlockTraits>(test::GenerateRandomDataVector<MosaicId>(3), test::GenerateRandomValue<UnresolvedMosaicId>());
	}
	
	TEST(TEST_CLASS, SuccessWhenAllConditionsAreMetLevy_Allow) {
		// Arrange:
		auto mosaicIds = test::GenerateRandomDataVector<MosaicId>(3);
		
		// Act:
		AssertSuccessLevy<test::AllowTraits>(mosaicIds, mosaicIds[1]);
	}
	
	TEST(TEST_CLASS, SuccessWhenAllConditionsAreMetLevy_Block) {
		// Act:
		AssertSuccessLevy<test::BlockTraits>(test::GenerateRandomDataVector<MosaicId>(3), test::GenerateRandomValue<MosaicId>());
	}
	// endregion
}}
