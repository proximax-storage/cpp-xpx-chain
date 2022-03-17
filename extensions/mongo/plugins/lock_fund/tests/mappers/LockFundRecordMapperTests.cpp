/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "plugins/txes/lock_fund/tests/test/LockFundTestUtils.h"
#include "src/mappers/LockFundRecordMapper.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "tests/TestHarness.h"
#include <bsoncxx/builder/stream/document.hpp>
#include <set>
#include "src/state/LockFundRecordGroup.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS LockFundRecordMapperTests

	namespace {

		void AssertMosaicMatch(const bsoncxx::array::view& array, std::map<MosaicId, Amount> mosaics)
		{
			ASSERT_EQ(mosaics.size(), test::GetFieldCount(array));
			auto mosaicsIter = array.cbegin();
			auto pMosaic = mosaics.cbegin();
			for (auto i = 0u; i < mosaics.size(); ++i) {
				EXPECT_EQ(pMosaic->first, MosaicId(test::GetUint64(mosaicsIter->get_document().view(), "id")));
				EXPECT_EQ(pMosaic->second, Amount(test::GetUint64(mosaicsIter->get_document().view(), "amount")));
				++pMosaic;
				++mosaicsIter;
			}
		}
		template<typename TDescriptor>
		typename TDescriptor::ValueIdentifier GetValueIdentifier(const bsoncxx::document::view& view)
		{
			if constexpr(std::is_same_v<typename TDescriptor::ValueIdentifier, Key>)
				return test::GetKeyValue(view, "key");
			else return test::GetHeight(view, "key");
		}

		void AssertCanMapLockFundRecord(const bsoncxx::document::view view, const state::LockFundRecord& record)
		{
			auto dbMosaics = view["activeMosaics"].get_array().value;
			auto& recordActiveMosaics = record.Get();

			AssertMosaicMatch(dbMosaics, recordActiveMosaics);

			auto inactiveRecords = view["inactiveRecords"].get_array().value;

			auto inactiveRecordsIterator = inactiveRecords.cbegin();
			auto pInactiveRecord = record.InactiveRecords.cbegin();

			ASSERT_EQ(record.InactiveRecords.size(), test::GetFieldCount(inactiveRecords));
			for (auto i = 0u; i < record.InactiveRecords.size(); ++i) {

				auto inactiveRecord = inactiveRecordsIterator->get_document().view();;
				auto innerMosaics = inactiveRecord["mosaics"].get_array().value;
				AssertMosaicMatch(innerMosaics, *pInactiveRecord);
				++pInactiveRecord;
				++inactiveRecordsIterator;
			}
		}

		template<typename TDescriptor>
		void AssertCanMapLockFundRecordGroup(uint32_t numRecords, typename TDescriptor::KeyType Identifier) {
			// Arrange:
			auto descriptor = test::GenerateRecordGroup<TDescriptor, test::DefaultRecordGroupGeneratorTraits<TDescriptor>>(Identifier, numRecords);

			// Act:
			auto document = ToDbModel(descriptor);
			auto documentView = document.view();

			// Assert:
			EXPECT_EQ(2u, test::GetFieldCount(documentView));

			auto lockFundRecords = documentView["records"].get_array().value;

			ASSERT_EQ(numRecords, test::GetFieldCount(lockFundRecords));
			auto iter = lockFundRecords.cbegin();
			for (auto i = 0u; i < numRecords; ++i) {
				auto view = iter->get_document().view();
				auto record = descriptor.LockFundRecords.find(GetValueIdentifier<TDescriptor>(view));
				ASSERT_NE(record, descriptor.LockFundRecords.end());
				AssertCanMapLockFundRecord(view, record->second);
				iter++;
			}
		}
	}

	// region ToDbModel

	TEST(TEST_CLASS, CanMapLockFundRecordHeightIndexWithoutRecords) {
		// Assert:
		AssertCanMapLockFundRecordGroup<state::LockFundHeightIndexDescriptor>(0, Height(10));
	}

	TEST(TEST_CLASS, CanMapLockFundRecordKeyIndexWithoutRecords) {
		// Assert:
		AssertCanMapLockFundRecordGroup<state::LockFundKeyIndexDescriptor>(0, test::GenerateRandomByteArray<Key>());
	}

	TEST(TEST_CLASS, CanMapLockFundRecordHeightIndexWithSingleRecord) {
		// Assert:
		AssertCanMapLockFundRecordGroup<state::LockFundHeightIndexDescriptor>(1, Height(10));
	}

	TEST(TEST_CLASS, CanMapLockFundRecordKeyIndexWithSingleRecord) {
		// Assert:
		AssertCanMapLockFundRecordGroup<state::LockFundKeyIndexDescriptor>(1, test::GenerateRandomByteArray<Key>());
	}

	TEST(TEST_CLASS, CanMapLockFundRecordHeightIndexWithMultipleRecords) {
		// Assert:
		AssertCanMapLockFundRecordGroup<state::LockFundHeightIndexDescriptor>(3, Height(10));
	}

	TEST(TEST_CLASS, CanMapLockFundRecordKeyIndexWithMultipleRecords) {
		// Assert:
		AssertCanMapLockFundRecordGroup<state::LockFundKeyIndexDescriptor>(3, test::GenerateRandomByteArray<Key>());
	}

	// endregion
}}}
