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
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS SourceChangeObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(SourceChange,)

	// region rollback

	namespace {
		using SourceChangeType = model::SourceChangeNotification<1>::SourceChangeType;

		void AssertRollbackDoesNotChangeObserverSource(SourceChangeType primaryChangeType, SourceChangeType secondaryChangeType) {
			// Arrange:
			auto pObserver = CreateSourceChangeObserver();
			auto notification = model::SourceChangeNotification<1>(primaryChangeType, 10, secondaryChangeType, 5);

			test::ObserverTestContext context(NotifyMode::Rollback, Height{444});
			context.statementBuilder().setSource({ 15, 22 });

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert:
			const auto& source = context.statementBuilder().source();
			EXPECT_EQ(15u, source.PrimaryId);
			EXPECT_EQ(22u, source.SecondaryId);
		}
	}

	TEST(TEST_CLASS, RollbackRelativeRelativeDoesNotChangeObserverSource) {
		// Assert:
		AssertRollbackDoesNotChangeObserverSource(SourceChangeType::Relative, SourceChangeType::Relative);
	}

	TEST(TEST_CLASS, RollbackAbsoluteRelativeDoesNotChangeObserverSource) {
		// Assert:
		AssertRollbackDoesNotChangeObserverSource(SourceChangeType::Absolute, SourceChangeType::Relative);
	}

	TEST(TEST_CLASS, RollbackRelativeAbsoluteDoesNotChangeObserverSource) {
		// Assert:
		AssertRollbackDoesNotChangeObserverSource(SourceChangeType::Relative, SourceChangeType::Absolute);
	}

	TEST(TEST_CLASS, RollbackAbsoluteAbsoluteDoesNotChangeObserverSource) {
		// Assert:
		AssertRollbackDoesNotChangeObserverSource(SourceChangeType::Absolute, SourceChangeType::Absolute);
	}

	// endregion

	// region commit

	namespace {
		void AssertCommitChangesObserverSource(
				SourceChangeType primaryChangeType,
				SourceChangeType secondaryChangeType,
				const model::ReceiptSource& expectedSource) {
			// Arrange:
			auto pObserver = CreateSourceChangeObserver();
			auto notification = model::SourceChangeNotification<1>(primaryChangeType, 10, secondaryChangeType, 5);

			test::ObserverTestContext context(NotifyMode::Commit, Height{444});
			context.statementBuilder().setSource({ 15, 22 });

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert:
			const auto& source = context.statementBuilder().source();
			EXPECT_EQ(expectedSource.PrimaryId, source.PrimaryId);
			EXPECT_EQ(expectedSource.SecondaryId, source.SecondaryId);
		}
	}

	TEST(TEST_CLASS, CommitRelativeRelativeChangesObserverSource) {
		// Assert:
		AssertCommitChangesObserverSource(SourceChangeType::Relative, SourceChangeType::Relative, { 15 + 10, 22 + 5 });
	}

	TEST(TEST_CLASS, CommitAbsoluteRelativeChangesObserverSource) {
		// Assert:
		AssertCommitChangesObserverSource(SourceChangeType::Absolute, SourceChangeType::Relative, { 10, 22 + 5 });
	}

	TEST(TEST_CLASS, CommitRelativeAbsoluteChangesObserverSource) {
		// Assert:
		AssertCommitChangesObserverSource(SourceChangeType::Relative, SourceChangeType::Absolute, { 15 + 10, 5 });
	}

	TEST(TEST_CLASS, CommitAbsoluteAbsoluteChangesObserverSource) {
		// Assert:
		AssertCommitChangesObserverSource(SourceChangeType::Absolute, SourceChangeType::Absolute, { 10, 5 });
	}

	// endregion
}}
