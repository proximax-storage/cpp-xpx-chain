/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/MetadataCacheTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS MetadataV1ModificationsValidatorTests

	constexpr uint8_t MaxFields = 5;

	DEFINE_COMMON_VALIDATOR_TESTS(MetadataV1Modifications)

	namespace {
		const Address Raw_Data = test::GenerateRandomByteArray<Address>();
		const model::MetadataV1Type Metadata_Type = model::MetadataV1Type::Address;
		const Hash256 Metadata_Id = state::GetHash(state::ToVector(Raw_Data), Metadata_Type);

		using Fields = std::vector<state::MetadataV1Field>;

		void PopulateCache(cache::CatapultCache& cache, const Fields& initValues) {
			auto delta = cache.createDelta();
			auto& metadataCacheDelta = delta.sub<cache::MetadataV1Cache>();
			auto metadataEntry = state::MetadataV1Entry(state::ToVector(Raw_Data), Metadata_Type);
			metadataEntry.fields().assign(initValues.begin(), initValues.end());

			metadataCacheDelta.insert(metadataEntry);

			cache.commit(Height(1));
		}

		struct Modification {
			std::string Key;
			std::string Value;
			model::MetadataV1ModificationType Type;
		};

		void AssertValidationResult(
				ValidationResult expectedResult,
				const Fields& initValues,
				const Hash256 & metadataId,
				const std::vector<Modification> modifications) {
			// Arrange:
			test::MutableBlockchainConfiguration mutableConfig;
			auto pluginConfig = config::MetadataV1Configuration::Uninitialized();
			pluginConfig.MaxFields = MaxFields;
			mutableConfig.Network.SetPluginConfiguration(pluginConfig);
			auto config = mutableConfig.ToConst();
			auto cache = test::MetadataV1CacheFactory::Create(config);
			PopulateCache(cache, initValues);
			auto pValidator = CreateMetadataV1ModificationsValidator();

			uint32_t sizeOfBuffer = 0;

			for (const auto& modification : modifications)
				sizeOfBuffer += sizeof(model::MetadataV1Modification) + modification.Key.size() + modification.Value.size();

			std::vector<uint8_t> modificationBuffer(sizeOfBuffer);
			std::vector<const model::MetadataV1Modification*> pointers;

			uint8_t* pointer = modificationBuffer.data();
			for (const auto& modification : modifications) {
				auto* pModifications = reinterpret_cast<model::MetadataV1Modification*>(pointer);
				pModifications->KeySize = modification.Key.size();
				pModifications->ValueSize = modification.Value.size();
				pModifications->ModificationType = modification.Type;

				pModifications->Size = sizeof(model::MetadataV1Modification) + modification.Key.size() + modification.Value.size();

				std::memcpy(pointer + sizeof(model::MetadataV1Modification), modification.Key.data(), modification.Key.size());
				std::memcpy(pointer + sizeof(model::MetadataV1Modification) + modification.Key.size(), modification.Value.data(), modification.Value.size());
				pointer += pModifications->Size;
				pointers.emplace_back(pModifications);
			}

			auto notification = model::MetadataV1ModificationsNotification<1>(metadataId, pointers);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, config);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, SuccessModifications_EmpyModifications) {
		// Act:
		AssertValidationResult(
			ValidationResult::Success,
			{},
			Metadata_Id,
			{});
	}

	/// addregion

	TEST(TEST_CLASS, SuccessModifications_AddNewFields_ToExisitngMetadata) {
		// Act:
		AssertValidationResult(
			ValidationResult::Success,
			{},
			Metadata_Id,
			{
				{ "Hello1", "World", model::MetadataV1ModificationType::Add },
				{ "Hello2", "World", model::MetadataV1ModificationType::Add },
				{ "Hello3", "World", model::MetadataV1ModificationType::Add },
			});
	}

	TEST(TEST_CLASS, SuccessModifications_AddNewFields_ToNotExisitngMetadata) {
		// Act:
		AssertValidationResult(
			ValidationResult::Success,
			{},
			test::GenerateRandomByteArray<Hash256>(),
			{
				{ "Hello1", "World", model::MetadataV1ModificationType::Add },
				{ "Hello2", "World", model::MetadataV1ModificationType::Add },
				{ "Hello3", "World", model::MetadataV1ModificationType::Add },
			});
	}

	TEST(TEST_CLASS, SuccessModifications_AddNewFields_ToExisitngMetadata_WithOtherKeys) {
		// Act:
		AssertValidationResult(
			ValidationResult::Success,
			{
				{ "Hello-1", "World", Height(0) },
				{ "Hello0", "World", Height(0) },
			},
			test::GenerateRandomByteArray<Hash256>(),
			{
				{ "Hello1", "World", model::MetadataV1ModificationType::Add },
				{ "Hello2", "World", model::MetadataV1ModificationType::Add },
				{ "Hello3", "World", model::MetadataV1ModificationType::Add },
			});
	}

	TEST(TEST_CLASS, SuccessModifications_AddNewFields_ToExisitngMetadata_Empty) {
		// Act:
		AssertValidationResult(
			ValidationResult::Success,
			{
			},
			test::GenerateRandomByteArray<Hash256>(),
			{
				{ "Hello1", "World", model::MetadataV1ModificationType::Add },
				{ "Hello2", "World", model::MetadataV1ModificationType::Add },
				{ "Hello3", "World", model::MetadataV1ModificationType::Add },
				{ "Hello4", "World", model::MetadataV1ModificationType::Add },
				{ "Hello5", "World", model::MetadataV1ModificationType::Add },
			});
	}

	TEST(TEST_CLASS, FailureModifications_AddNewFields_ToExisitngMetadata_WhenKeysMoreMaxFieldsKeys) {
		// Act:
		AssertValidationResult(
			Failure_Metadata_Too_Much_Keys,
			{
			},
			test::GenerateRandomByteArray<Hash256>(),
			{
				{ "Hello1", "World", model::MetadataV1ModificationType::Add },
				{ "Hello2", "World", model::MetadataV1ModificationType::Add },
				{ "Hello3", "World", model::MetadataV1ModificationType::Add },
				{ "Hello4", "World", model::MetadataV1ModificationType::Add },
				{ "Hello5", "World", model::MetadataV1ModificationType::Add },
				{ "Hello6", "World", model::MetadataV1ModificationType::Add },
			});
	}

	TEST(TEST_CLASS, FailureModifications_AddNewFields_ToExisitngMetadata_WithOtherKeys_WhenKeysMoreMaxFieldsKeys) {
		// Act:
		AssertValidationResult(
			Failure_Metadata_Too_Much_Keys,
			{
				{ "Hello-2", "World", Height(0) },
				{ "Hello-1", "World", Height(0) },
				{ "Hello0", "World", Height(0) },
			},
			Metadata_Id,
			{
				{ "Hello1", "World", model::MetadataV1ModificationType::Add },
				{ "Hello2", "World", model::MetadataV1ModificationType::Add },
				{ "Hello3", "World", model::MetadataV1ModificationType::Add },
			});
	}

	TEST(TEST_CLASS, SuccessModifications_AddNewFields_ToExisitngMetadata_WithOtherKeys_WhenKeysMoreThanMaxFields_ButOneKeyIsRemoved) {
		// Act:
		AssertValidationResult(
			ValidationResult::Success,
			{
				{ "Hello-2", "World", Height(15) },
				{ "Hello-1", "World", Height(0) },
				{ "Hello0", "World", Height(0) },
			},
			Metadata_Id,
			{
				{ "Hello1", "World", model::MetadataV1ModificationType::Add },
				{ "Hello2", "World", model::MetadataV1ModificationType::Add },
				{ "Hello3", "World", model::MetadataV1ModificationType::Add },
			});
	}

	TEST(TEST_CLASS, FailureModifications_AddNewFields_WhereTwoModificationsAreEqual) {
		// Act:
		AssertValidationResult(
			Failure_Metadata_Modification_Key_Redundant,
			{
				{ "Hello-1", "World", Height(0) },
				{ "Hello0", "World", Height(0) },
			},
			Metadata_Id,
			{
				{ "Hello1", "World1", model::MetadataV1ModificationType::Add },
				{ "Hello1", "World2", model::MetadataV1ModificationType::Add },
				{ "Hello2", "World", model::MetadataV1ModificationType::Add },
			});
	}

	TEST(TEST_CLASS, FailureModifications_AddNewFields_ToExistingAccount_WhereModificationEqualsToFiledOfAccount) {
		// Act:
		AssertValidationResult(
			Failure_Metadata_Modification_Value_Redundant,
			{
				{ "Hello-1", "World", Height(0) },
				{ "Hello0", "World", Height(0) },
			},
			Metadata_Id,
			{
				{ "Hello0", "World", model::MetadataV1ModificationType::Add },
				{ "Hello1", "World", model::MetadataV1ModificationType::Add },
				{ "Hello2", "World", model::MetadataV1ModificationType::Add },
			});
	}

	TEST(TEST_CLASS, SuccessModifications_AddNewFields_ToExistingAccount_WhereModificationEqualsToFiledOfAccount_ButAnotherValue) {
		// Act:
		AssertValidationResult(
			ValidationResult::Success,
			{
				{ "Hello-1", "World", Height(0) },
				{ "Hello0", "World", Height(0) },
			},
			Metadata_Id,
			{
				{ "Hello0", "World1", model::MetadataV1ModificationType::Add },
				{ "Hello1", "World", model::MetadataV1ModificationType::Add },
				{ "Hello2", "World", model::MetadataV1ModificationType::Add },
			});
	}

	/// end addregion

	/// removeregion

	TEST(TEST_CLASS, SuccessModifications_RemoveFields_FromExisitngMetadata) {
		// Act:
		AssertValidationResult(
			ValidationResult::Success,
			{
				{ "Hello1", "World", Height(0) },
				{ "Hello2", "World", Height(0) },
				{ "Hello3", "World", Height(0) }
			},
			Metadata_Id,
			{
				{ "Hello1", "", model::MetadataV1ModificationType::Del },
				{ "Hello2", "World231", model::MetadataV1ModificationType::Del },
				{ "Hello3", "World4214", model::MetadataV1ModificationType::Del },
			});
	}

	TEST(TEST_CLASS, FailureModifications_RemoveFields_FromExisitngMetadata_WhenOneFieldsAlreadyRemoved) {
		// Act:
		AssertValidationResult(
			Failure_Metadata_Remove_Not_Existing_Key,
			{
				{ "Hello1", "World", Height(15) },
				{ "Hello2", "World", Height(0) },
				{ "Hello3", "World", Height(0) }
			},
			Metadata_Id,
			{
				{ "Hello1", "", model::MetadataV1ModificationType::Del },
				{ "Hello2", "World231", model::MetadataV1ModificationType::Del },
				{ "Hello3", "World4214", model::MetadataV1ModificationType::Del },
			});
	}

	TEST(TEST_CLASS, FailureModifications_RemoveFields_FromNotExisitngMetadata) {
		// Act:
		AssertValidationResult(
			Failure_Metadata_Remove_Not_Existing_Key,
			{
			},
			test::GenerateRandomByteArray<Hash256>(),
			{
				{ "Hello1", "", model::MetadataV1ModificationType::Del },
			});
	}

	TEST(TEST_CLASS, FailureModifications_RemoveFields_FromExisitngMetadata_Empty) {
		// Act:
		AssertValidationResult(
			Failure_Metadata_Remove_Not_Existing_Key,
			{
			},
			Metadata_Id,
			{
				{ "Hello1", "", model::MetadataV1ModificationType::Del },
			});
	}

	/// end removeregion

	/// mixregion

	TEST(TEST_CLASS, SuccessModifications_RemoveFields_FromExisitngMetadata_AddNewKeys) {
		// Act:
		AssertValidationResult(
			ValidationResult::Success,
			{
				{ "Hello1", "World", Height(0) },
				{ "Hello2", "World", Height(0) },
				{ "Hello3", "World", Height(0) }
			},
			Metadata_Id,
			{
				{ "Hello1", "", model::MetadataV1ModificationType::Del },
				{ "Hello2", "World231", model::MetadataV1ModificationType::Del },
				{ "Hello3", "World4214", model::MetadataV1ModificationType::Del },
				{ "Hello4", "World", model::MetadataV1ModificationType::Add },
				{ "Hello5", "World", model::MetadataV1ModificationType::Add },
				{ "Hello6", "World", model::MetadataV1ModificationType::Add },
				{ "Hello7", "World", model::MetadataV1ModificationType::Add },
				{ "Hello8", "World", model::MetadataV1ModificationType::Add },
			});
	}

	TEST(TEST_CLASS, FailureModifications_RemoveAndAddTheSameField) {
		// Act:
		AssertValidationResult(
			Failure_Metadata_Modification_Key_Redundant,
			{
				{ "Hello1", "World", Height(0) },
				{ "Hello2", "World", Height(0) },
				{ "Hello3", "World", Height(0) }
			},
			Metadata_Id,
			{
				{ "Hello1", "", model::MetadataV1ModificationType::Del },
				{ "Hello1", "World123", model::MetadataV1ModificationType::Add },
			});
	}

	/// end mixregion
}}
