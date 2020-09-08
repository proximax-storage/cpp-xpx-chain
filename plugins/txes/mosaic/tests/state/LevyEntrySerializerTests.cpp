/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/core/mocks/MockMemoryStream.h"
#include "tests/TestHarness.h"
#include "tests/test/LevyTestUtils.h"

namespace catapult { namespace state {

#define TEST_CLASS LevyEntrySerializerTests
		
	// region headers

	namespace {
		
		constexpr size_t History_Count = 1;
		constexpr auto LevyDataEntry_Size =
			sizeof(model::LevyType)
			+ Address_Decoded_Size
			+ sizeof(uint64_t)
			+ sizeof(uint64_t);
		
		constexpr auto Entry_Size_With_History =
			sizeof(VersionType)
			+ sizeof(uint64_t)
			+ sizeof(uint8_t)
			+ LevyDataEntry_Size
			+ sizeof(uint16_t);
		
		constexpr auto Entry_Size_Without_History =
			sizeof(VersionType)
			+ sizeof(uint64_t)
			+ sizeof(uint8_t)
			+ LevyDataEntry_Size;

		void WriteLevyData(const LevyEntryData& entryData, uint8_t*& pData) {
			
			memcpy(pData, &entryData.Type, sizeof(model::LevyType));
			pData += sizeof(model::LevyType);
			
			memcpy(pData, entryData.Recipient.data(), Address_Decoded_Size);
			pData += Address_Decoded_Size;
			
			auto value = entryData.MosaicId.unwrap();
			memcpy(pData, &value, sizeof(uint64_t));
			pData += sizeof(uint64_t);
			
			value = entryData.Fee.unwrap();
			memcpy(pData, &value, sizeof(uint64_t));
			pData += sizeof(uint64_t);
			
		}

		std::vector<uint8_t> CreateEntryBuffer(const LevyEntry& entry, size_t expectedSize, bool writeHistory) {
			std::vector<uint8_t> buffer(expectedSize);
			auto* pData = buffer.data();
			auto version = 1;
			memcpy(pData, &version, sizeof(VersionType));
			pData += sizeof(VersionType);
			
			auto value = entry.mosaicId().unwrap();
			memcpy(pData, &value, sizeof(uint64_t));
			pData += sizeof(uint64_t);
			
			uint8_t flag = 0;
			if(entry.levy()) {
				flag = 1;
				memcpy(pData, &flag, sizeof(uint8_t));
				pData += sizeof(uint8_t);
				
				WriteLevyData(*entry.levy(), pData);
			}else{
				memcpy(pData, &flag, sizeof(uint8_t));
				pData += sizeof(uint8_t);
			}
			
			if (writeHistory) {
				auto size = utils::checked_cast<size_t, uint16_t>(entry.updateHistory().size());
				memcpy(pData, &size, sizeof(uint16_t));
				pData += sizeof(uint16_t);
				if( size > 0) {
					for (const auto &pair : entry.updateHistory()) {
						auto height = pair.first.unwrap();
						memcpy(pData, &height, sizeof(uint64_t));
						pData += sizeof(uint64_t);
						WriteLevyData(pair.second, pData);
					}
				}
			}
			
			return buffer;
		}

		void AssertLevyData(
			const state::LevyEntryData& rhs,
			const state::LevyEntryData& lhs) {

			EXPECT_EQ(rhs.Type, lhs.Type);
			EXPECT_EQ(rhs.Fee, lhs.Fee);
			EXPECT_EQ(rhs.Recipient, lhs.Recipient);
			EXPECT_EQ(rhs.MosaicId, lhs.MosaicId);
		}

		template<std::size_t N>
		std::array<uint8_t, N> Extract(const uint8_t* data) {
			std::array<uint8_t, N> buffer = { 0 };
			memcpy(buffer.data(), data, N);
			return buffer;
		}
		
