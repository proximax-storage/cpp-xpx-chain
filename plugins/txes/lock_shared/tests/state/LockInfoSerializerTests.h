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
#include "plugins/txes/lock_shared/tests/test/LockInfoCacheTestUtils.h"
#include "tests/test/core/SerializerTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

	// region PackedLockInfo

	/// A packed lock info template.
	template<VersionType version>
	struct PackedLockInfo;

#pragma pack(push, 1)

	/// A packed lock info version 1.
	template<>
	struct PackedLockInfo<1> {
	public:
		/// Creates a lock info from \a lockInfo.
		explicit PackedLockInfo<1>(const LockInfo& lockInfo)
				: Version(1)
				, Account(lockInfo.Account)
				, MosaicId(lockInfo.Mosaics.begin()->first)
				, Amount(lockInfo.Mosaics.begin()->second)
				, Height(lockInfo.Height)
				, Status(lockInfo.Status)
		{}

	public:
		VersionType Version;
		Key Account;
		catapult::MosaicId MosaicId;
		catapult::Amount Amount;
		catapult::Height Height;
		LockStatus Status;
	};

	/// A packed lock info version 2.
	template<>
	struct PackedLockInfo<2> {
	private:
		static constexpr auto Num_Mosaics = 2u;

	public:
		/// Creates a lock info from \a lockInfo.
		explicit PackedLockInfo<2>(const LockInfo& lockInfo)
				: Version(2)
				, Account(lockInfo.Account)
				, Height(lockInfo.Height)
				, Status(lockInfo.Status)
				, MosaicCount(Num_Mosaics) {
			auto i = 0u;
			for (const auto& pair : lockInfo.Mosaics)
				Mosaics[i++] = model::Mosaic{pair.first, pair.second};
		}

	public:
		VersionType Version;
		Key Account;
		catapult::Height Height;
		LockStatus Status;
		uint8_t MosaicCount;
		std::array<model::Mosaic, Num_Mosaics> Mosaics;
	};

#pragma pack(pop)

	// endregion

	/// Lock info serializer test suite.
	template<typename TLockInfoTraits>
	class LockInfoSerializerTests {
	private:
		static void AssertBuffer(const std::vector<typename TLockInfoTraits::ValueType>& values, const std::vector<uint8_t>& buffer) {
			std::vector<typename TLockInfoTraits::PackedValueType> packedValues;
			for (const auto& value : values)
				packedValues.emplace_back(value);

			ASSERT_EQ(sizeof(typename TLockInfoTraits::PackedValueType) * packedValues.size(), buffer.size());
			EXPECT_EQ_MEMORY(packedValues.data(), buffer.data(), buffer.size());
		}

		static void AssertCanSaveLockInfos(size_t numLockInfos) {
			// Arrange:
			std::vector<uint8_t> buffer;
			mocks::MockMemoryStream outputStream(buffer);
			auto lockInfos = test::CreateLockInfos<TLockInfoTraits>(numLockInfos);

			// Act:
			for (const auto& lockInfo : lockInfos)
				TLockInfoTraits::SerializerType::Save(lockInfo, outputStream);

			// Assert:
			AssertBuffer(lockInfos, buffer);
		}

	public:
		static void AssertCanSaveSingleLockInfo() {
			// Assert:
			AssertCanSaveLockInfos(1);
		}

	private:
		static std::vector<uint8_t> CreateBuffer(const std::vector<typename TLockInfoTraits::ValueType>& lockInfos) {
			std::vector<uint8_t> buffer(lockInfos.size() * TLockInfoTraits::ValueTypeSize());

			auto* pData = buffer.data();
			for (const auto& lockInfo : lockInfos) {
				using PackedValueType = typename TLockInfoTraits::PackedValueType;
				PackedValueType packedInfo(lockInfo);
				memcpy(pData, &packedInfo, sizeof(PackedValueType));
				pData += sizeof(PackedValueType);
			}

			return buffer;
		}

	public:
		static void AssertCanLoadSingleLockInfo() {
			// Arrange:
			auto originalLockInfo = test::CreateLockInfos<TLockInfoTraits>(1)[0];
			auto buffer = CreateBuffer({ originalLockInfo });

			// Act:
			typename TLockInfoTraits::ValueType result;
			test::RunLoadValueTest<typename TLockInfoTraits::SerializerType>(buffer, result);

			// Assert:
			TLockInfoTraits::AssertEqual(originalLockInfo, result);
		}
	};
}}

#define MAKE_LOCK_INFO_SERIALIZER_TEST(TRAITS_NAME, TEST_NAME) \
	TEST(TEST_CLASS, TEST_NAME) { LockInfoSerializerTests<TRAITS_NAME>::Assert##TEST_NAME(); }

#define DEFINE_LOCK_INFO_SERIALIZER_TESTS(TRAITS_NAME) \
	MAKE_LOCK_INFO_SERIALIZER_TEST(TRAITS_NAME, CanSaveSingleLockInfo) \
	MAKE_LOCK_INFO_SERIALIZER_TEST(TRAITS_NAME, CanLoadSingleLockInfo)
