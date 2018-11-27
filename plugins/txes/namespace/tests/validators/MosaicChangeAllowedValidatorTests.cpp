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
#include "catapult/model/BlockChainConfiguration.h"
#include "tests/test/MosaicCacheTestUtils.h"
#include "tests/test/NamespaceCacheTestUtils.h"
#include "tests/test/NamespaceTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS MosaicChangeAllowedValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(MosaicChangeAllowed,)

	namespace {
		constexpr auto Default_Mosaic_Id = MosaicId(111);
		constexpr auto Namespace_Expiry_Height = Height(123);
		constexpr auto Mosaic_Expiry_Height = Height(150);

		void SeedCacheWithRoot25TreeSigner(cache::NamespaceCacheDelta& namespaceCacheDelta, const Key& signer) {
			// Arrange: create a cache with { 25 } and { 25, 36 }
			namespaceCacheDelta.insert(state::RootNamespace(NamespaceId(25), signer, test::CreateLifetime(10, 123)));
			namespaceCacheDelta.insert(state::Namespace(test::CreatePath({ 25, 36 })));

			// Sanity:
			test::AssertCacheContents(namespaceCacheDelta, { 25, 36 });
		}

		void AssertValidationResult(
				ValidationResult expectedResult,
				MosaicId affectedMosaicId,
				Height height,
				const Key& transactionSigner,
				const Key& artifactOwner) {
			// Arrange:
			auto pValidator = CreateMosaicChangeAllowedValidator();

			// - create the notification
			model::MosaicChangeNotification notification(transactionSigner, affectedMosaicId);

			// - create the validator context
			auto cache = test::MosaicCacheFactory::Create(model::BlockChainConfiguration::Uninitialized());
			auto delta = cache.createDelta();
			test::AddMosaic(delta, NamespaceId(36), Default_Mosaic_Id, Height(50), BlockDuration(100), artifactOwner);
			SeedCacheWithRoot25TreeSigner(delta.sub<cache::NamespaceCache>(), artifactOwner);
			cache.commit(Height());

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, height);

			// Assert:
			EXPECT_EQ(expectedResult, result) << "height " << height << ", id " << affectedMosaicId;
		}

		void AssertValidationResult(ValidationResult expectedResult, MosaicId affectedMosaicId, Height height) {
			auto key = test::GenerateRandomData<Key_Size>();
			AssertValidationResult(expectedResult, affectedMosaicId, height, key, key);
		}
	}

	TEST(TEST_CLASS, CannotChangeUnknownMosaic) {
		// Assert:
		AssertValidationResult(Failure_Mosaic_Expired, Default_Mosaic_Id + MosaicId(1), Height(100));
	}

	TEST(TEST_CLASS, CannotChangeExpiredMosaic) {
		// Assert:
		AssertValidationResult(Failure_Mosaic_Expired, Default_Mosaic_Id, Mosaic_Expiry_Height);
	}

	TEST(TEST_CLASS, CannotChangeMosaicWithExpiredNamespace) {
		// Assert:
		AssertValidationResult(Failure_Namespace_Expired, Default_Mosaic_Id, Namespace_Expiry_Height);
	}

	TEST(TEST_CLASS, CannotChangeActiveMosaicWithWrongOwner) {
		// Assert:
		auto key1 = test::GenerateRandomData<Key_Size>();
		auto key2 = test::GenerateRandomData<Key_Size>();
		AssertValidationResult(Failure_Mosaic_Owner_Conflict, Default_Mosaic_Id, Height(100), key1, key2);
	}

	TEST(TEST_CLASS, CanChangeActiveMosaicWithCorrectOwner) {
		// Assert:
		AssertValidationResult(ValidationResult::Success, Default_Mosaic_Id, Height(100));
	}
}}
