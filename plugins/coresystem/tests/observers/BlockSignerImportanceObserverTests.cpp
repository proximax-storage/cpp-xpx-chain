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
#include "catapult/model/InflationCalculator.h"
#include "tests/test/cache/BalanceTransferTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/core/NotificationTestUtils.h"
#include "tests/test/plugins/AccountObserverTestContext.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS BlockSignerImportanceObserverTests

		DEFINE_COMMON_OBSERVER_TESTS(BlockSignerImportance,)

		namespace {
			constexpr MosaicId Currency_Mosaic_Id(1234);

			static auto& AddAccount(cache::AccountStateCacheDelta& delta, const Key& publicKey, Height height, Amount normal, Amount locked) {
				delta.addAccount(publicKey, height);
				auto accountIter =  delta.find(publicKey);
				auto& account = accountIter.get();
				account.AccountType = state::AccountType::Main;
				account.Balances.credit(Currency_Mosaic_Id, normal+locked);
				account.Balances.lock(Currency_Mosaic_Id, locked);
				model::BalanceSnapshot snapshot{Amount(1000), Amount(100), height};
				account.Balances.addSnapshot(snapshot);
				return account;
			}
		}
		// region fee credit/debit

		namespace {
			void AssertReceipt(
					Amount expectedAmount,
					Amount expectedLockedAmount,
					const model::SignerBalanceReceipt& receipt) {
				ASSERT_EQ(sizeof(model::SignerBalanceReceipt), receipt.Size);
				EXPECT_EQ(1u, receipt.Version);
				EXPECT_EQ(model::Receipt_Type_Block_Signer_Importance, receipt.Type);
				EXPECT_EQ(expectedAmount, receipt.Amount);
				EXPECT_EQ(expectedLockedAmount, receipt.LockedAmount);
			}

			template<typename TAction>
			void RunSignerImportanceObserverTest(
					NotifyMode notifyMode,
					std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder,
					TAction action) {
				test::AccountObserverTestContext context(notifyMode, Height{444}, pConfigHolder->Config());
				auto pObserver = CreateBlockSignerImportanceObserver();

				// Act + Assert:
				action(context, *pObserver);
			}

			template<typename TAction>
			void RunSignerImportanceObserverTest(NotifyMode notifyMode, TAction action) {
				test::MutableBlockchainConfiguration mutableConfig;
				mutableConfig.Immutable.CurrencyMosaicId = Currency_Mosaic_Id;
				mutableConfig.Network.ImportanceGrouping = 5;
				auto config = mutableConfig.ToConst();
				RunSignerImportanceObserverTest(notifyMode, config::CreateMockConfigurationHolder(config), action);
			}
		}

		TEST(TEST_CLASS, BlockGeneratesValidReceipt) {
			// Arrange:
			RunSignerImportanceObserverTest(NotifyMode::Commit, [](auto& context, const auto& observer) {
			  auto signerPublicKey = test::GenerateRandomByteArray<Key>();
			  auto& accountStateCache = context.cache().template sub<cache::AccountStateCache>();
			  auto& accountStateIter = AddAccount(accountStateCache, signerPublicKey, Height(1), Amount(1000), Amount(100));
			  auto notification = model::BlockSignerImportanceNotification<1>(signerPublicKey);

			  // Act:
			  test::ObserveNotification(observer, notification, context);

			  // Assert:
			  test::AssertBalances(context.cache(), signerPublicKey, { { Currency_Mosaic_Id, Amount(1000) } });

			  // - check receipt
			  auto pStatement = context.statementBuilder().build();
			  ASSERT_EQ(1u, pStatement->BlockchainStateStatements.size());
			  const auto& receiptPair = *pStatement->BlockchainStateStatements.find(model::ReceiptSource());
			  ASSERT_EQ(1u, receiptPair.second.size());

			  const auto& receipt = static_cast<const model::SignerBalanceReceipt&>(receiptPair.second.receiptAt(0));
			  AssertReceipt(Amount(1000), Amount(100), receipt);
			});
		}

		// endregion
	}}
