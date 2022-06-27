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

#include "src/observers/Observers.h"
#include "src/cache/MosaicCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "tests/test/MosaicCacheTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS MosaicSupplyChangeObserverTests

	using ObserverTestContext = test::ObserverTestContextT<test::MosaicCacheFactory>;

	DEFINE_COMMON_OBSERVER_TESTS(MosaicSupplyChangeV1,)
	DEFINE_COMMON_OBSERVER_TESTS(MosaicSupplyChangeV2,)

	namespace {
		constexpr MosaicId Default_Mosaic_Id(345);

		struct V1TestTraits
		{
			using Notification = model::MosaicSupplyChangeNotification<1>;
			static auto Create(){
				return CreateMosaicSupplyChangeV1Observer();
			}
		};

		struct V2TestTraits
		{
			using Notification = model::MosaicSupplyChangeNotification<2>;
			static auto Create(){
				return CreateMosaicSupplyChangeV2Observer();
			}
		};


		template<typename TTestTraits>
		void AssertSupplyChange(
				model::MosaicSupplyChangeDirection direction,
				NotifyMode mode,
				Amount initialSupply,
				Amount initialOwnerSupply,
				Amount delta,
				Amount finalSupply,
				Amount finalOwnerSupply) {
			// Arrange: create observer and notification
			auto pObserver = TTestTraits::Create();

			auto signer = test::GenerateRandomByteArray<Key>();
			typename TTestTraits::Notification notification(signer, test::UnresolveXor(Default_Mosaic_Id), direction, delta);

			// - initialize cache with a mosaic supply
			ObserverTestContext context(mode, Height(888));
			test::AddMosaic(context.cache(), Default_Mosaic_Id, Height(7), Eternal_Artifact_Duration, initialSupply);
			test::AddMosaicOwner(context.cache(), Default_Mosaic_Id, signer, initialOwnerSupply);

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert:
			const auto& mosaicCacheDelta = context.cache().sub<cache::MosaicCache>();
			EXPECT_EQ(finalSupply, mosaicCacheDelta.find(Default_Mosaic_Id).get().supply());

			const auto& accountStateCacheDelta = context.cache().sub<cache::AccountStateCache>();
			auto signerAddress = accountStateCacheDelta.find(signer).get().Address;
			EXPECT_EQ(finalOwnerSupply, accountStateCacheDelta.find(signerAddress).get().Balances.get(Default_Mosaic_Id));
		}

		template<typename TTestTraits>
		void AssertSupplyIncrease(model::MosaicSupplyChangeDirection direction, NotifyMode mode) {
			// Assert:
			AssertSupplyChange<TTestTraits>(direction, mode, Amount(500), Amount(222), Amount(123), Amount(500 + 123), Amount(222 + 123));
		}

		template<typename TTestTraits>
		void AssertSupplyDecrease(model::MosaicSupplyChangeDirection direction, NotifyMode mode) {
			// Assert:
			AssertSupplyChange<TTestTraits>(direction, mode, Amount(500), Amount(222), Amount(123), Amount(500 - 123), Amount(222 - 123));
		}
	}
#define TRAITS_BASED_TEST(TEST_CLASS, TEST_NAME) \
    template<typename TTestTraits>                                 \
	void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_v1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<V1TestTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_v2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<V2TestTraits>(); } \
    template<typename TTestTraits>                                 \
	void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	TRAITS_BASED_TEST(TEST_CLASS, IncreaseCommitIncreasesSupply) {
		// Assert:
		AssertSupplyIncrease<TTestTraits>(model::MosaicSupplyChangeDirection::Increase, NotifyMode::Commit);
	}

	TRAITS_BASED_TEST(TEST_CLASS, DecreaseCommitDecreasesSupply) {
		// Assert:
		AssertSupplyDecrease<TTestTraits>(model::MosaicSupplyChangeDirection::Decrease, NotifyMode::Commit);
	}

	TRAITS_BASED_TEST(TEST_CLASS, IncreaseRollbackDecreasesSupply) {
		// Assert:
		AssertSupplyDecrease<TTestTraits>(model::MosaicSupplyChangeDirection::Increase, NotifyMode::Rollback);
	}

	TRAITS_BASED_TEST(TEST_CLASS, DecreaseRollbackIncreasesSupply) {
		// Assert:
		AssertSupplyIncrease<TTestTraits>(model::MosaicSupplyChangeDirection::Decrease, NotifyMode::Rollback);
	}
}}
