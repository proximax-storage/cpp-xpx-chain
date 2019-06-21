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
#include "tests/TestHarness.h"

namespace catapult { namespace state {

	/// Lock info test suite.
	template<typename TLockInfo>
	class LockInfoTests {
	public:
		static void AssertIsActiveReturnsTrueWhenHeightIsLessThanUnusedLockInfoHeight() {
			// Arrange:
			TLockInfo lockInfo;
			lockInfo.Status = LockStatus::Unused;
			lockInfo.Height = Height(123);

			// Act + Assert:
			EXPECT_TRUE(lockInfo.isActive(Height(122)));
			EXPECT_TRUE(lockInfo.isActive(Height(1)));
		}

		static void AssertIsActiveReturnsFalseWhenHeightIsGreaterThanOrEqualToUnusedLockInfoHeight() {
			// Assert:
			AssertIsActiveReturnsFalseWhenHeightIsGreaterThanOrEqualToLockInfoHeight(LockStatus::Unused);
		}

		static void AssertIsActiveReturnsFalseWhenHeightIsLessThanUsedLockInfoHeight() {
			// Arrange:
			TLockInfo lockInfo;
			lockInfo.Status = LockStatus::Used;
			lockInfo.Height = Height(123);

			// Act + Assert:
			EXPECT_FALSE(lockInfo.isActive(Height(122)));
			EXPECT_FALSE(lockInfo.isActive(Height(1)));
		}

		static void AssertIsActiveReturnsFalseWhenHeightIsGreaterThanOrEqualToUsedLockInfoHeight() {
			// Assert:
			AssertIsActiveReturnsFalseWhenHeightIsGreaterThanOrEqualToLockInfoHeight(LockStatus::Used);
		}

	private:
		static void AssertIsActiveReturnsFalseWhenHeightIsGreaterThanOrEqualToLockInfoHeight(LockStatus status) {
			// Arrange:
			TLockInfo lockInfo;
			lockInfo.Status = status;
			lockInfo.Height = Height(123);

			// Act + Assert:
			EXPECT_FALSE(lockInfo.isActive(Height(123)));
			EXPECT_FALSE(lockInfo.isActive(Height(124)));
			EXPECT_FALSE(lockInfo.isActive(Height(1111)));
		}
	};
}}

#define MAKE_LOCK_INFO_TEST(LOCK_INFO_TYPE, TEST_NAME) \
	TEST(TEST_CLASS, TEST_NAME) { LockInfoTests<LOCK_INFO_TYPE>::Assert##TEST_NAME(); }

#define DEFINE_LOCK_INFO_TESTS(LOCK_INFO_TYPE) \
	MAKE_LOCK_INFO_TEST(LOCK_INFO_TYPE, IsActiveReturnsTrueWhenHeightIsLessThanUnusedLockInfoHeight) \
	MAKE_LOCK_INFO_TEST(LOCK_INFO_TYPE, IsActiveReturnsFalseWhenHeightIsGreaterThanOrEqualToUnusedLockInfoHeight) \
	MAKE_LOCK_INFO_TEST(LOCK_INFO_TYPE, IsActiveReturnsFalseWhenHeightIsLessThanUsedLockInfoHeight) \
	MAKE_LOCK_INFO_TEST(LOCK_INFO_TYPE, IsActiveReturnsFalseWhenHeightIsGreaterThanOrEqualToUsedLockInfoHeight)
