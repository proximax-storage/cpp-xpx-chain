/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#include "src/observers/Observers.h"
#include "tests/test/cache/BalanceTransferTestUtils.h"
#include "tests/test/plugins/AccountObserverTestContext.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"
#include "catapult/utils/MemoryUtils.h"
#include "src/model/MosaicLevy.h"
#include "catapult/types.h"
#include "src/model/MosaicEntityType.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/test/MosaicCacheTestUtils.h"
#include "tests/test/LevyTestUtils.h"

namespace catapult { namespace observers {

#define TEST_CLASS LevyTransferObserverTests

		DEFINE_COMMON_OBSERVER_TESTS(LevyTransfer,)

		namespace {

			using ObserverTestContext = test::ObserverTestContextT<test::LevyCacheFactory>;

			constexpr UnresolvedMosaicId unresMosaicId(1234);
			auto Currency_Mosaic_Id = MosaicId(unresMosaicId.unwrap());

			template<typename TTraits>
			void AssertCommitObservation() {
				// Arrange:
				ObserverTestContext context(NotifyMode::Commit, Height{444});
				auto pObserver = CreateLevyTransferObserver();

				auto sender = test::GenerateRandomByteArray<Key>();
				auto recipient = test::GenerateRandomByteArray<Address>();
				auto sink = test::GenerateRandomByteArray<Address>();

				auto notification = TTraits::CreateNotification(sender, test::UnresolveXor(recipient));

				test::SetCacheBalances(context.cache(), recipient, TTraits::GetInitialRecipientBalances());
				test::SetCacheBalances(context.cache(), sink, TTraits::GetInitialSinkBalances());

				test::AddMosaicWithLevy(context.cache(), Currency_Mosaic_Id, Height(7),
						model::MosaicLevy(model::LevyType::Absolute, test::UnresolveXor(sink), Currency_Mosaic_Id, Amount(10)));

				test::AddMosaicOwner(context.cache(), Currency_Mosaic_Id, sender, TTraits::GetSenderInitialBalanceAmount());

				// Act:
				test::ObserveNotification(*pObserver, notification, context);

				// Assert:
				test::AssertBalances(context.cache(), sender, TTraits::GetFinalSenderBalances());
				test::AssertBalances(context.cache(), recipient, TTraits::GetFinalRecipientBalances());
				test::AssertBalances(context.cache(), sink, TTraits::GetFinalSinkBalances());
			}

			template<typename TTraits>
			void AssertRollbackObservation() {
				// Arrange:
				ObserverTestContext context(NotifyMode::Rollback, Height{444});
				auto pObserver = CreateLevyTransferObserver();

				auto sender = test::GenerateRandomByteArray<Key>();
				auto recipient = test::GenerateRandomByteArray<Address>();
				auto sink = test::GenerateRandomByteArray<Address>();

				auto notification = TTraits::CreateNotification(sender, test::UnresolveXor(recipient));

				test::AddMosaicWithLevy(context.cache(), Currency_Mosaic_Id, Height(7),
				                        model::MosaicLevy(model::LevyType::Absolute, test::UnresolveXor(sink), Currency_Mosaic_Id, Amount(10)));

				test::SetCacheBalances(context.cache(), recipient, TTraits::GetFinalRecipientBalances());
				test::AddMosaicOwner(context.cache(), Currency_Mosaic_Id, sender, TTraits::GetSenderFinalBalanceAmount());
				test::AddMosaicOwner(context.cache(), Currency_Mosaic_Id, sink, TTraits::GetSinkFinalBalanceAmount());

				// Act:
				test::ObserveNotification(*pObserver, notification, context);

				// Assert:
				test::AssertBalances(context.cache(), sender, TTraits::GetInitialSenderBalances());
				test::AssertBalances(context.cache(), recipient, TTraits::GetInitialRecipientBalances());
				test::AssertBalances(context.cache(), sink, test::BalanceTransfers());      // should be empty
			}
		}

#define DEFINE_BALANCE_OBSERVATION_TESTS(TEST_NAME) \
	TEST(TEST_CLASS, CanTransfer##TEST_NAME##_Commit) { AssertCommitObservation<TEST_NAME##Traits>(); } \
	TEST(TEST_CLASS, CanTransfer##TEST_NAME##_Rollback) { AssertRollbackObservation<TEST_NAME##Traits>(); }

		// region transfer mosaic
		namespace {

			struct TestMosaicTransferTraits {
				static auto CreateNotification(const Key& sender, const UnresolvedAddress& recipient) {
					utils::Mempool pool;
					auto pMosaicLevyData = pool.malloc(model::MosaicLevyData(unresMosaicId));
					return model::BalanceTransferNotification<1>(sender, recipient, test::UnresolveXor(Currency_Mosaic_Id),
					                                             UnresolvedAmount(Amount(234).unwrap(), UnresolvedAmountType::LeviedTransfer, pMosaicLevyData));
				}

				static Amount GetSenderInitialBalanceAmount() {
					return Amount(1000);
				}

				static Amount GetSenderFinalBalanceAmount() {
					return Amount(1000-10);
				}

				static Amount GetSinkFinalBalanceAmount() {
					return Amount(10);
				}

				static test::BalanceTransfers GetInitialSenderBalances() {
					return { { Currency_Mosaic_Id, GetSenderInitialBalanceAmount() } };
				}

				static test::BalanceTransfers GetFinalSenderBalances() {
					return { { Currency_Mosaic_Id, GetSenderFinalBalanceAmount() } };
				}

				static test::BalanceTransfers GetInitialRecipientBalances() {
					return { { Currency_Mosaic_Id, Amount(750) } };
				}

				static test::BalanceTransfers GetFinalRecipientBalances() {
					return { { Currency_Mosaic_Id, Amount(750 ) } };
				}

				static test::BalanceTransfers GetInitialSinkBalances() {
					return { { Currency_Mosaic_Id, Amount(0) } };
				}

				static test::BalanceTransfers GetFinalSinkBalances() {
					return { { Currency_Mosaic_Id, GetSinkFinalBalanceAmount() } };
				}
			};
		}

		DEFINE_BALANCE_OBSERVATION_TESTS(TestMosaicTransfer)

		// endregion

	}}
