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

#pragma once
#include "catapult/model/BlockChainConfiguration.h"
#include "catapult/validators/NotificationValidator.h"
#include "catapult/validators/ValidatorContext.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/ResolverTestUtils.h"

namespace catapult { namespace test {

	// region CreateValidatorContext

	/// Creates a validator context around a \a height, \a network and \a cache.
	inline validators::ValidatorContext CreateValidatorContext(
			Height height,
			const model::NetworkInfo& network,
			const cache::ReadOnlyCatapultCache& cache) {
		return validators::ValidatorContext(height, Timestamp(0), network, CreateResolverContextXor(), cache);
	}

	/// Creates a validator context around a \a height and \a cache.
	inline validators::ValidatorContext CreateValidatorContext(Height height, const cache::ReadOnlyCatapultCache& cache) {
		return CreateValidatorContext(height, model::NetworkInfo(), cache);
	}

	// endregion

	// region ValidateNotification

	/// Validates \a notification with \a validator.
	template<typename TNotification>
	validators::ValidationResult ValidateNotification(
			const validators::stateless::NotificationValidatorT<TNotification>& validator,
			const TNotification& notification) {
		return validator.validate(notification);
	}

	/// Validates \a notification with \a validator using \a context.
	template<typename TNotification>
	validators::ValidationResult ValidateNotification(
			const validators::stateful::NotificationValidatorT<TNotification>& validator,
			const TNotification& notification,
			const validators::ValidatorContext& context) {
		return validator.validate(notification, context);
	}

	/// Validates \a notification with \a validator using \a cache at \a height.
	template<typename TNotification>
	validators::ValidationResult ValidateNotification(
			const validators::stateful::NotificationValidatorT<TNotification>& validator,
			const TNotification& notification,
			const cache::CatapultCache& cache,
			Height height = Height(1)) {
		auto cacheView = cache.createView();
		auto readOnlyCache = cacheView.toReadOnly();
		auto context = CreateValidatorContext(height, readOnlyCache);
		return validator.validate(notification, context);
	}

	// endregion

/// Defines common validator tests for a validator with \a NAME.
#define DEFINE_COMMON_VALIDATOR_TESTS(NAME, ...) \
	TEST(NAME##ValidatorTests, CanCreate##NAME##Validator) { \
		auto pValidator = Create##NAME##Validator(__VA_ARGS__); \
		EXPECT_EQ(#NAME "Validator", pValidator->name()); \
	}

	// region plugin config validator tests

	namespace {
		template<typename TTraits, VersionType version>
		void AssertPluginConfig(const std::string& pluginName, const utils::ConfigurationBag& bag, validators::ValidationResult expectedResult) {
			// Arrange:
			auto pValidator = TTraits::CreatePluginConfigValidator();
			model::PluginConfigNotification<version> notification(pluginName, bag);

			// Act:
			auto result = ValidateNotification(*pValidator, notification);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}

	}

	/// Asserts success on valid plugin config.
	template<typename TTraits, VersionType version>
	void AssertValidPluginConfig(const std::string& pluginName, validators::ValidationResult expectedResult) {
		AssertPluginConfig<TTraits, version>(pluginName, TTraits::GetValidConfigBag(), expectedResult);
	}

	/// Asserts failure on invalid plugin config.
	template<typename TTraits, VersionType version>
	void AssertInvalidPluginConfig(const std::string& pluginName, validators::ValidationResult expectedResult) {
		AssertPluginConfig<TTraits, version>(pluginName, TTraits::GetInvalidConfigBag(), expectedResult);
	}

	// endregion

#define MAKE_PLUGIN_CONFIG_VALIDATOR_TEST(TEST_CLASS, TEST_TRAITS, VERSION, PLUGIN_SUFFIX, TEST_NAME, EXPECTED_RESULT) \
	TEST(TEST_CLASS, TEST_NAME) { test::Assert##TEST_NAME<TEST_TRAITS, VERSION>("catapult.plugins." #PLUGIN_SUFFIX, EXPECTED_RESULT); }

#define DEFINE_PLUGIN_CONFIG_VALIDATOR_TESTS(TEST_CLASS, TEST_TRAITS, VERSION, PLUGIN_SUFFIX, CONFIG_NAME) \
	MAKE_PLUGIN_CONFIG_VALIDATOR_TEST(TEST_CLASS, TEST_TRAITS, VERSION, PLUGIN_SUFFIX, ValidPluginConfig, validators::ValidationResult::Success) \
	MAKE_PLUGIN_CONFIG_VALIDATOR_TEST(TEST_CLASS, TEST_TRAITS, VERSION, PLUGIN_SUFFIX, InvalidPluginConfig, Failure_##CONFIG_NAME##_Plugin_Config_Malformed)
}}
