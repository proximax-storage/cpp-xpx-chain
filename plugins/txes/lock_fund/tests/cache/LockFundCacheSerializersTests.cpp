/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/LockFundCacheSerializers.h"
#include "tests/test/LockFundTestUtils.h"

namespace catapult { namespace cache {

#define TEST_CLASS LockFundCacheSerializersTests

	namespace {
		using HeightBasedSerializer = LockFundPrimarySerializer;
		using KeyBasedSerializer = KeyedLockFundSerializer;
		const auto MOSAIC_SIZE = sizeof(MosaicId)+sizeof(Amount);
		template<uint32_t MosaicCount>
		const auto ACTIVE_RECORD_SIZE = MosaicCount * (MOSAIC_SIZE+1);
		template<typename TIdentifier, uint32_t TRecordCount, uint32_t TActiveRecordMosaicCount, uint32_t TInactiveRecordCount, uint32_t TInactiveRecordMosaicCount>
		const auto FULL_HEIGHT_RECORD_GROUP_SIZE = sizeof(VersionType) + // Version
									  + sizeof(typename TIdentifier::KeyType)// Identifier
									  + sizeof(uint32_t)// Size
									  + TRecordCount// Number of records
									  * (sizeof(typename TIdentifier::ValueIdentifier) // Inner Identifier
									  	+ 1 // hasActiveRecord
									  	+ sizeof(uint32_t) // Size of Inactive records
									    + ACTIVE_RECORD_SIZE<TActiveRecordMosaicCount> // Size of active record
									    + TInactiveRecordCount *(ACTIVE_RECORD_SIZE<TInactiveRecordMosaicCount>)); // Size of inactive records

	}

	TEST(TEST_CLASS, HeightBasedSerializer_CanSerializeEmptyRecord) {
		// Arrange:
		auto ns = state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>(Height(10), {});

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
		auto ns = state::LockFundRecordGroup<state::LockFundKeyIndexDescriptor>(key, {});

		// Act:
		auto result = KeyBasedSerializer::SerializeValue(ns);

		// Assert:
		ASSERT_EQ(sizeof(VersionType) + sizeof(Key) + sizeof(uint32_t), result.size());

		const auto* pValue = reinterpret_cast<const Key*>(result.data() + sizeof(VersionType));
		EXPECT_EQ(key, *pValue);
	}

	TEST(TEST_CLASS, HeightBasedSerializer_CanSerializeFullValue) {
		// Arrange:
		auto ns = test::GenerateRecordGroup<state::LockFundHeightIndexDescriptor, test::DefaultRecordGroupGeneratorTraits<state::LockFundHeightIndexDescriptor>>(Height(10), 3);

		// Act:
		auto result = HeightBasedSerializer::SerializeValue(ns);

		// Assert:

		// Size
		ASSERT_EQ((FULL_HEIGHT_RECORD_GROUP_SIZE<state::LockFundHeightIndexDescriptor, 3, 1, 2, 1>), result.size());

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
			EXPECT_EQ(*reinterpret_cast<const uint8_t*>(pWalker), 1);
			pWalker += sizeof(uint8_t);
			EXPECT_EQ(*reinterpret_cast<const MosaicId*>(pWalker), MosaicId(73));
			pWalker += sizeof(MosaicId);
			EXPECT_EQ(*reinterpret_cast<const Amount*>(pWalker), Amount(400));
			pWalker += sizeof(Amount);
			EXPECT_EQ(*reinterpret_cast<const uint8_t*>(pWalker), 1);
			pWalker += sizeof(uint8_t);
			EXPECT_EQ(*reinterpret_cast<const MosaicId*>(pWalker), MosaicId(73));
			pWalker += sizeof(MosaicId);
			EXPECT_EQ(*reinterpret_cast<const Amount*>(pWalker), Amount(400));
			pWalker += sizeof(Amount);
		}
	}

	TEST(TEST_CLASS, KeyBasedSerializer_CanSerializeFullValue) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto ns = test::GenerateRecordGroup<state::LockFundKeyIndexDescriptor, test::DefaultRecordGroupGeneratorTraits<state::LockFundKeyIndexDescriptor>>(key, 3);

		// Act:
		auto result = KeyBasedSerializer::SerializeValue(ns);

		// Assert:

		// Size
		ASSERT_EQ((FULL_HEIGHT_RECORD_GROUP_SIZE<state::LockFundKeyIndexDescriptor, 3, 1, 2, 1>), result.size());