		void AssertLevyEntry(const state::LevyEntryData& data, const uint8_t*& pData) {
			EXPECT_EQ(data.Type, *reinterpret_cast<const model::LevyType*>(pData));
			pData += sizeof(model::LevyType);
			
			std::array<uint8_t, Address_Decoded_Size> recipient = { 0 };
			memcpy(recipient.data(), pData, Address_Decoded_Size);
			
			EXPECT_EQ(data.Recipient, recipient);
			pData += Address_Decoded_Size;
			
			EXPECT_EQ(data.MosaicId.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			
			EXPECT_EQ(data.Fee.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
		}

		void AssertLevyEntry(
			const state::LevyEntry& rhs,
			const state::LevyEntry& lhs,
			bool withHistory = false) {
			
			EXPECT_EQ(rhs.mosaicId(), lhs.mosaicId());
			
			if (rhs.levy() && lhs.levy()) {
				AssertLevyData(*rhs.levy(), *lhs.levy());
		    } else {
				EXPECT_EQ(rhs.levy(), nullptr);
				EXPECT_EQ(lhs.levy(), nullptr);
			}
			
			if(!withHistory) {
				EXPECT_EQ(rhs.updateHistory().size(), 0);
				return;
			}
			
			EXPECT_EQ(rhs.updateHistory().size(), lhs.updateHistory().size());
			
			for( size_t i = 0; i < rhs.updateHistory().size(); i++) {
				
				auto rhsPair = rhs.updateHistory()[i];
				auto lhsPair = lhs.updateHistory()[i];
				
				EXPECT_EQ(rhsPair.first, lhsPair.first);
				AssertLevyData(rhsPair.second, lhsPair.second);
			}
		}

		void AssertEntryBuffer(const LevyEntry& entry, const uint8_t* pData, size_t expectedSize, bool assertLevyData, bool assertHistory) {
			const auto* pExpectedEnd = pData + expectedSize;
			
			EXPECT_EQ(1, *reinterpret_cast<const VersionType*>(pData));
			pData += sizeof(VersionType);
			
			EXPECT_EQ(entry.mosaicId().unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			
			EXPECT_EQ(entry.levy()? 1:0, *reinterpret_cast<const uint8_t*>(pData));
			pData += sizeof(uint8_t);
			
			if(assertLevyData && entry.levy()) {
				AssertLevyEntry(*entry.levy(), pData);
			}
			
			if(assertHistory)  {
				EXPECT_EQ(entry.updateHistory().size(), *reinterpret_cast<const uint16_t*>(pData));
				pData += sizeof(uint16_t);
				
				if( entry.updateHistory().size() > 0) {
					for (const auto &pair : entry.updateHistory()) {
						EXPECT_EQ(pair.first.unwrap(), *reinterpret_cast<const uint64_t *>(pData));
						pData += sizeof(uint64_t);
						
						AssertLevyEntry(pair.second, pData);
					}
				}
			}
			
			EXPECT_EQ(pExpectedEnd, pData);
		}
	}
	
	// endregion
	
	// region Save
	namespace {
		template<typename TSerializer>
		void SerializerSaveTest(bool withLevy,  bool withHistoryEntry, bool writeHistory = true) {
			
			std::vector<uint8_t> buffer;
			mocks::MockMemoryStream stream(buffer);
			auto entry = test::CreateLevyEntry(withLevy, withHistoryEntry);
			size_t size = writeHistory? Entry_Size_With_History +
				(entry.updateHistory().size() * (sizeof(uint64_t) + LevyDataEntry_Size))
				: Entry_Size_Without_History;
			
			if(!withLevy)
				size -= LevyDataEntry_Size;
			
			// Act:
			TSerializer::Save(entry, stream);
			
			// Assert:
			AssertEntryBuffer(entry, buffer.data(), size, withLevy, writeHistory);
		}
	}
	
	TEST(TEST_CLASS, CanSaveEntry) {
		bool withLevy = true;
		for(auto i = 0; i < 2; i++) {
			bool withHistory = true;
			for(auto j = 0; j < 2; j++) {
				SerializerSaveTest<LevyEntrySerializer>(withLevy, withHistory);
				SerializerSaveTest<LevyEntryNonHistoricalSerializer>(withLevy, withHistory,false);
				withHistory = !withHistory;
			}
			withLevy = !withLevy;
		}
	}
	// endregion

	// region Load
	namespace {
		template<typename TSerializer>
		void SerializerLoadTest(bool withLevy, bool withHistoryEntry, bool bufferWithHistory = true) {
			
			auto inputEntry = test::CreateLevyEntry(withLevy, withHistoryEntry);
			
			size_t size = bufferWithHistory? Entry_Size_With_History +
				(inputEntry.updateHistory().size() * (sizeof(uint64_t) + LevyDataEntry_Size))
			    : Entry_Size_Without_History;
			
			if(!withLevy)
				size -= LevyDataEntry_Size;
			
			std::vector<uint8_t>  buffer = CreateEntryBuffer(inputEntry, size, bufferWithHistory);
			mocks::MockMemoryStream stream(buffer);
			
			// Act:
			auto entry = TSerializer::Load(stream);
			
			// Assert:
			AssertLevyEntry(entry, inputEntry, bufferWithHistory);
		}
	}
	
	TEST(TEST_CLASS, CanLoadEntry) {
		bool withLevy = true;
		for(auto i = 0; i < 2; i++) {
			bool withHistoryEntry = true;
			for(auto j = 0; j < 2; j++) {
				SerializerLoadTest<LevyEntrySerializer>(withLevy, withHistoryEntry);
				SerializerLoadTest<LevyEntryNonHistoricalSerializer>(withLevy, withHistoryEntry, false);
				withHistoryEntry = !withHistoryEntry;
			}
			withLevy = !withLevy;
		}
	}
	// endregion
}}
