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

#include "src/cache/LockFundCacheSerializers.h"
#include "tests/TestHarness.h"
#include "src/cache/LockFundCacheTypes.h"
#include "tests/test/LockFundTestUtils.h"

namespace catapult { namespace cache {

#define TEST_CLASS NamespaceCacheSerializersTests

	namespace {
		using HeightBasedSerializer = LockFundPrimarySerializer;
		using KeyBasedSerializer = KeyedLockFundSerializer;
		const auto MOSAIC_SIZE = sizeof(MosaicId)+sizeof(Amount);
		const auto ACTIVE_RECORD_SIZE = MOSAIC_SIZE+1;
	}

	TEST(TEST_CLASS, HeightBasedSerializer_CanSerializeEmptyRecord) {
		// Arrange:
		auto ns = state::LockFundRecordGroup<LockFundHeightIndexDescriptor>(Height(10), {});

		// Act:
		auto result = HeightBasedSerializer::SerializeValue(ns);

		// Assert:
		ASSERT_EQ(sizeof(VersionType) + sizeof(uint64_t) + sizeof(uint32_t), result.size());

		const auto* pValue = reinterpret_cast<const uint64_t*>(result.data() + sizeof(VersionType));
		EXPECT_EQ(10, *pValue);
	}

	TEST(TEST_CLASS, KeyBasedSerializer_CanSerializeEmptyRecord) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto ns = state::LockFundRecordGroup<LockFundKeyIndexDescriptor>(key, {});

		// Act:
		auto result = KeyBasedSerializer::SerializeValue(ns);

		// Assert:
		ASSERT_EQ(sizeof(VersionType) + sizeof(Key) + sizeof(uint32_t), result.size());

		const auto* pValue = reinterpret_cast<const Key*>(result.data() + sizeof(VersionType));
		EXPECT_EQ(key, *pValue);
	}

	TEST(TEST_CLASS, HeightBasedSerializer_CanSerializeFullValue) {
		// Arrange:
		auto ns = test::GenerateRecordGroup<LockFundHeightIndexDescriptor, test::DefaultRecordGroupGeneratorTraits<LockFundHeightIndexDescriptor>>(Height(10), 3);

		// Act:
		auto result = HeightBasedSerializer::SerializeValue(ns);

		// Assert:

		// Size
		ASSERT_EQ(sizeof(VersionType) + sizeof(uint64_t) + sizeof(uint32_t) + 3 * (sizeof(Key) + sizeof(uint32_t) + ACTIVE_RECORD_SIZE + 2*(MOSAIC_SIZE)), result.size());

		// Data
		const auto* pWalker = reinterpret_cast<const uint8_t*>(result.data() + sizeof(VersionType));
		EXPECT_EQ(10, *reinterpret_cast<const uint64_t*>(pWalker));
		pWalker += sizeof(Height);
		EXPECT_EQ(3, *reinterpret_cast<const uint32_t*>(pWalker));
		pWalker += sizeof(uint32_t);
		for(auto i = 0; i < 3; i++)
		{
			EXPECT_EQ(ns.LockFundRecords.find(*reinterpret_cast<const Key*>(pWalker))->first, *reinterpret_cast<const Key*>(pWalker));
			pWalker += sizeof(Key);
			EXPECT_EQ(*reinterpret_cast<const uint8_t*>(pWalker), 1);
			pWalker += sizeof(uint8_t);
			EXPECT_EQ(*reinterpret_cast<const uint8_t*>(pWalker), 1);
			pWalker += sizeof(uint8_t);
			EXPECT_EQ(*reinterpret_cast<const MosaicId*>(pWalker), MosaicId(72));
			pWalker += sizeof(MosaicId);
			EXPECT_EQ(*reinterpret_cast<const Amount*>(pWalker), Amount(200));
			pWalker += sizeof(Amount);
			EXPECT_EQ(*reinterpret_cast<const uint32_t*>(pWalker), 2);
			pWalker += sizeof(uint32_t);
			EXPECT_EQ(*reinterpret_cast<const MosaicId*>(pWalker), MosaicId(73));
			pWalker += sizeof(MosaicId);
			EXPECT_EQ(*reinterpret_cast<const Amount*>(pWalker), Amount(400));
			pWalker += sizeof(Amount);
			EXPECT_EQ(*reinterpret_cast<const MosaicId*>(pWalker), MosaicId(73));
			pWalker += sizeof(MosaicId);
			EXPECT_EQ(*reinterpret_cast<const Amount*>(pWalker), Amount(400));
		}
	}