		// Data
		const auto* pWalker = reinterpret_cast<const uint8_t*>(result.data() + sizeof(VersionType));
		EXPECT_EQ(key, *reinterpret_cast<const Key*>(pWalker));
		pWalker += sizeof(Key);
		EXPECT_EQ(3, *reinterpret_cast<const uint32_t*>(pWalker));
		pWalker += sizeof(uint32_t);
		for(auto i = 0; i < 3; i++)
		{
			EXPECT_NE(ns.LockFundRecords.find(*reinterpret_cast<const Height*>(pWalker)), ns.LockFundRecords.end());
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
			EXPECT_EQ(*reinterpret_cast<const uint8_t*>(pWalker), 1);
			pWalker += sizeof(uint8_t);
			EXPECT_EQ(*reinterpret_cast<const MosaicId*>(pWalker), MosaicId(73));
			pWalker += sizeof(MosaicId);
			EXPECT_EQ(*reinterpret_cast<const Amount*>(pWalker), Amount(400));
			pWalker += sizeof(Amount);
			EXPECT_EQ(*reinterpret_cast<const uint8_t*>(pWalker), 1);
			pWalker += sizeof(uint8_t);
			EXPECT_EQ(*reinterpret_cast<const MosaicId*>(pWalker), MosaicId(73));
			pWalker += sizeof(MosaicId);
			EXPECT_EQ(*reinterpret_cast<const Amount*>(pWalker), Amount(400));
			pWalker += sizeof(Amount);
		}
	}

	TEST(TEST_CLASS, HeightBasedSerializer_CanDerializeEmptyRecord) {
		// Arrange:
		auto record = state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>(Height(10), {});
		auto serialized = HeightBasedSerializer::SerializeValue(record);

		// Act:
		auto ns = HeightBasedSerializer::DeserializeValue({ reinterpret_cast<uint8_t*>(serialized.data()), (FULL_HEIGHT_RECORD_GROUP_SIZE<state::LockFundHeightIndexDescriptor, 0, 0, 0, 0>) });
		EXPECT_EQ(record.Identifier, ns.Identifier);
		EXPECT_EQ(record.LockFundRecords.size(), ns.LockFundRecords.size());
		EXPECT_EQ(ns.LockFundRecords.size(), 0);

	}

	TEST(TEST_CLASS, KeyBasedSerializer_CanDerializeEmptyRecord) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto record = state::LockFundRecordGroup<state::LockFundKeyIndexDescriptor>(key, {});
		auto serialized = KeyBasedSerializer::SerializeValue(record);

		// Act:
		auto ns = KeyBasedSerializer::DeserializeValue({ reinterpret_cast<uint8_t*>(serialized.data()), (FULL_HEIGHT_RECORD_GROUP_SIZE<state::LockFundKeyIndexDescriptor, 0, 0, 0, 0>) });
		EXPECT_EQ(record.Identifier, ns.Identifier);
		EXPECT_EQ(record.LockFundRecords.size(), ns.LockFundRecords.size());
		EXPECT_EQ(ns.LockFundRecords.size(), 0);

	}

	TEST(TEST_CLASS, HeightBasedSerializer_CanDerializeFullRecord) {
		// Arrange:
		auto record = test::GenerateRecordGroup<state::LockFundHeightIndexDescriptor, test::DefaultRecordGroupGeneratorTraits<state::LockFundHeightIndexDescriptor>>(Height(10), 3);
		auto serialized = HeightBasedSerializer::SerializeValue(record);

		// Act:
		auto ns = HeightBasedSerializer::DeserializeValue({ reinterpret_cast<uint8_t*>(serialized.data()), (FULL_HEIGHT_RECORD_GROUP_SIZE<state::LockFundHeightIndexDescriptor, 3, 1, 2, 1>) });
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
		auto record = test::GenerateRecordGroup<state::LockFundKeyIndexDescriptor, test::DefaultRecordGroupGeneratorTraits<state::LockFundKeyIndexDescriptor>>(key, 3);
		auto serialized = KeyBasedSerializer::SerializeValue(record);

		// Act:
		auto ns = KeyBasedSerializer::DeserializeValue({ reinterpret_cast<uint8_t*>(serialized.data()), (FULL_HEIGHT_RECORD_GROUP_SIZE<state::LockFundKeyIndexDescriptor, 3, 1, 2, 1>) });
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
