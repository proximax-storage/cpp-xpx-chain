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
#include "src/config/NamespaceConfiguration.h"
#include "src/model/NamespaceIdGenerator.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS NamespaceNameValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(NamespaceName)

	namespace {
		model::NamespaceNameNotification<1> CreateNamespaceNameNotification(uint8_t nameSize, const uint8_t* pName) {
			auto notification = model::NamespaceNameNotification<1>(Key(),NamespaceId(), NamespaceId(777), nameSize, pName);
			notification.NamespaceId = model::GenerateNamespaceId(NamespaceId(777), reinterpret_cast<const char*>(pName));
			return notification;
		}

		auto CreateConfig(uint8_t maxNameSize, const Key& nemesis, const std::unordered_set<std::string>& reservedRootNamespaceNames) {
			auto pluginConfig = config::NamespaceConfiguration::Uninitialized();
			pluginConfig.MaxNameSize = maxNameSize;
			pluginConfig.ReservedRootNamespaceNames = reservedRootNamespaceNames;
			test::MutableBlockchainConfiguration config;
			config.Network.SetPluginConfiguration(pluginConfig);
			config.Network.Info.PublicKey = nemesis;
			return config.ToConst();
		}

		auto CreateConfig(uint8_t maxNameSize, const std::unordered_set<std::string>& reservedRootNamespaceNames) {
			return CreateConfig(maxNameSize, test::GenerateRandomByteArray<Key>(), reservedRootNamespaceNames);
		}
	}

	// region name size

	namespace {
		void AssertSizeValidationResult(ValidationResult expectedResult, uint8_t nameSize, uint8_t maxNameSize) {
			// Arrange:
			auto config = CreateConfig(maxNameSize, {});
			auto cache = test::CreateEmptyCatapultCache(config);
			auto pValidator = CreateNamespaceNameValidator();
			auto name = std::string(nameSize, 'a');
			auto notification = CreateNamespaceNameNotification(nameSize, reinterpret_cast<const uint8_t*>(name.data()));

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, config);

			// Assert:
			EXPECT_EQ(expectedResult, result)
					<< "nameSize " << static_cast<uint16_t>(nameSize)
					<< ", maxNameSize " << static_cast<uint16_t>(maxNameSize);
		}
	}

	TEST(TEST_CLASS, SuccessWhenValidatingNamespaceWithNameSizeLessThanMax) {
		// Assert:
		AssertSizeValidationResult(ValidationResult::Success, 100, 123);
	}

	TEST(TEST_CLASS, SuccessWhenValidatingNamespaceWithNameSizeEqualToMax) {
		// Assert:
		AssertSizeValidationResult(ValidationResult::Success, 123, 123);
	}

	TEST(TEST_CLASS, FailureWhenValidatingNamespaceWithNameSizeGreaterThanMax) {
		// Assert:
		AssertSizeValidationResult(Failure_Namespace_Invalid_Name, 124, 123);
		AssertSizeValidationResult(Failure_Namespace_Invalid_Name, 200, 123);
	}

	TEST(TEST_CLASS, FailureWhenValidatingEmptyNamespaceName) {
		// Assert:
		AssertSizeValidationResult(Failure_Namespace_Invalid_Name, 0, 123);
	}

	// endregion

	// region name characters

	namespace {
		void AssertNameValidationResult(ValidationResult expectedResult, const std::string& name) {
			// Arrange:
			auto config = CreateConfig(static_cast<uint8_t>(name.size()), {});
			auto cache = test::CreateEmptyCatapultCache(config);
			auto pValidator = CreateNamespaceNameValidator();
			auto notification = CreateNamespaceNameNotification(
					static_cast<uint8_t>(name.size()),
					reinterpret_cast<const uint8_t*>(name.data()));

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, config);

			// Assert:
			EXPECT_EQ(expectedResult, result) << "namespace with name " << name;
		}
	}

	TEST(TEST_CLASS, SuccessWhenValidatingValidNamespaceNames) {
		// Assert:
		for (const auto& name : { "a", "be", "cat", "doom", "al-ce", "al_ce", "alice-", "alice_" })
			AssertNameValidationResult(ValidationResult::Success, name);
	}

	TEST(TEST_CLASS, FailureWhenValidatingInvalidNamespaceNames) {
		// Assert:
		for (const auto& name : { "-alice", "_alice", "al.ce", "alIce", "al ce", "al@ce", "al#ce", "!@#$%" })
			AssertNameValidationResult(Failure_Namespace_Invalid_Name, name);
	}

	// endregion

	// region name and id consistency

	TEST(TEST_CLASS, SuccessWhenValidatingNamespaceWithMatchingNameAndId) {
		// Arrange: note that CreateNamespaceNameNotification creates proper id
		auto config = CreateConfig(100, {});
		auto cache = test::CreateEmptyCatapultCache(config);
		auto pValidator = CreateNamespaceNameValidator();
		auto name = std::string(10, 'a');
		auto notification = CreateNamespaceNameNotification(
				static_cast<uint8_t>(name.size()),
				reinterpret_cast<const uint8_t*>(name.data()));

		// Act:
		auto result = test::ValidateNotification(*pValidator, notification, cache, config);

		// Assert:
		EXPECT_EQ(ValidationResult::Success, result);
	}

	TEST(TEST_CLASS, FailureWhenValidatingNamespaceWithMismatchedNameAndId) {
		// Arrange: corrupt the id
		auto config = CreateConfig(100, {});
		auto cache = test::CreateEmptyCatapultCache(config);
		auto pValidator = CreateNamespaceNameValidator();
		auto name = std::string(10, 'a');
		auto notification = CreateNamespaceNameNotification(
				static_cast<uint8_t>(name.size()),
				reinterpret_cast<const uint8_t*>(name.data()));
		notification.NamespaceId = notification.NamespaceId + NamespaceId(1);

		// Act:
		auto result = test::ValidateNotification(*pValidator, notification, cache, config);

		// Assert:
		EXPECT_EQ(Failure_Namespace_Name_Id_Mismatch, result);
	}

	// endregion

	// region reserved root names

	namespace {
		model::NamespaceNameNotification<1> CreateRootNamespaceNotification(const std::string& name, const Key& signer) {
			auto nameSize = static_cast<uint8_t>(name.size());
			const auto* pName = reinterpret_cast<const uint8_t*>(name.data());
			auto notification = model::NamespaceNameNotification<1>(signer, NamespaceId(), Namespace_Base_Id, nameSize, pName);
			notification.NamespaceId = model::GenerateNamespaceId(Namespace_Base_Id, name);
			return notification;
		}

		model::NamespaceNameNotification<1> CreateChildNamespaceNotification(const std::string& parentName, const Key& signer) {
			const auto* pChildName = reinterpret_cast<const uint8_t*>("alice");
			auto notification = model::NamespaceNameNotification<1>(signer, NamespaceId(), NamespaceId(), 5, pChildName);
			notification.ParentId = model::GenerateNamespaceId(Namespace_Base_Id, parentName);
			notification.NamespaceId = model::GenerateNamespaceId(notification.ParentId, reinterpret_cast<const char*>(pChildName));
			return notification;
		}

		void AssertReservedRootNamesValidationResult(
				ValidationResult expectedResult,
				bool byNemesis,
				const std::string& name,
				const std::function<model::NamespaceNameNotification<1>(const std::string&, const Key&)>& createNotification) {
			// Arrange:
			auto config = CreateConfig(20, { "foo", "foobar" });
			auto cache = test::CreateEmptyCatapultCache(config);
			auto pValidator = CreateNamespaceNameValidator();

			auto signer = test::GenerateRandomByteArray<Key>();
			if (byNemesis) {
				signer = config.Network.Info.PublicKey;
			}
			auto notification = createNotification(name, signer);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, config);

			// Assert:
			EXPECT_EQ(expectedResult, result) << "namespace with name " << name;
		}
	}

	TEST(TEST_CLASS, SuccessWhenValidatingAllowedRootNamespaceNames) {
		// Assert:
		for (const auto& name : { "fo", "foo123", "bar", "bazz", "qux" })
			AssertReservedRootNamesValidationResult(ValidationResult::Success, false, name, CreateRootNamespaceNotification);
	}

	TEST(TEST_CLASS, FailureWhenValidatingReservedRootNamespaceNames) {
		// Assert:
		for (const auto& name : { "foo", "foobar" })
			AssertReservedRootNamesValidationResult(Failure_Namespace_Root_Name_Reserved, false, name, CreateRootNamespaceNotification);
	}

	TEST(TEST_CLASS, SuccessWhenValidatingChildNamespaceWithAllowedParentNamespaceNames) {
		// Assert:
		for (const auto& name : { "fo", "foo123", "bar", "bazz", "qux" })
			AssertReservedRootNamesValidationResult(ValidationResult::Success, false, name, CreateChildNamespaceNotification);
	}

	TEST(TEST_CLASS, FailureWhenValidatingChildNamespaceWithReservedParentNamespaceNames) {
		// Assert:
		for (const auto& name : { "foo", "foobar" })
			AssertReservedRootNamesValidationResult(Failure_Namespace_Root_Name_Reserved, false, name, CreateChildNamespaceNotification);
	}

	TEST(TEST_CLASS, FailureWhenValidatingChildNamespaceWithReservedParentNamespaceNamesByNemesis) {
		// Assert:
		for (const auto& name : { "foo", "foobar" })
			AssertReservedRootNamesValidationResult(ValidationResult::Success, true, name, CreateChildNamespaceNotification);
	}

	// endregion
}}
