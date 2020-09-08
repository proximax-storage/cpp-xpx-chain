/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/EndExecuteTransaction.h"
#include "src/observers/Observers.h"
#include "tests/test/cache/BalanceTransferTestUtils.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"

namespace catapult { namespace observers {

#define TEST_CLASS EndExecuteCosignersObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(EndExecuteCosigners, )

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::SuperContractCacheFactory>;
		using Notification = model::AggregateCosignaturesNotification<2>;

		constexpr auto Num_Mosaics = 5u;
		const auto Current_Height = Height(123);
		const auto Super_Contract_Key = test::GenerateRandomByteArray<Key>();

		const std::vector<model::Cosignature> Cosignatures{
			{ test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Signature>() },
			{ test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Signature>() },
			{ test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Signature>() },
		};

		const std::vector<model::Mosaic> Initial_Balances{
			{ MosaicId(1), Amount(100) },
			{ MosaicId(2), Amount(100) },
			{ MosaicId(3), Amount(100) },
			{ MosaicId(4), Amount(100) },
			{ MosaicId(5), Amount(100) },
		};

		const std::vector<std::vector<model::Mosaic>> Final_Balances{
			{
				{ MosaicId(1), Amount(104) },
				{ MosaicId(2), Amount(107) },
				{ MosaicId(3), Amount(110) },
				{ MosaicId(4), Amount(114) },
				{ MosaicId(5), Amount(117) },
			},
			{
				{ MosaicId(1), Amount(103) },
				{ MosaicId(2), Amount(107) },
				{ MosaicId(3), Amount(110) },
				{ MosaicId(4), Amount(113) },
				{ MosaicId(5), Amount(117) },
			},
			{
				{ MosaicId(1), Amount(103) },
				{ MosaicId(2), Amount(106) },
				{ MosaicId(3), Amount(110) },
				{ MosaicId(4), Amount(113) },
				{ MosaicId(5), Amount(116) },
			},
		};

		auto CreateEndExecuteTransaction() {
			auto pTransaction = test::CreateEndExecuteTransaction<model::EmbeddedEndExecuteTransaction>(Num_Mosaics);
			pTransaction->Signer = Super_Contract_Key;
			return pTransaction;
		}
	}

	TEST(TEST_CLASS, EndExecuteCosigners_Commit) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Commit, Current_Height);
		auto pTransaction = CreateEndExecuteTransaction();
		Notification notification(test::GenerateRandomByteArray<Key>(), 1, pTransaction.get(), Cosignatures.size(), Cosignatures.data());
		auto pObserver = CreateEndExecuteCosignersObserver();

		// Populate cache.
		for (const auto& cosignature : Cosignatures)
			test::SetCacheBalances(context.cache(), cosignature.Signer, Initial_Balances);

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		for (auto i = 0u; i < Cosignatures.size(); ++i)
			test::AssertBalances(context.cache(), Cosignatures[i].Signer, Final_Balances[i]);
	}

	TEST(TEST_CLASS, EndExecuteCosigners_Rollback) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Rollback, Current_Height);
		auto pTransaction = CreateEndExecuteTransaction();
		Notification notification(test::GenerateRandomByteArray<Key>(), 1, pTransaction.get(), Cosignatures.size(), Cosignatures.data());
		auto pObserver = CreateEndExecuteCosignersObserver();

		// Populate cache.
		for (auto i = 0u; i < Cosignatures.size(); ++i)
			test::SetCacheBalances(context.cache(), Cosignatures[i].Signer, Final_Balances[i]);

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		for (const auto& cosignature : Cosignatures)
			test::AssertBalances(context.cache(), cosignature.Signer, Initial_Balances);
	}
}}