	TEST(TEST_CLASS, KeyBasedSerializer_CanSerializeFullValue) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto ns = test::GenerateRecordGroup<LockFundKeyIndexDescriptor, test::DefaultRecordGroupGeneratorTraits<LockFundKeyIndexDescriptor>>(key, 3);

		// Act:
		auto result = KeyBasedSerializer::SerializeValue(ns);

		// Assert:

		// Size
		ASSERT_EQ(sizeof(VersionType) + sizeof(uint64_t) + sizeof(uint32_t) + 3 * (sizeof(Height) + sizeof(uint32_t) + ACTIVE_RECORD_SIZE + 2*(MOSAIC_SIZE)), result.size());

		// Data
		const auto* pWalker = reinterpret_cast<const uint8_t*>(result.data() + sizeof(VersionType));
		EXPECT_EQ(key, *reinterpret_cast<const Key*>(pWalker));
		pWalker += sizeof(Key);
		EXPECT_EQ(3, *reinterpret_cast<const uint32_t*>(pWalker));
		pWalker += sizeof(uint32_t);
		for(auto i = 0; i < 3; i++)
		{
			EXPECT_EQ(*reinterpret_cast<const Height*>(pWalker), Height(i));
			EXPECT_EQ(ns.LockFundRecords.find(*reinterpret_cast<const Height*>(pWalker))->first, *reinterpret_cast<const Height*>(pWalker));
			pWalker += sizeof(Height);
			EXPECT_EQ(*reinterpret_cast<const uint8_t*>(pWalker), 1);
			pWalker += sizeof(uint8_t);
			EXPECT_EQ(*reinterpret_cast<const uint8_t*>(pWalker), 1);
			pWalker += sizeof(uint8_t);
			EXPECT_EQ(*reinterpret_cast<const MosaicId*>(pWalker), MosaicId(72));
			pWalker += sizeof(MosaicId);
			EXPECT_EQ(*reinterpret_cast<const Amount*>(pWalker), Amount(200));
			pWalker += sizeof(Amount);
			EXPECT_EQ(*reinterpret_cast<const uint32_t*>(pWalker), 2);
			pWalker += sizeof(uint32_t);
			EXPECT_EQ(*reinterpret_cast<const MosaicId*>(pWalker), MosaicId(73));
			pWalker += sizeof(MosaicId);
			EXPECT_EQ(*reinterpret_cast<const Amount*>(pWalker), Amount(400));
			pWalker += sizeof(Amount);
			EXPECT_EQ(*reinterpret_cast<const MosaicId*>(pWalker), MosaicId(73));
			pWalker += sizeof(MosaicId);
			EXPECT_EQ(*reinterpret_cast<const Amount*>(pWalker), Amount(400));
		}
	}

	TEST(TEST_CLASS, HeightBasedSerializer_CanDerializeEmptyRecord) {
		// Arrange:
		auto record = state::LockFundRecordGroup<LockFundHeightIndexDescriptor>(Height(10), {});
		auto serialized = HeightBasedSerializer::SerializeValue(record);

		// Act:
		auto ns = HeightBasedSerializer::DeserializeValue({ reinterpret_cast<uint8_t*>(serialized.data()), sizeof(VersionType) + sizeof(uint64_t) + sizeof(uint32_t) + 3 * (sizeof(Key) + sizeof(uint32_t) + ACTIVE_RECORD_SIZE + 2*(MOSAIC_SIZE)) });
		EXPECT_EQ(record.Identifier, ns.Identifier);
		EXPECT_EQ(record.LockFundRecords.size(), ns.LockFundRecords.size());
		EXPECT_EQ(ns.LockFundRecords.size(), 0);

	}

	TEST(TEST_CLASS, KeyBasedSerializer_CanDerializeEmptyRecord) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto record = state::LockFundRecordGroup<LockFundKeyIndexDescriptor>(key, {});
		auto serialized = KeyBasedSerializer::SerializeValue(record);

		// Act:
		auto ns = KeyBasedSerializer::DeserializeValue({ reinterpret_cast<uint8_t*>(serialized.data()), sizeof(VersionType) + sizeof(Key) + sizeof(uint32_t) + 3 * (sizeof(Height) + sizeof(uint32_t) + ACTIVE_RECORD_SIZE + 2*(MOSAIC_SIZE)) });
		EXPECT_EQ(record.Identifier, ns.Identifier);
		EXPECT_EQ(record.LockFundRecords.size(), ns.LockFundRecords.size());
		EXPECT_EQ(ns.LockFundRecords.size(), 0);

	}

	TEST(TEST_CLASS, HeightBasedSerializer_CanDerializeFullRecord) {
		// Arrange:
		auto record = test::GenerateRecordGroup<LockFundHeightIndexDescriptor, test::DefaultRecordGroupGeneratorTraits<LockFundHeightIndexDescriptor>>(Height(10), 3);
		auto serialized = HeightBasedSerializer::SerializeValue(record);

		// Act:
		auto ns = HeightBasedSerializer::DeserializeValue({ reinterpret_cast<uint8_t*>(serialized.data()), sizeof(VersionType) + sizeof(uint64_t) + sizeof(uint32_t) + 3 * (sizeof(Key) + sizeof(uint32_t) + ACTIVE_RECORD_SIZE + 2*(MOSAIC_SIZE)) });
		EXPECT_EQ(record.Identifier, ns.Identifier);
		EXPECT_EQ(record.LockFundRecords.size(), ns.LockFundRecords.size());
		EXPECT_EQ(ns.LockFundRecords.size(), 3);
		for(auto &pair : record.LockFundRecords)
		{
			auto deserializedPair = ns.LockFundRecords.find(pair.first);
			EXPECT_EQ(pair.first, deserializedPair->first);
			EXPECT_EQ(pair.second.Size(), deserializedPair->second.Size());
			EXPECT_EQ(pair.second.Active(), deserializedPair->second.Active());
			EXPECT_EQ(pair.second.Get().find(MosaicId(72))->second, deserializedPair->second.Get().find(MosaicId(72))->second);
			EXPECT_EQ(pair.second.InactiveRecords[0].find(MosaicId(73))->second, deserializedPair->second.InactiveRecords[0].find(MosaicId(73))->second);
			EXPECT_EQ(pair.second.InactiveRecords[1].find(MosaicId(73))->second, deserializedPair->second.InactiveRecords[1].find(MosaicId(73))->second);
		}

	}

	TEST(TEST_CLASS, KeyBasedSerializer_CanDerializeFullRecord) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto record = test::GenerateRecordGroup<LockFundKeyIndexDescriptor, test::DefaultRecordGroupGeneratorTraits<LockFundKeyIndexDescriptor>>(key, 3);
		auto serialized = KeyBasedSerializer::SerializeValue(record);

		// Act:
		auto ns = KeyBasedSerializer::DeserializeValue({ reinterpret_cast<uint8_t*>(serialized.data()), sizeof(VersionType) + sizeof(Key) + sizeof(uint32_t) + 3 * (sizeof(Height) + sizeof(uint32_t) + ACTIVE_RECORD_SIZE + 2*(MOSAIC_SIZE)) });
		EXPECT_EQ(record.Identifier, ns.Identifier);
		EXPECT_EQ(record.LockFundRecords.size(), ns.LockFundRecords.size());
		EXPECT_EQ(ns.LockFundRecords.size(), 3);
		for(auto &pair : record.LockFundRecords)
		{
			auto deserializedPair = ns.LockFundRecords.find(pair.first);
			EXPECT_EQ(pair.first, deserializedPair->first);
			EXPECT_EQ(pair.second.Size(), deserializedPair->second.Size());
			EXPECT_EQ(pair.second.Active(), deserializedPair->second.Active());
			EXPECT_EQ(pair.second.Get().find(MosaicId(72))->second, deserializedPair->second.Get().find(MosaicId(72))->second);
			EXPECT_EQ(pair.second.InactiveRecords[0].find(MosaicId(73))->second, deserializedPair->second.InactiveRecords[0].find(MosaicId(73))->second);
			EXPECT_EQ(pair.second.InactiveRecords[1].find(MosaicId(73))->second, deserializedPair->second.InactiveRecords[1].find(MosaicId(73))->second);
		}

	}
}}
