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
#include "src/model/TransactionTypePropertyTransaction.h"
#include "tests/test/PropertyCacheTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS TransactionTypeNoSelfBlockingValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(TransactionTypeNoSelfBlocking,)

	namespace {
		using Notification = model::ModifyTransactionTypePropertyValueNotification;

		constexpr auto Add = model::PropertyModificationType::Add;
		constexpr auto Del = model::PropertyModificationType::Del;
		constexpr auto Relevant_Entity_Type = model::TransactionTypePropertyTransaction::Entity_Type;
		constexpr auto Failure_Result = Failure_Property_Modification_Not_Allowed;

		struct TransactionTypePropertyTraits : public test::BaseTransactionTypePropertyTraits {
			using NotificationType = model::ModifyTransactionTypePropertyValueNotification;
		};

		auto RandomValue() {
			return static_cast<model::EntityType>(test::RandomByte());
		}

		template<typename TOperationTraits>
		void AddToCache(cache::CatapultCache& cache, const Key& key, const std::vector<model::EntityType>& values) {
			auto delta = cache.createDelta();
			auto& propertyCacheDelta = delta.sub<cache::PropertyCache>();
			auto address = model::PublicKeyToAddress(key, model::NetworkIdentifier::Zero);
			auto accountProperties = state::AccountProperties(address);
			auto& accountProperty = accountProperties.property(model::PropertyType::TransactionType);
			for (auto value : values)
				TOperationTraits::Add(accountProperty, state::ToVector(value));

			propertyCacheDelta.insert(accountProperties);
			cache.commit(Height(1));
		}

		void RunValidator(ValidationResult expectedResult, cache::CatapultCache& cache, const Notification& notification) {
			// Arrange:
			auto pValidator = CreateTransactionTypeNoSelfBlockingValidator();

			// Act:
			auto result = test::ValidateNotification<Notification>(*pValidator, notification, cache);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}

		enum class CacheSeed { No, EmptyProperties, RandomValue, RelevantValue};

		template<typename TOperationTraits>
		void AssertValidationResult(
				ValidationResult expectedResult,
				CacheSeed cacheSeed,
				const Key& seedKey,
				const Notification& notification) {
			// Arrange:
			auto cache = test::PropertyCacheFactory::Create();
			switch (cacheSeed) {
			case CacheSeed::No:
				break;
			case CacheSeed::EmptyProperties:
				AddToCache<TOperationTraits>(cache, seedKey, {});
				break;
			case CacheSeed::RandomValue:
				AddToCache<TOperationTraits>(cache, seedKey, { RandomValue() });
				break;
			case CacheSeed::RelevantValue:
				AddToCache<TOperationTraits>(cache, seedKey, { Relevant_Entity_Type });
				break;
			}

			// Act:
			RunValidator(expectedResult, cache, notification);
		}
	}

	// region failure

	TEST(TEST_CLASS, FailureWhenAccountIsUnknown_Allow_Add_NotRelevantType) {
		// Arrange:
		auto seedKey = test::GenerateRandomByteArray<Key>();
		auto notificationKey = test::GenerateRandomByteArray<Key>();
		auto modification = model::PropertyModification<model::EntityType>{ Add, RandomValue() };
		auto notification = test::CreateNotification<TransactionTypePropertyTraits, test::AllowTraits>(notificationKey, modification);

		// Act + Assert:
		AssertValidationResult<test::AllowTraits>(Failure_Result, CacheSeed::No, seedKey, notification);
	}

	TEST(TEST_CLASS, FailureWhenAccountIsUnknown_Block_Add_RelevantType) {
		// Arrange:
		auto seedKey = test::GenerateRandomByteArray<Key>();
		auto notificationKey = test::GenerateRandomByteArray<Key>();
		auto modification = model::PropertyModification<model::EntityType>{ Add, Relevant_Entity_Type };
		auto notification = test::CreateNotification<TransactionTypePropertyTraits, test::BlockTraits>(notificationKey, modification);

		// Act + Assert:
		AssertValidationResult<test::BlockTraits>(Failure_Result, CacheSeed::No, seedKey, notification);
	}

	TEST(TEST_CLASS, FailureWhenAccountIsKnown_Allow_Del_RelevantType) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto modification = model::PropertyModification<model::EntityType>{ Del, Relevant_Entity_Type };
		auto notification = test::CreateNotification<TransactionTypePropertyTraits, test::AllowTraits>(key, modification);

		// Act + Assert:
		AssertValidationResult<test::AllowTraits>(Failure_Result, CacheSeed::EmptyProperties, key, notification);
	}

	TEST(TEST_CLASS, FailureWhenAccountIsKnown_Allow_Add_NotRelevantType) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto modification = model::PropertyModification<model::EntityType>{ Add, RandomValue() };
		auto notification = test::CreateNotification<TransactionTypePropertyTraits, test::AllowTraits>(key, modification);

		// Act + Assert:
		AssertValidationResult<test::AllowTraits>(Failure_Result, CacheSeed::EmptyProperties, key, notification);
	}

	TEST(TEST_CLASS, FailureWhenAccountIsKnown_Block_Add_RelevantType) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto modification = model::PropertyModification<model::EntityType>{ Add, Relevant_Entity_Type };
		auto notification = test::CreateNotification<TransactionTypePropertyTraits, test::BlockTraits>(key, modification);

		// Act + Assert:
		AssertValidationResult<test::BlockTraits>(Failure_Result, CacheSeed::RandomValue, key, notification);
	}

	// endregion

	// region success

	TEST(TEST_CLASS, SuccessWhenAccountIsUnknown_Allow_Add_RelevantType) {
		// Arrange:
		auto seedKey = test::GenerateRandomByteArray<Key>();
		auto notificationKey = test::GenerateRandomByteArray<Key>();
		auto modification = model::PropertyModification<model::EntityType>{ Add, Relevant_Entity_Type };
		auto notification = test::CreateNotification<TransactionTypePropertyTraits, test::AllowTraits>(notificationKey, modification);

		// Act + Assert:
		AssertValidationResult<test::AllowTraits>(ValidationResult::Success, CacheSeed::No, seedKey, notification);
	}

	TEST(TEST_CLASS, SuccessWhenAccountIsUnknown_Block_Add_NotRelevantType) {
		// Arrange:
		auto seedKey = test::GenerateRandomByteArray<Key>();
		auto notificationKey = test::GenerateRandomByteArray<Key>();
		auto modification = model::PropertyModification<model::EntityType>{ Add, RandomValue() };
		auto notification = test::CreateNotification<TransactionTypePropertyTraits, test::BlockTraits>(notificationKey, modification);

		// Act + Assert:
		AssertValidationResult<test::BlockTraits>(ValidationResult::Success, CacheSeed::No, seedKey, notification);
	}

	TEST(TEST_CLASS, SuccessWhenAccountIsKnown_Allow_Add_RelevantType) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto modification = model::PropertyModification<model::EntityType>{ Add, Relevant_Entity_Type };
		auto notification = test::CreateNotification<TransactionTypePropertyTraits, test::AllowTraits>(key, modification);

		// Act + Assert:
		AssertValidationResult<test::AllowTraits>(ValidationResult::Success, CacheSeed::EmptyProperties, key, notification);
	}

	TEST(TEST_CLASS, SuccessWhenAccountIsKnown_Allow_Add_NotRelevantType_SeededRelevantType) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto modification = model::PropertyModification<model::EntityType>{ Add, RandomValue() };
		auto notification = test::CreateNotification<TransactionTypePropertyTraits, test::AllowTraits>(key, modification);

		// Act + Assert:
		AssertValidationResult<test::AllowTraits>(ValidationResult::Success, CacheSeed::RelevantValue, key, notification);
	}

	TEST(TEST_CLASS, SuccessWhenAccountIsKnown_Block_Add_NotRelevantType) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto modification = model::PropertyModification<model::EntityType>{ Add, RandomValue() };
		auto notification = test::CreateNotification<TransactionTypePropertyTraits, test::BlockTraits>(key, modification);

		// Act + Assert:
		AssertValidationResult<test::BlockTraits>(ValidationResult::Success, CacheSeed::RandomValue, key, notification);
	}

	// endregion
}}
