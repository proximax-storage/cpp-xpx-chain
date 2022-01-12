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

#include "catapult/state/AccountState.h"
#include "catapult/state/AccountBalances.h"
#include "tests/test/core/TransactionTestUtils.h"

namespace catapult { namespace state {

#define TEST_CLASS AccountBalancesTests

	namespace {
		constexpr MosaicId Test_Mosaic_Id1 = MosaicId(12345);
		constexpr MosaicId Test_Mosaic_Id2 = MosaicId(54321);
		constexpr MosaicId Test_Mosaic_Id3 = MosaicId(99999);
		AccountState Test_Account(Address{ { 1 } }, Height(1));
	}

	// region construction + assignment

	TEST(TEST_CLASS, CanCreateEmptyAccountBalances) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);

		// Act:
		auto amount1 = balances.get(Test_Mosaic_Id1);
		auto amount2 = balances.get(Test_Mosaic_Id2);

		// Assert:
		EXPECT_EQ(0u, balances.size());
		EXPECT_EQ(0u, balances.snapshots().size());
		EXPECT_EQ(Amount(0), amount1);
		EXPECT_EQ(Amount(0), amount2);

		EXPECT_EQ(MosaicId(), balances.optimizedMosaicId());
	}

	namespace {
		AccountBalances CreateBalancesForConstructionTests(MosaicId optimizedMosaicId) {
			AccountBalances balances(&Test_Account);
			balances.track(Test_Mosaic_Id3);
			balances.credit(Test_Mosaic_Id2, Amount(777));
			balances.credit(Test_Mosaic_Id3, Amount(1000));
			balances.optimize(optimizedMosaicId);
			return balances;
		}

		void AssertCopied(const AccountBalances& balances, const AccountBalances& balancesCopy, MosaicId optimizedMosaicId) {
			// Assert: the copy is detached from the original
			EXPECT_EQ(Amount(777), balances.get(Test_Mosaic_Id2));
			EXPECT_EQ(Amount(1000), balances.get(Test_Mosaic_Id3));

			EXPECT_EQ(Amount(777), balancesCopy.get(Test_Mosaic_Id2));
			EXPECT_EQ(Amount(1500), balancesCopy.get(Test_Mosaic_Id3));

			// - optimization is preserved
			EXPECT_EQ(optimizedMosaicId, balancesCopy.optimizedMosaicId());
			EXPECT_EQ(balances.trackedMosaicId(), balancesCopy.trackedMosaicId());
		}

		void AssertMoved(const AccountBalances& balances, const AccountBalances& balancesMoved, MosaicId optimizedMosaicId) {
			// Assert: the original values are moved into the copy (move does not clear first mosaic)
			if (Test_Mosaic_Id3 == optimizedMosaicId) {
				EXPECT_EQ(Amount(0), balances.get(Test_Mosaic_Id2));
				EXPECT_EQ(Amount(1000), balances.get(Test_Mosaic_Id3));
			} else {
				EXPECT_EQ(Amount(777), balances.get(Test_Mosaic_Id2));
				EXPECT_EQ(Amount(0), balances.get(Test_Mosaic_Id3));
			}

			EXPECT_EQ(Amount(777), balancesMoved.get(Test_Mosaic_Id2));
			EXPECT_EQ(Amount(1000), balancesMoved.get(Test_Mosaic_Id3));

			// - optimization is preserved
			EXPECT_EQ(optimizedMosaicId, balancesMoved.optimizedMosaicId());
			EXPECT_EQ(balances.trackedMosaicId(), balancesMoved.trackedMosaicId());
		}

		void AssertCanCopyConstructAccountBalances(MosaicId optimizedMosaicId) {
			// Arrange:
			auto balances = CreateBalancesForConstructionTests(optimizedMosaicId);

			// Act:
			AccountBalances balancesCopy(balances);
			balancesCopy.credit(Test_Mosaic_Id3, Amount(500), Height(2));
			balancesCopy.commitSnapshots();

			// Assert:
			AssertCopied(balances, balancesCopy, optimizedMosaicId);
		}

		void AssertCanMoveConstructAccountBalances(MosaicId optimizedMosaicId) {
			// Arrange:
			auto balances = CreateBalancesForConstructionTests(optimizedMosaicId);

			// Act:
			AccountBalances balancesMoved(std::move(balances));

			// Assert:
			AssertMoved(balances, balancesMoved, optimizedMosaicId);
		}

		void AssertCanAssignAccountBalances(MosaicId optimizedMosaicId) {
			// Arrange:
			auto balances = CreateBalancesForConstructionTests(optimizedMosaicId);

			// Act:
			AccountBalances balancesCopy(&Test_Account);
			const auto& assignResult = balancesCopy = balances;
			balancesCopy.credit(Test_Mosaic_Id3, Amount(500), Height(2));
			balancesCopy.commitSnapshots();

			// Assert:
			EXPECT_EQ(&balancesCopy, &assignResult);
			AssertCopied(balances, balancesCopy, optimizedMosaicId);
		}

		void AssertCanMoveAssignAccountBalances(MosaicId optimizedMosaicId) {
			// Arrange:
			auto balances = CreateBalancesForConstructionTests(optimizedMosaicId);

			// Act:
			AccountBalances balancesMoved(&Test_Account);
			const auto& assignResult = balancesMoved = std::move(balances);

			// Assert:
			EXPECT_EQ(&balancesMoved, &assignResult);
			AssertMoved(balances, balancesMoved, optimizedMosaicId);
		}
	}

	TEST(TEST_CLASS, CanCopyConstructAccountBalances) {
		// Assert:
		AssertCanCopyConstructAccountBalances(Test_Mosaic_Id2);
	}

	TEST(TEST_CLASS, CanCopyConstructAccountBalances_NoOptimization) {
		// Assert:
		AssertCanCopyConstructAccountBalances(MosaicId());
	}

	TEST(TEST_CLASS, CanMoveConstructAccountBalances) {
		// Assert:
		AssertCanMoveConstructAccountBalances(Test_Mosaic_Id3);
	}

	TEST(TEST_CLASS, CanMoveConstructAccountBalances_NoOptimization) {
		// Assert:
		AssertCanMoveConstructAccountBalances(MosaicId());
	}

	TEST(TEST_CLASS, CanAssignAccountBalances) {
		// Assert:
		AssertCanAssignAccountBalances(Test_Mosaic_Id2);
	}

	TEST(TEST_CLASS, CanAssignAccountBalances_NoOptimization) {
		// Assert:
		AssertCanAssignAccountBalances(MosaicId());
	}

	TEST(TEST_CLASS, CanMoveAssignAccountBalances) {
		// Assert:
		AssertCanMoveAssignAccountBalances(Test_Mosaic_Id3);
	}

	TEST(TEST_CLASS, CanMoveAssignAccountBalances_NoOptimization) {
		// Assert:
		AssertCanMoveAssignAccountBalances(MosaicId());
	}

	// endregion

	// region credit

	TEST(TEST_CLASS, CreditDoesNotAddZeroBalance) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);

		// Act:
		balances.credit(Test_Mosaic_Id3, Amount(0), Height(1));

		// Assert:
		EXPECT_EQ(0u, balances.size());
		EXPECT_EQ(Amount(0), balances.get(Test_Mosaic_Id3));
		EXPECT_EQ(Amount(0), balances.getEffectiveBalance(Height(0), 0));
		EXPECT_EQ(0, balances.snapshots().size());
	}

	TEST(TEST_CLASS, CreditIncreasesAmountStored) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);

		// Act:
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(1u, balances.size());
		EXPECT_EQ(Amount(12345), balances.get(Test_Mosaic_Id3));
		EXPECT_EQ(1, balances.snapshots().size());
		EXPECT_EQ(Amount(12345), balances.snapshots().front().Amount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);
	}

	TEST(TEST_CLASS, InterleavingCreditsYieldCorrectState) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);

		// Act:
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));
		balances.credit(Test_Mosaic_Id1, Amount(3456), Height(1));
		balances.credit(Test_Mosaic_Id3, Amount(54321), Height(1));
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(2u, balances.size());
		EXPECT_EQ(Amount(12345 + 54321), balances.get(Test_Mosaic_Id3));
		EXPECT_EQ(Amount(3456), balances.get(Test_Mosaic_Id1));
		EXPECT_EQ(1, balances.snapshots().size());
		EXPECT_EQ(Amount(12345 + 54321), balances.snapshots().front().Amount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);
	}

	// endregion

	// region debit

	TEST(TEST_CLASS, CanDebitZeroFromZeroBalance) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);

		// Act:
		balances.debit(Test_Mosaic_Id3, Amount(0), Height(1));

		// Assert:
		EXPECT_EQ(0u, balances.size());
		EXPECT_EQ(Amount(0), balances.get(Test_Mosaic_Id3));
		EXPECT_EQ(0, balances.snapshots().size());
	}

	TEST(TEST_CLASS, DebitDecreasesAmountStored) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));
		balances.commitSnapshots();

		// Act:
		balances.debit(Test_Mosaic_Id3, Amount(222), Height(1));
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(1u, balances.size());
		EXPECT_EQ(Amount(12345 - 222), balances.get(Test_Mosaic_Id3));
		EXPECT_EQ(1, balances.snapshots().size());
		EXPECT_EQ(Amount(12345 - 222), balances.snapshots().front().Amount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);
	}

	TEST(TEST_CLASS, FullDebitRemovesMosaicFromCache) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		Amount amount = Amount(12345);
		balances.credit(Test_Mosaic_Id3, amount, Height(1));
		balances.commitSnapshots();

		// Act:
		balances.debit(Test_Mosaic_Id3, amount, Height(2));
		balances.commitSnapshots();
		auto xpxHeld = balances.get(Test_Mosaic_Id3);

		// Assert:
		EXPECT_EQ(0u, balances.size());
		EXPECT_EQ(Amount(0), xpxHeld);
		EXPECT_EQ(2, balances.snapshots().size());
		EXPECT_EQ(amount, balances.snapshots().front().Amount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);
		EXPECT_EQ(Amount(0), balances.snapshots().back().Amount);
		EXPECT_EQ(Height(2), balances.snapshots().back().BalanceHeight);
	}

	TEST(TEST_CLASS, InterleavingDebitsYieldCorrectState) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));
		balances.credit(Test_Mosaic_Id1, Amount(3456), Height(1));

		// Act:
		balances.debit(Test_Mosaic_Id3, Amount(222), Height(1));
		balances.debit(Test_Mosaic_Id1, Amount(1111), Height(1));
		balances.debit(Test_Mosaic_Id3, Amount(111), Height(1));
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(2u, balances.size());
		EXPECT_EQ(Amount(12345 - 222 - 111), balances.get(Test_Mosaic_Id3));
		EXPECT_EQ(Amount(3456 - 1111), balances.get(Test_Mosaic_Id1));
		EXPECT_EQ(1, balances.snapshots().size());
		EXPECT_EQ(Amount(12345 - 222 - 111), balances.snapshots().front().Amount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);
	}

	TEST(TEST_CLASS, DebitDoesNotAllowUnderflowOfNonZeroBalance) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));

		// Act + Assert:
		EXPECT_THROW(balances.debit(Test_Mosaic_Id3, Amount(12346), Height(2)), catapult_runtime_error);
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(1u, balances.size());
		EXPECT_EQ(Amount(12345), balances.get(Test_Mosaic_Id3));
		EXPECT_EQ(1, balances.snapshots().size());
		EXPECT_EQ(Amount(12345), balances.snapshots().front().Amount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);
	}

	TEST(TEST_CLASS, DebitDoesNotAllowUnderflowOfZeroBalance) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);

		// Act + Assert:
		EXPECT_THROW(balances.debit(Test_Mosaic_Id3, Amount(222), Height(1)), catapult_runtime_error);

		// Assert:
		EXPECT_EQ(0u, balances.size());
		EXPECT_EQ(Amount(0), balances.get(Test_Mosaic_Id3));
		EXPECT_EQ(0, balances.snapshots().size());
	}

	// endregion

	// region lock

	TEST(TEST_CLASS, CanLockZeroFromZeroBalance) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);

		// Act:
		balances.lock(Test_Mosaic_Id3, Amount(0), Height(1));

		// Assert:
		EXPECT_EQ(0u, balances.size());
		EXPECT_EQ(Amount(0), balances.get(Test_Mosaic_Id3));
		EXPECT_EQ(0, balances.snapshots().size());
	}

	TEST(TEST_CLASS, LockDecreasesAmountStored) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));
		balances.commitSnapshots();

		// Act:
		balances.lock(Test_Mosaic_Id3, Amount(222), Height(1));
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(1u, balances.size());
		EXPECT_EQ(Amount(12345 - 222), balances.get(Test_Mosaic_Id3));
		EXPECT_EQ(Amount(222), balances.getLocked(Test_Mosaic_Id3));
		EXPECT_EQ(1, balances.snapshots().size());
		EXPECT_EQ(Amount(222), balances.snapshots().front().LockedAmount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);
	}

	TEST(TEST_CLASS, FullLockRemovesMosaicFromCache) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		Amount amount = Amount(12345);
		balances.credit(Test_Mosaic_Id3, amount, Height(1));
		balances.commitSnapshots();

		// Act:
		balances.lock(Test_Mosaic_Id3, amount, Height(2));
		balances.commitSnapshots();
		auto xpxHeld = balances.get(Test_Mosaic_Id3);

		// Assert:
		EXPECT_EQ(0u, balances.balances().size());
		EXPECT_EQ(1u, balances.lockedBalances().size());
		EXPECT_EQ(Amount(0), xpxHeld);
		EXPECT_EQ(2, balances.snapshots().size());
		EXPECT_EQ(amount, balances.snapshots().front().Amount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);
		EXPECT_EQ(Amount(0), balances.snapshots().back().Amount);
		EXPECT_EQ(Height(2), balances.snapshots().back().BalanceHeight);
	}

	TEST(TEST_CLASS, InterleavingLocksYieldCorrectState) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));
		balances.credit(Test_Mosaic_Id1, Amount(3456), Height(1));

		// Act:
		balances.lock(Test_Mosaic_Id3, Amount(222), Height(1));
		balances.lock(Test_Mosaic_Id1, Amount(1111), Height(1));
		balances.lock(Test_Mosaic_Id3, Amount(111), Height(1));
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(2u, balances.size());
		EXPECT_EQ(Amount(12345 - 222 - 111), balances.get(Test_Mosaic_Id3));
		EXPECT_EQ(Amount(3456 - 1111), balances.get(Test_Mosaic_Id1));
		EXPECT_EQ(Amount(222 + 111), balances.getLocked(Test_Mosaic_Id3));
		EXPECT_EQ(Amount(1111), balances.getLocked(Test_Mosaic_Id1));
		EXPECT_EQ(1, balances.snapshots().size());
		EXPECT_EQ(Amount(12345 - 222 - 111), balances.snapshots().front().Amount);
		EXPECT_EQ(Amount(222 + 111), balances.snapshots().front().LockedAmount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);
	}

	TEST(TEST_CLASS, LockDoesNotAllowUnderflowOfNonZeroBalance) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));

		// Act + Assert:
		EXPECT_THROW(balances.lock(Test_Mosaic_Id3, Amount(12346), Height(2)), catapult_runtime_error);
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(1u, balances.size());
		EXPECT_EQ(Amount(12345), balances.get(Test_Mosaic_Id3));
		EXPECT_EQ(1, balances.snapshots().size());
		EXPECT_EQ(Amount(12345), balances.snapshots().front().Amount);
		EXPECT_EQ(Amount(0), balances.snapshots().front().LockedAmount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);
	}

	TEST(TEST_CLASS, LockDoesNotAllowUnderflowOfZeroBalance) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);

		// Act + Assert:
		EXPECT_THROW(balances.debit(Test_Mosaic_Id3, Amount(222), Height(1)), catapult_runtime_error);

		// Assert:
		EXPECT_EQ(0u, balances.size());
		EXPECT_EQ(Amount(0), balances.get(Test_Mosaic_Id3));
		EXPECT_EQ(Amount(0), balances.getLocked(Test_Mosaic_Id3));
		EXPECT_EQ(0, balances.snapshots().size());
	}

	// endregion

	// region unlock

	TEST(TEST_CLASS, CanUnlockZeroFromZeroBalance) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);

		// Act:
		balances.unlock(Test_Mosaic_Id3, Amount(0), Height(1));

		// Assert:
		EXPECT_EQ(0u, balances.size());
		EXPECT_EQ(Amount(0), balances.get(Test_Mosaic_Id3));
		EXPECT_EQ(0, balances.snapshots().size());
	}

	TEST(TEST_CLASS, UnlockDecreasesAmountStored) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));
		balances.commitSnapshots();
		balances.lock(Test_Mosaic_Id3, Amount(222), Height(1));
		balances.commitSnapshots();
		// Act:
		balances.unlock(Test_Mosaic_Id3, Amount(111), Height(1));
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(1u, balances.size());
		EXPECT_EQ(Amount(12345 - 222 + 111), balances.get(Test_Mosaic_Id3));
		EXPECT_EQ(Amount(222 - 111), balances.getLocked(Test_Mosaic_Id3));
		EXPECT_EQ(1, balances.snapshots().size());
		EXPECT_EQ(Amount(222 - 111), balances.snapshots().front().LockedAmount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);
	}

	TEST(TEST_CLASS, UnlockRemovesMosaicFromCache) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		Amount amount = Amount(12345);
		balances.credit(Test_Mosaic_Id3, amount, Height(1));
		balances.lock(Test_Mosaic_Id3, amount, Height(1));
		balances.commitSnapshots();

		// Act:
		balances.unlock(Test_Mosaic_Id3, amount, Height(2));
		balances.commitSnapshots();
		auto xpxHeld = balances.get(Test_Mosaic_Id3);

		// Assert:
		EXPECT_EQ(1u, balances.balances().size());
		EXPECT_EQ(0u, balances.lockedBalances().size());
		EXPECT_EQ(Amount(12345), xpxHeld);
		EXPECT_EQ(2, balances.snapshots().size());
		EXPECT_EQ(amount, balances.snapshots().front().LockedAmount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);
		EXPECT_EQ(Amount(amount), balances.snapshots().back().Amount);
		EXPECT_EQ(Amount(0), balances.snapshots().back().LockedAmount);
		EXPECT_EQ(Height(2), balances.snapshots().back().BalanceHeight);
	}

	TEST(TEST_CLASS, InterleavingUnlocksYieldCorrectState) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));
		balances.credit(Test_Mosaic_Id1, Amount(3456), Height(1));
		balances.lock(Test_Mosaic_Id3, Amount(222), Height(1));
		balances.lock(Test_Mosaic_Id1, Amount(1111), Height(1));
		balances.lock(Test_Mosaic_Id3, Amount(111), Height(1));
		balances.commitSnapshots();
		// Act:
		balances.unlock(Test_Mosaic_Id3, Amount(111), Height(1));
		balances.unlock(Test_Mosaic_Id1, Amount(111), Height(1));
		balances.unlock(Test_Mosaic_Id3, Amount(11), Height(1));
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(2u, balances.size());
		EXPECT_EQ(Amount(12345 - 222 - 111 + 111 + 11), balances.get(Test_Mosaic_Id3));
		EXPECT_EQ(Amount(3456 - 1111 + 111), balances.get(Test_Mosaic_Id1));
		EXPECT_EQ(Amount(222 + 111 - 111 - 11), balances.getLocked(Test_Mosaic_Id3));
		EXPECT_EQ(Amount(1111 - 111), balances.getLocked(Test_Mosaic_Id1));
		EXPECT_EQ(1, balances.snapshots().size());
		EXPECT_EQ(Amount(12345 - 222 - 111 + 111 + 11), balances.snapshots().front().Amount);
		EXPECT_EQ(Amount(222 + 111 - 111 - 11), balances.snapshots().front().LockedAmount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);
	}

	TEST(TEST_CLASS, UnlockDoesNotAllowUnderflowOfNonZeroBalance) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));
		balances.lock(Test_Mosaic_Id3, Amount(12345), Height(1));
		balances.commitSnapshots();

		// Act + Assert:
		EXPECT_THROW(balances.unlock(Test_Mosaic_Id3, Amount(12346), Height(2)), catapult_runtime_error);
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(1u, balances.size());
		EXPECT_EQ(Amount(12345), balances.getLocked(Test_Mosaic_Id3));
		EXPECT_EQ(1, balances.snapshots().size());
		EXPECT_EQ(Amount(0), balances.snapshots().front().Amount);
		EXPECT_EQ(Amount(12345), balances.snapshots().front().LockedAmount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);
	}

	TEST(TEST_CLASS, UnlockDoesNotAllowUnderflowOfZeroBalance) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);

		// Act + Assert:
		EXPECT_THROW(balances.unlock(Test_Mosaic_Id3, Amount(222), Height(1)), catapult_runtime_error);

		// Assert:
		EXPECT_EQ(0u, balances.size());
		EXPECT_EQ(Amount(0), balances.get(Test_Mosaic_Id3));
		EXPECT_EQ(Amount(0), balances.getLocked(Test_Mosaic_Id3));
		EXPECT_EQ(0, balances.snapshots().size());
	}

	// endregion

	// region credit + debit

	TEST(TEST_CLASS, InterleavingDebitsAndCreditsYieldCorrectState) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));
		balances.credit(Test_Mosaic_Id1, Amount(3456), Height(1));

		// Act:
		balances.debit(Test_Mosaic_Id1, Amount(1111), Height(1));
		balances.credit(Test_Mosaic_Id3, Amount(1111), Height(1));
		balances.credit(Test_Mosaic_Id2, Amount(0), Height(1)); // no op
		balances.debit(Test_Mosaic_Id3, Amount(2345), Height(1));
		balances.debit(Test_Mosaic_Id2, Amount(0), Height(1)); // no op
		balances.credit(Test_Mosaic_Id1, Amount(5432), Height(1));
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(2u, balances.size());
		EXPECT_EQ(Amount(12345 + 1111 - 2345), balances.get(Test_Mosaic_Id3));
		EXPECT_EQ(Amount(3456 - 1111 + 5432), balances.get(Test_Mosaic_Id1));
		EXPECT_EQ(1, balances.snapshots().size());
		EXPECT_EQ(Amount(12345 + 1111 - 2345), balances.snapshots().front().Amount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);
	}

	TEST(TEST_CLASS, ChainedInterleavingDebitsAndCreditsYieldCorrectState) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances
			.credit(Test_Mosaic_Id3, Amount(12345), Height(1))
			.credit(Test_Mosaic_Id1, Amount(3456), Height(1));

		// Act:
		balances
			.debit(Test_Mosaic_Id1, Amount(1111), Height(1))
			.credit(Test_Mosaic_Id3, Amount(1111), Height(1))
			.credit(Test_Mosaic_Id2, Amount(0), Height(1)) // no op
			.debit(Test_Mosaic_Id3, Amount(2345), Height(1))
			.debit(Test_Mosaic_Id2, Amount(0), Height(1)) // no op
			.credit(Test_Mosaic_Id1, Amount(5432), Height(1));
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(2u, balances.size());
		EXPECT_EQ(Amount(12345 + 1111 - 2345), balances.get(Test_Mosaic_Id3));
		EXPECT_EQ(Amount(3456 - 1111 + 5432), balances.get(Test_Mosaic_Id1));
		EXPECT_EQ(1, balances.snapshots().size());
		EXPECT_EQ(Amount(12345 + 1111 - 2345), balances.snapshots().front().Amount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);
	}

	// endregion

	// region optimize

	TEST(TEST_CLASS, CanOptimizeMosaicStorage) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances
			.credit(Test_Mosaic_Id1, Amount(12345))
			.credit(Test_Mosaic_Id3, Amount(2244))
			.credit(Test_Mosaic_Id2, Amount(3456));

		// Sanity:
		EXPECT_EQ(Test_Mosaic_Id1, balances.balances().begin()->first);

		// Act:
		balances.optimize(Test_Mosaic_Id2);

		// Assert:
		EXPECT_EQ(Test_Mosaic_Id2, balances.optimizedMosaicId());
		EXPECT_EQ(Test_Mosaic_Id2, balances.balances().begin()->first);
	}

	// endregion

	// region iteration

	TEST(TEST_CLASS, CanIterateOverAllBalances) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances
			.credit(Test_Mosaic_Id3, Amount(12345), Height(1))
			.credit(Test_Mosaic_Id2, Amount(0), Height(1))
			.credit(Test_Mosaic_Id1, Amount(3456), Height(1));
		balances
				.lock(Test_Mosaic_Id3, Amount(5), Height(1))
				.lock(Test_Mosaic_Id2, Amount(0), Height(1))
				.lock(Test_Mosaic_Id1, Amount(6), Height(1));

		// Act:
		auto numBalances = 0u;
		std::map<MosaicId, Amount> iteratedBalances;
		for (const auto& pair : balances.balances()) {
			iteratedBalances.emplace(pair);
			++numBalances;
		}
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(2u, numBalances);
		EXPECT_EQ(2u, iteratedBalances.size());
		EXPECT_EQ(Amount(12340), iteratedBalances[Test_Mosaic_Id3]);
		EXPECT_EQ(Amount(3450), iteratedBalances[Test_Mosaic_Id1]);
		EXPECT_EQ(1, balances.snapshots().size());
		EXPECT_EQ(Amount(12340), balances.snapshots().front().Amount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);
	}

	// endregion

	// region snapshots

	TEST(TEST_CLASS, DoesNotAddSnapshotsForZeroHeight) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);

		// Act:
		balances.credit(Test_Mosaic_Id3, Amount(12345));

		// Assert:
		EXPECT_EQ(0, balances.snapshots().size());

		// Act:
		balances.debit(Test_Mosaic_Id3, Amount(12345));

		// Assert:
		EXPECT_EQ(0, balances.snapshots().size());

		// Assert:
		EXPECT_THROW(balances.credit(Test_Mosaic_Id3, Amount(12345), Height(0)), catapult_runtime_error);
		EXPECT_THROW(balances.debit(Test_Mosaic_Id3, Amount(12345), Height(0)), catapult_runtime_error);
	}

	TEST(TEST_CLASS, AddOneSnapshotForNemesisBlock) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(1, balances.snapshots().size());
		EXPECT_EQ(Amount(12345), balances.snapshots().front().Amount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);
	}

	TEST(TEST_CLASS, AddTwoSnapshotIfSnapshotsAreEmptyAndPreviousBlockIsNotNemesis_PreviousBalanceOfAccountIsZero) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(2));
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(2, balances.snapshots().size());
		EXPECT_EQ(Amount(0), balances.snapshots().front().Amount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);
		EXPECT_EQ(Amount(12345), balances.snapshots().back().Amount);
		EXPECT_EQ(Height(2), balances.snapshots().back().BalanceHeight);
	}

	TEST(TEST_CLASS, AddTwoSnapshotIfSnapshotsAreEmptyAndPreviousBlockIsNotNemesis_PreviousBalanceOfAccountIsNotZero) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345));
		// Assert:
		EXPECT_EQ(0, balances.snapshots().size());

		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(2));
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(2, balances.snapshots().size());
		EXPECT_EQ(Amount(12345), balances.snapshots().front().Amount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);
		EXPECT_EQ(Amount(12345 + 12345), balances.snapshots().back().Amount);
		EXPECT_EQ(Height(2), balances.snapshots().back().BalanceHeight);
	}

	TEST(TEST_CLASS, UpdateOfBalanceOnCurrentHeightMustUpdateSnapshot) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(1, balances.snapshots().size());
		EXPECT_EQ(Amount(12345), balances.snapshots().front().Amount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);

		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(1, balances.snapshots().size());
		EXPECT_EQ(Amount(12345 + 12345), balances.snapshots().front().Amount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);

		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(1));
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(1, balances.snapshots().size());
		EXPECT_EQ(Amount(12345), balances.snapshots().front().Amount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);
	}

	TEST(TEST_CLASS, UpdateOfBalanceOnIncreasingHeightMustAddNewSnapshot) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(2));
		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(3));
		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(4));
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(5));
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(5, balances.snapshots().size());
		auto it = balances.snapshots().begin();
		EXPECT_EQ(Amount(12345), it->Amount);
		EXPECT_EQ(Height(1), it->BalanceHeight);
		++it;
		EXPECT_EQ(Amount(12345 + 12345), it->Amount);
		EXPECT_EQ(Height(2), it->BalanceHeight);
		++it;
		EXPECT_EQ(Amount(12345), it->Amount);
		EXPECT_EQ(Height(3), it->BalanceHeight);
		++it;
		EXPECT_EQ(Amount(0), it->Amount);
		EXPECT_EQ(Height(4), it->BalanceHeight);
		++it;
		EXPECT_EQ(Amount(12345), it->Amount);
		EXPECT_EQ(Height(5), it->BalanceHeight);
	}

	TEST(TEST_CLASS, UpdateOfBalanceOnPreviousHeightAfterRollback) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(2));
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(2, balances.snapshots().size());
		EXPECT_EQ(Amount(12345), balances.snapshots().front().Amount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);
		EXPECT_EQ(Amount(12345 + 12345), balances.snapshots().back().Amount);
		EXPECT_EQ(Height(2), balances.snapshots().back().BalanceHeight);

		// Rollback last credit
		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(2));

		// Update balance on previous height
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(1, balances.snapshots().size());
		EXPECT_EQ(Amount(12345 + 12345), balances.snapshots().front().Amount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);
	}

	TEST(TEST_CLASS, SeveralCommitsAndRollbacks_OnTheSameHeights) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(2));
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(2));
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(2));
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(2));
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(2, balances.snapshots().size());

		// Rollbacks
		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(2));
		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(2));
		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(2));
		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(2));
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(1, balances.snapshots().size());
		EXPECT_EQ(Amount(12345), balances.snapshots().front().Amount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);
	}

	TEST(TEST_CLASS, SeveralCommitsAndRollbacks_OnTheDifferentHeights) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(2));
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(3));
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(4));
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(5));
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(5, balances.snapshots().size());

		// Rollbacks
		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(5));
		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(4));
		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(3));
		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(2));
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(1, balances.snapshots().size());
		EXPECT_EQ(Amount(12345), balances.snapshots().front().Amount);
		EXPECT_EQ(Height(1), balances.snapshots().front().BalanceHeight);
	}

	// endregion

	// region effective balance region

	TEST(TEST_CLASS, GetEffectiveBalanceOfOldBalance) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345));

		// Assert:
		EXPECT_EQ(Amount(12345), balances.getEffectiveBalance(Height(0), 0));
	}

	TEST(TEST_CLASS, GetEffectiveBalanceOfBalanceFromNemesisBlock) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));

		// Assert:
		EXPECT_EQ(Amount(12345), balances.getEffectiveBalance(Height(0), 0));
	}

	TEST(TEST_CLASS, GetEffectiveBalanceOfBalanceFromNotNemesisBlock) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(2));

		// Assert:
		// We expect zero because previous balance of account on nemesis block was zero
		EXPECT_EQ(Amount(0), balances.getEffectiveBalance(Height(0), 0));
	}

	TEST(TEST_CLASS, GetEffectiveBalanceDefaultCase) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345 + 12345 + 12345), Height(1));
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(2));

		// Assert:
		EXPECT_EQ(Amount(12345 + 12345 + 12345), balances.getEffectiveBalance(Height(0), 0));

		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(3));
		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(4));

		// Assert:
		EXPECT_EQ(Amount(12345 + 12345), balances.getEffectiveBalance(Height(0), 0));

		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(5));

		// Assert:
		EXPECT_EQ(Amount(12345), balances.getEffectiveBalance(Height(0), 0));

		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(6));

		// Assert:
		EXPECT_EQ(Amount(0), balances.getEffectiveBalance(Height(0), 0));
	}

	TEST(TEST_CLASS, GetEffectiveBalanceWithRollback) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345));
		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(1));

		// Assert:
		EXPECT_EQ(Amount(0), balances.getEffectiveBalance(Height(0), 0));

		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));

		// Assert:
		EXPECT_EQ(Amount(12345), balances.getEffectiveBalance(Height(0), 0));
	}

	TEST(TEST_CLASS, GetEffectiveBalanceWithRollback_WithCommitAfterDebit) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345 * 3));
		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(1));
		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(2));
		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(3));
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(Amount(0), balances.getEffectiveBalance(Height(0), 0));
		EXPECT_EQ(3, balances.snapshots().size());

		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));

		// Assert:
		EXPECT_EQ(Amount(0), balances.getEffectiveBalance(Height(0), 0));

		balances.commitSnapshots();
		EXPECT_EQ(1, balances.snapshots().size());
	}

	TEST(TEST_CLASS, GetEffectiveBalanceWithEffectiveRange) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(2));
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(3));
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(4));

		// Assert:
		EXPECT_EQ(Amount(12345 * 4), balances.getEffectiveBalance(Height(4), 0));
		EXPECT_EQ(Amount(12345 * 3), balances.getEffectiveBalance(Height(4), 1));
		EXPECT_EQ(Amount(12345 * 2), balances.getEffectiveBalance(Height(4), 2));
		EXPECT_EQ(Amount(12345 * 1), balances.getEffectiveBalance(Height(4), 3));
		EXPECT_EQ(Amount(12345 * 1), balances.getEffectiveBalance(Height(4), 4));
		EXPECT_EQ(Amount(12345 * 1), balances.getEffectiveBalance(Height(4), 5));
	}

	TEST(TEST_CLASS, GetEffectiveBalanceWithEffectiveRange_SeveralDebitAndCredit) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));
		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(2));
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(3));
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(4));
		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(5));
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(6));

		// Assert:
		EXPECT_EQ(Amount(12345 * 2), balances.getEffectiveBalance(Height(6), 0));
		EXPECT_EQ(Amount(12345 * 1), balances.getEffectiveBalance(Height(6), 1));
		EXPECT_EQ(Amount(12345 * 1), balances.getEffectiveBalance(Height(6), 2));
		EXPECT_EQ(Amount(12345 * 1), balances.getEffectiveBalance(Height(6), 3));
		EXPECT_EQ(Amount(12345 * 0), balances.getEffectiveBalance(Height(6), 4));
		EXPECT_EQ(Amount(12345 * 0), balances.getEffectiveBalance(Height(6), 5));
	}

	TEST(TEST_CLASS, GetEffectiveBalanceWithEffectiveRange_OnlyDebit) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345 * 5));
		// Assert:
		EXPECT_EQ(Amount(12345 * 5), balances.getEffectiveBalance(Height(1), 6));

		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(1));
		// Assert:
		EXPECT_EQ(Amount(12345 * 4), balances.getEffectiveBalance(Height(1), 6));

		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(2));
		// Assert:
		EXPECT_EQ(Amount(12345 * 3), balances.getEffectiveBalance(Height(2), 6));

		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(3));
		// Assert:
		EXPECT_EQ(Amount(12345 * 2), balances.getEffectiveBalance(Height(3), 6));

		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(4));
		// Assert:
		EXPECT_EQ(Amount(12345 * 1), balances.getEffectiveBalance(Height(4), 6));

		balances.debit(Test_Mosaic_Id3, Amount(12345), Height(5));
		// Assert:
		EXPECT_EQ(Amount(12345 * 0), balances.getEffectiveBalance(Height(5), 6));
	}

	// endregion

	// region account height

	TEST(TEST_CLASS, CreditOrDebitOnHeightThatLowerThanHeightOfAccount) {
		// Arrange:
		AccountState accountState(Address{ { 1 } }, Height(100));
		AccountBalances balances(&accountState);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345));

		// Assert:
		EXPECT_EQ(0, balances.snapshots().size());

		// Act + Assert:
		EXPECT_THROW(balances.credit(Test_Mosaic_Id3, Amount(12346), Height(50)), catapult_runtime_error);
		EXPECT_THROW(balances.debit(Test_Mosaic_Id3, Amount(12346), Height(50)), catapult_runtime_error);
	}

	TEST(TEST_CLASS, CreditOrDebitWidthoutAccount) {
		// Arrange:
		AccountBalances balances(nullptr);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345));

		// Assert:
		EXPECT_EQ(0, balances.snapshots().size());

		// Act + Assert:
		EXPECT_THROW(balances.credit(Test_Mosaic_Id3, Amount(12346), Height(1)), catapult_runtime_error);
		EXPECT_THROW(balances.debit(Test_Mosaic_Id3, Amount(12346), Height(1)), catapult_runtime_error);
	}

	TEST(TEST_CLASS, TrackAnotherIdWhenRemoteAndLocalSnapshotsAreEmpty) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);

		// Act:
		balances.track(Test_Mosaic_Id2);
	}

	TEST(TEST_CLASS, TrackAnotherIdWhenLocalSnapshotsIsNotEmpty) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(1));
		balances.commitSnapshots();

		// Assert:
		EXPECT_EQ(1, balances.snapshots().size());

		// Act + Assert:
		EXPECT_THROW(balances.track(Test_Mosaic_Id2), catapult_runtime_error);
	}

	TEST(TEST_CLASS, TrackAnotherIdWhenRemoteSnapshotsIsNotEmpty) {
		// Arrange:
		AccountBalances balances(&Test_Account);
		balances.track(Test_Mosaic_Id3);
		balances.credit(Test_Mosaic_Id3, Amount(12345), Height(2));

		// Act + Assert:
		EXPECT_THROW(balances.track(Test_Mosaic_Id2), catapult_runtime_error);
	}

	// endregion
}}
