/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/AccountStateCacheStorage.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "src/validators/Validators.h"
#include "src/state/MetadataUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/MetadataCacheTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS MetadataModificationsValidatorTests

	constexpr uint8_t MaxFields = 5;

	DEFINE_COMMON_VALIDATOR_TESTS(MetadataModifications, config::CreateMockConfigurationHolder())

	namespace {
		const Address Raw_Data = test::GenerateRandomByteArray<Address>();
		const model::MetadataType Metadata_Type = model::MetadataType::Address;
		const Hash256 Metadata_Id = state::GetHash(state::ToVector(Raw_Data), Metadata_Type);

		using Fields = std::vector<state::MetadataField>;

		void PopulateCache(cache::CatapultCache& cache, const Fields& initValues) {
			auto delta = cache.createDelta();
			auto& metadataCacheDelta = delta.sub<cache::MetadataCache>();
			auto metadataEntry = state::MetadataEntry(state::ToVector(Raw_Data), Metadata_Type);
			metadataEntry.fields().assign(initValues.begin(), initValues.end());

			metadataCacheDelta.insert(metadataEntry);

			cache.commit(Height(1));
		}

		struct Modification {
			std::string Key;
			std::string Value;
			model::MetadataModificationType Type;
		};

		void AssertValidationResult(
				ValidationResult expectedResult,
				const Fields& initValues,
				const Hash256 & metadataId,
				const std::vector<Modification> modifications) {
			// Arrange:
			auto pluginConfig = config::MetadataConfiguration::Uninitialized();
			pluginConfig.MaxFields = MaxFields;
			auto networkConfig = model::NetworkConfiguration::Uninitialized();
			networkConfig.SetPluginConfiguration(PLUGIN_NAME(metadata), pluginConfig);
			auto cache = test::MetadataCacheFactory::Create(networkConfig);
			PopulateCache(cache, initValues);
			auto pConfigHolder = config::CreateMockConfigurationHolder(networkConfig);
			auto pValidator = CreateMetadataModificationsValidator(pConfigHolder);

			uint32_t sizeOfBuffer = 0;

			for (const auto& modification : modifications)
				sizeOfBuffer += sizeof(model::MetadataModification) + modification.Key.size() + modification.Value.size();

			std::vector<uint8_t> modificationBuffer(sizeOfBuffer);
			std::vector<const model::MetadataModification*> pointers;

			uint8_t* pointer = modificationBuffer.data();
			for (const auto& modification : modifications) {
				auto* pModifications = reinterpret_cast<model::MetadataModification*>(pointer);
				pModifications->KeySize = modification.Key.size();
				pModifications->ValueSize = modification.Value.size();
				pModifications->ModificationType = modification.Type;

				pModifications->Size = sizeof(model::MetadataModification) + modification.Key.size() + modification.Value.size();

				std::memcpy(pointer + sizeof(model::MetadataModification), modification.Key.data(), modification.Key.size());
				std::memcpy(pointer + sizeof(model::MetadataModification) + modification.Key.size(), modification.Value.data(), modification.Value.size());
				pointer += pModifications->Size;
				pointers.emplace_back(pModifications);
			}

			auto notification = model::MetadataModificationsNotification<1>(metadataId, pointers);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache);

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
				{ "Hello1", "World", model::MetadataModificationType::Add },
				{ "Hello2", "World", model::MetadataModificationType::Add },
				{ "Hello3", "World", model::MetadataModificationType::Add },
			});
	}

	TEST(TEST_CLASS, SuccessModifications_AddNewFields_ToNotExisitngMetadata) {
		// Act:
		AssertValidationResult(
			ValidationResult::Success,
			{},
			test::GenerateRandomByteArray<Hash256>(),
			{
				{ "Hello1", "World", model::MetadataModificationType::Add },
				{ "Hello2", "World", model::MetadataModificationType::Add },
				{ "Hello3", "World", model::MetadataModificationType::Add },
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
				{ "Hello1", "World", model::MetadataModificationType::Add },
				{ "Hello2", "World", model::MetadataModificationType::Add },
				{ "Hello3", "World", model::MetadataModificationType::Add },
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
				{ "Hello1", "World", model::MetadataModificationType::Add },
				{ "Hello2", "World", model::MetadataModificationType::Add },
				{ "Hello3", "World", model::MetadataModificationType::Add },
				{ "Hello4", "World", model::MetadataModificationType::Add },
				{ "Hello5", "World", model::MetadataModificationType::Add },
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
				{ "Hello1", "World", model::MetadataModificationType::Add },
				{ "Hello2", "World", model::MetadataModificationType::Add },
				{ "Hello3", "World", model::MetadataModificationType::Add },
				{ "Hello4", "World", model::MetadataModificationType::Add },
				{ "Hello5", "World", model::MetadataModificationType::Add },
				{ "Hello6", "World", model::MetadataModificationType::Add },
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
				{ "Hello1", "World", model::MetadataModificationType::Add },
				{ "Hello2", "World", model::MetadataModificationType::Add },
				{ "Hello3", "World", model::MetadataModificationType::Add },
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
				{ "Hello1", "World", model::MetadataModificationType::Add },
				{ "Hello2", "World", model::MetadataModificationType::Add },
				{ "Hello3", "World", model::MetadataModificationType::Add },
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
				{ "Hello1", "World1", model::MetadataModificationType::Add },
				{ "Hello1", "World2", model::MetadataModificationType::Add },
				{ "Hello2", "World", model::MetadataModificationType::Add },
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
				{ "Hello0", "World", model::MetadataModificationType::Add },
				{ "Hello1", "World", model::MetadataModificationType::Add },
				{ "Hello2", "World", model::MetadataModificationType::Add },
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
				{ "Hello0", "World1", model::MetadataModificationType::Add },
				{ "Hello1", "World", model::MetadataModificationType::Add },
				{ "Hello2", "World", model::MetadataModificationType::Add },
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
				{ "Hello1", "", model::MetadataModificationType::Del },
				{ "Hello2", "World231", model::MetadataModificationType::Del },
				{ "Hello3", "World4214", model::MetadataModificationType::Del },
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
				{ "Hello1", "", model::MetadataModificationType::Del },
				{ "Hello2", "World231", model::MetadataModificationType::Del },
				{ "Hello3", "World4214", model::MetadataModificationType::Del },
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
				{ "Hello1", "", model::MetadataModificationType::Del },
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
				{ "Hello1", "", model::MetadataModificationType::Del },
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
				{ "Hello1", "", model::MetadataModificationType::Del },
				{ "Hello2", "World231", model::MetadataModificationType::Del },
				{ "Hello3", "World4214", model::MetadataModificationType::Del },
				{ "Hello4", "World", model::MetadataModificationType::Add },
				{ "Hello5", "World", model::MetadataModificationType::Add },
				{ "Hello6", "World", model::MetadataModificationType::Add },
				{ "Hello7", "World", model::MetadataModificationType::Add },
				{ "Hello8", "World", model::MetadataModificationType::Add },
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
				{ "Hello1", "", model::MetadataModificationType::Del },
				{ "Hello1", "World123", model::MetadataModificationType::Add },
			});
	}

	/// end mixregion
}}
