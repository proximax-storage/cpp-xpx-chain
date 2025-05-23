/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "plugins/txes/namespace/src/config/NamespaceConfiguration.h"
#include "src/validators/Validators.h"
#include "tests/test/MetadataCacheTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS ModifyNamespaceMetadataValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(ModifyNamespaceMetadata,)

	namespace {
		constexpr NamespaceId CreateValidNamespaceId(uint64_t value) {
			return NamespaceId((1ull << 63) | value);
		}

		const Key Namespace_Owner = test::GenerateRandomByteArray<Key>();
		constexpr NamespaceId Root_Namespace_Id = CreateValidNamespaceId(25);
		constexpr NamespaceId Child_Namespace_Id = CreateValidNamespaceId(36);
		constexpr NamespaceId Child_Child_Namespace_Id = CreateValidNamespaceId(49);

		state::Namespace::Path CreatePath(const std::vector<NamespaceId::ValueType>& ids) {
			state::Namespace::Path path;
			for (auto id : ids)
				path.push_back(NamespaceId(id));

			return path;
		}

		void PopulateCache(cache::CatapultCache& cache) {
			auto delta = cache.createDelta();
			auto& namespaceCacheDelta = delta.sub<cache::NamespaceCache>();

			namespaceCacheDelta.insert(state::RootNamespace(Root_Namespace_Id, Namespace_Owner, state::NamespaceLifetime(Height(10), Height(20))));
			namespaceCacheDelta.insert(state::Namespace(CreatePath({ Root_Namespace_Id.unwrap(), Child_Namespace_Id.unwrap() })));
			namespaceCacheDelta.insert(state::Namespace(CreatePath({
				Root_Namespace_Id.unwrap(),
				Child_Namespace_Id.unwrap(),
				Child_Child_Namespace_Id.unwrap()
			})));

			cache.commit(Height(1));
		}

		void AssertValidationResult(
				ValidationResult expectedResult,
				const NamespaceId& metadataId,
				Key signer) {
			// Arrange:
			test::MutableBlockchainConfiguration config;
			config.Network.BlockGenerationTargetTime = utils::TimeSpan::FromHours(1);
			auto namespacePluginConfig = config::NamespaceConfiguration::Uninitialized();
			namespacePluginConfig.NamespaceGracePeriodDuration = utils::BlockSpan::FromHours(100);
			auto metadataPluginConfig = config::MetadataConfiguration::Uninitialized();
			config.Network.SetPluginConfiguration(namespacePluginConfig);
			config.Network.SetPluginConfiguration(metadataPluginConfig);
			auto cache = test::MetadataCacheFactory::Create(config.ToConst());
			PopulateCache(cache);
			auto pValidator = CreateModifyNamespaceMetadataValidator();
			auto notification = model::ModifyNamespaceMetadataNotification_v1(signer, metadataId);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, SuccessWhenNamespaceExistsAndOwnerEquals) {
		// Act:
		AssertValidationResult(
			ValidationResult::Success,
			Root_Namespace_Id,
			Namespace_Owner);
	}

	TEST(TEST_CLASS, FailureWhenNamespaceIdMalformed) {
		// Act:
		AssertValidationResult(
			Failure_Metadata_NamespaceId_Malformed,
			NamespaceId(1), // high bit not set
			Namespace_Owner);
	}

	TEST(TEST_CLASS, FailureWhenNamespaceNotExistAndOwnerEquals) {
		// Act:
		AssertValidationResult(
			Failure_Metadata_Namespace_Not_Found,
			CreateValidNamespaceId(3),
			Namespace_Owner);
	}

	TEST(TEST_CLASS, FailureWhenNamespaceExistButOwnerNotEqual) {
		// Act:
		AssertValidationResult(
			Failure_Metadata_Namespace_Modification_Not_Permitted,
			Root_Namespace_Id,
			test::GenerateRandomByteArray<Key>());
	}

	TEST(TEST_CLASS, FailureWhenNamespaceNoExistButOwnerNotEquals) {
		// Act:
		AssertValidationResult(
			Failure_Metadata_Namespace_Not_Found,
			CreateValidNamespaceId(3),
			test::GenerateRandomByteArray<Key>());
	}
}}
