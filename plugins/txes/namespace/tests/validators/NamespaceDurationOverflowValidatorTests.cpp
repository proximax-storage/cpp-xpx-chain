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
#include "tests/test/NamespaceCacheTestUtils.h"
#include "tests/test/NamespaceTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS NamespaceDurationOverflowValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(NamespaceDurationOverflow)

	namespace {
		// region test utils

		struct TestOptions {
			BlockDuration MaxDuration;
			BlockDuration GracePeriodDuration;
			catapult::Height Height;
		};

		void RunRootTest(
				ValidationResult expectedResult,
				const model::RootNamespaceNotification<1>& notification,
				const TestOptions& options) {
			// Arrange:
			test::MutableBlockchainConfiguration mutableConfig;
			auto pluginConfig = config::NamespaceConfiguration::Uninitialized();
			pluginConfig.NamespaceGracePeriodDuration = utils::BlockSpan::FromHours(options.GracePeriodDuration.unwrap());
			pluginConfig.MaxNamespaceDuration = utils::BlockSpan::FromHours(options.MaxDuration.unwrap());
			mutableConfig.Network.BlockGenerationTargetTime = utils::TimeSpan::FromHours(1);
			mutableConfig.Network.SetPluginConfiguration(pluginConfig);
			auto config = mutableConfig.ToConst();

			auto cache = test::NamespaceCacheFactory::Create(config,
			        options.GracePeriodDuration, pluginConfig.MaxNamespaceDuration );
			{
				auto cacheDelta = cache.createDelta();
				auto& namespaceCacheDelta = cacheDelta.sub<cache::NamespaceCache>();

				// - create a cache with { 25 }
				auto signer = test::GenerateRandomByteArray<Key>();
				namespaceCacheDelta.insert(state::RootNamespace(NamespaceId(25), signer, test::CreateLifetime(10, 20)));

				cache.commit(Height());
			}

			auto pValidator = CreateNamespaceDurationOverflowValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, config, options.Height);

			// Assert:
			EXPECT_EQ(expectedResult, result)
					<< "MaxDuration " << options.MaxDuration
					<< ", GracePeriodDuration " << options.GracePeriodDuration
					<< ", Height " << options.Height
					<< ", duration " << notification.Duration;
		}

		// endregion
	}

	// region new

	namespace {
		void AssertCanAddWithDuration(BlockDuration duration) {
			// Arrange:
			TestOptions options;
			options.MaxDuration = BlockDuration(105);
			options.GracePeriodDuration = BlockDuration(25);

			for (auto height : { Height(1), Height(15), Height(999) }) {
				options.Height = height;

				// Act: try to create a (new) root with a specified duration
				auto notification = model::RootNamespaceNotification<1>(Key(), NamespaceId(26), duration);
				RunRootTest(ValidationResult::Success, notification, options);
			}
		}
	}

	TEST(TEST_CLASS, CanAddNewRootNamespaceWithEternalDuration) {
		// Assert:
		AssertCanAddWithDuration(Eternal_Artifact_Duration);
	}

	TEST(TEST_CLASS, CanAddNewRootNamespaceWithNonEternalDuration) {
		// Assert:
		AssertCanAddWithDuration(BlockDuration(111));
	}

	TEST(TEST_CLASS, CanAddNewRootNamespaceWithNonEternalMaxDuration) {
		// Arrange:
		TestOptions options;
		options.MaxDuration = BlockDuration(105);
		options.GracePeriodDuration = BlockDuration(25);
		options.Height = Height(std::numeric_limits<uint64_t>::max()) - Height(111);

		// Act: try to create a (new) root with a maximum duration [Height + Duration == Max]
		auto notification = model::RootNamespaceNotification<1>(Key(), NamespaceId(26), BlockDuration(111));
		RunRootTest(ValidationResult::Success, notification, options);
	}

	TEST(TEST_CLASS, CannotAddNewRootNamespaceWithDurationThatCausesOverflow) {
		// Arrange:
		TestOptions options;
		options.MaxDuration = BlockDuration(105);
		options.GracePeriodDuration = BlockDuration(25);

		for (auto heightAdjustment : { Height(1), Height(15) }) {
			options.Height = Height(std::numeric_limits<uint64_t>::max()) - Height(111) + heightAdjustment;

			// Act: try to create a (new) root with a duration too large [Height + Duration > Max]
			auto notification = model::RootNamespaceNotification<1>(Key(), NamespaceId(26), BlockDuration(111));
			RunRootTest(Failure_Namespace_Invalid_Duration, notification, options);
		}
	}

	// endregion

	// region existing, expired

	TEST(TEST_CLASS, CanRenewRootNamespaceAfterGracePeriodExpiration) {
		// Arrange:
		TestOptions options;
		options.MaxDuration = BlockDuration(105);
		options.GracePeriodDuration = BlockDuration(25);

		// - namespace is deactivated at height 20 and grace period is 25, so it is available starting at 45
		for (auto height : { Height(45), Height(100) }) {
			options.Height = height;

			// Act: renew an expired root
			auto notification = model::RootNamespaceNotification<1>(Key(), NamespaceId(25), BlockDuration(111));
			RunRootTest(ValidationResult::Success, notification, options);
		}
	}

	TEST(TEST_CLASS, CanRenewRootNamespaceAfterGracePeriodExpirationMaxDuration) {
		// Arrange:
		TestOptions options;
		options.MaxDuration = BlockDuration(105);
		options.GracePeriodDuration = BlockDuration(25);
		options.Height = Height(std::numeric_limits<uint64_t>::max()) - Height(111);

		// Act: renew an expired root with a maximum duration [Height + Duration == Max]
		auto notification = model::RootNamespaceNotification<1>(Key(), NamespaceId(25), BlockDuration(111));
		RunRootTest(ValidationResult::Success, notification, options);
	}

	TEST(TEST_CLASS, CannotRenewRootNamespaceAfterGracePeriodExpirationWithDurationThatCausesOverflow) {
		// Arrange:
		TestOptions options;
		options.MaxDuration = BlockDuration(105);
		options.GracePeriodDuration = BlockDuration(25);

		for (auto heightAdjustment : { Height(1), Height(15) }) {
			options.Height = Height(std::numeric_limits<uint64_t>::max()) - Height(111) + heightAdjustment;

			// Act: renew an expired root with a duration too large [Height + Duration > Max]
			auto notification = model::RootNamespaceNotification<1>(Key(), NamespaceId(25), BlockDuration(111));
			RunRootTest(Failure_Namespace_Invalid_Duration, notification, options);
		}
	}

	// endregion

	// region existing, renewable

	TEST(TEST_CLASS, CanRenewRootNamespaceBeforeGracePeriodExpiration) {
		// Arrange:
		TestOptions options;
		options.MaxDuration = BlockDuration(105);
		options.GracePeriodDuration = BlockDuration(25);

		// - namespace is deactivated at height 20 and grace period is 25, so it is available starting at 45
		for (auto height : { Height(15), Height(20), Height(44) }) {
			options.Height = height;

			// Act: try to renew / extend a root that has not yet exceeded its grace period
			auto notification = model::RootNamespaceNotification<1>(Key(), NamespaceId(25), BlockDuration(111));
			RunRootTest(ValidationResult::Success, notification, options);
		}
	}

	TEST(TEST_CLASS, CanRenewRootNamespaceBeforeGracePeriodExpirationMaxDuration) {
		// Arrange: [MaxDuration + GracePeriodDuration + Height == Max]
		TestOptions options;
		options.MaxDuration = BlockDuration((std::numeric_limits<uint64_t>::max() - 25 - 16) / utils::TimeSpan::FromHours(1).millis());
		options.GracePeriodDuration = BlockDuration(25);
		options.Height = Height(16);

		// Act: try to renew / extend a root that has not yet exceeded its grace period with a maximum duration
		//      [lifetime.End(20) + Duration == Max]
		auto duration = BlockDuration((std::numeric_limits<uint64_t>::max() - 20) / utils::TimeSpan::FromHours(1).millis());
		auto notification = model::RootNamespaceNotification<1>(Key(), NamespaceId(25), duration);
		RunRootTest(ValidationResult::Success, notification, options);
	}

	TEST(TEST_CLASS, CanRenewRootNamespaceBeforeGracePeriodExpirationMaxDurationMaxLifetimeOverflow) {
		// Arrange: [MaxDuration + GracePeriodDuration + Height > Max]
		TestOptions options;
		options.MaxDuration = BlockDuration((std::numeric_limits<uint64_t>::max() - 25) / utils::TimeSpan::FromHours(1).millis());
		options.GracePeriodDuration = BlockDuration(25);
		options.Height = Height(16);

		// Act: try to renew / extend a root that has not yet exceeded its grace period with a maximum duration
		//      [lifetime.End(20) + Duration == Max]
		auto duration = BlockDuration((std::numeric_limits<uint64_t>::max() - 20) / utils::TimeSpan::FromHours(1).millis());
		auto notification = model::RootNamespaceNotification<1>(Key(), NamespaceId(25), duration);
		RunRootTest(ValidationResult::Success, notification, options);
	}

	TEST(TEST_CLASS, CannotRenewRootNamespaceBeforeGracePeriodExpirationWithDurationThatCausesOverflow) {
		// Arrange:
		TestOptions options;
		options.MaxDuration = BlockDuration(std::numeric_limits<uint64_t>::max() - 25 - 16);
		options.GracePeriodDuration = BlockDuration(25);
		options.Height = Height(16);

		for (auto durationAdjustment : { BlockDuration(1), BlockDuration(3) }) {
			// Act: try to renew / extend a root that has not yet exceeded its grace period with a duration too large
			//      [lifetime.End(20) + Duration > Max]
			auto duration = BlockDuration(std::numeric_limits<uint64_t>::max() - 20) + durationAdjustment;
			auto notification = model::RootNamespaceNotification<1>(Key(), NamespaceId(25), duration);
			RunRootTest(Failure_Namespace_Invalid_Duration, notification, options);
		}
	}

	// endregion

	// region existing, renewable, max check

	TEST(TEST_CLASS, CannotRenewRootNamespaceWithDurationTooLarge) {
		// Arrange:
		TestOptions options;
		options.MaxDuration = BlockDuration(105);
		options.GracePeriodDuration = BlockDuration(25);
		options.Height = Height(15);

		// - max duration is 125 [MaxDuration(105) + GracePeriodDuration(25) + Height(15) - lifetime.End(20)]
		for (auto duration : { BlockDuration(126), BlockDuration(200) }) {
			// Act:
			auto notification = model::RootNamespaceNotification<1>(Key(), NamespaceId(25), duration);
			RunRootTest(Failure_Namespace_Invalid_Duration, notification, options);
		}
	}

	TEST(TEST_CLASS, CanRenewRootNamespaceWithAcceptableDuration) {
		// Arrange:
		TestOptions options;
		options.MaxDuration = BlockDuration(105);
		options.GracePeriodDuration = BlockDuration(25);
		options.Height = Height(15);

		// - max duration is 125 [MaxDuration(105) + GracePeriodDuration(25) + Height(15) - lifetime.End(20)]
		for (auto duration : { BlockDuration(20), BlockDuration(75), BlockDuration(125) }) {
			// Act:
			auto notification = model::RootNamespaceNotification<1>(Key(), NamespaceId(25), duration);
			RunRootTest(ValidationResult::Success, notification, options);
		}
	}

	// endregion
}}
