/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#include "src/state/LevyEntrySerializer.h"
#include "tests/test/core/mocks/MockMemoryStream.h"
#include "tests/TestHarness.h"
#include "src/model/MosaicLevy.h"
#include <tests/test/LevyTestUtils.h>

namespace catapult { namespace state {

#define TEST_CLASS LevyEntrySerializerTests
		
		// region headers

#pragma pack(push, 1)
		
		namespace {
			struct LevyEntryHeader {
				VersionType Version;
				catapult::MosaicId MosaicId;
				model::MosaicLevy Levy;
			};
			
			void AssertEntryHeader(
				const std::vector<uint8_t>& buffer,
				VersionType version,
				MosaicId mosaicId,
				model::MosaicLevy levy) {
				auto message = "entry header at 0";
				const auto& entryHeader = reinterpret_cast<const LevyEntryHeader&>(buffer[0]);
				
				// - id and supply
				EXPECT_EQ(version, entryHeader.Version) << message;
				EXPECT_EQ(mosaicId, entryHeader.MosaicId) << message;
				
				EXPECT_EQ(entryHeader.Levy.Type, levy.Type);
				EXPECT_EQ(entryHeader.Levy.Fee, levy.Fee);
				EXPECT_EQ(entryHeader.Levy.Recipient, levy.Recipient);
				EXPECT_EQ(entryHeader.Levy.MosaicId, levy.MosaicId);
			}
		}

#pragma pack(pop)
		
		// endregion
		
		// region Save
		
		TEST(TEST_CLASS, CanSaveEntry) {
			// Arrange:
			std::vector<uint8_t> buffer;
			mocks::MockMemoryStream stream(buffer);
			auto levy = test::CreateValidMosaicLevy();
			auto entry = LevyEntry(MosaicId(123), levy);
			
			// Act:
			LevyEntrySerializer::Save(entry, stream);
			
			// Assert:
			AssertEntryHeader(buffer, VersionType{1}, MosaicId(123), levy);
		}
		
		// endregion
		
		// region Load
		
		namespace {
			void AssertLevyEntry(
				const state::LevyEntry& entry,
				MosaicId mosaicId,
				model::MosaicLevy levy) {
				auto message = "entry " + std::to_string(entry.mosaicId().unwrap());
				
				// - entry
				EXPECT_EQ(mosaicId, entry.mosaicId()) << message;
				EXPECT_EQ(entry.levy().Type, levy.Type);
				EXPECT_EQ(entry.levy().Fee, levy.Fee);
				EXPECT_EQ(entry.levy().Recipient, levy.Recipient);
				EXPECT_EQ(entry.levy().MosaicId, levy.MosaicId);
			}
		}
		
		TEST(TEST_CLASS, CanLoadEntry) {
			// Arrange:
			std::vector<uint8_t> buffer(sizeof(LevyEntryHeader));
			auto levy = test::CreateValidMosaicLevy();
			reinterpret_cast<LevyEntryHeader&>(buffer[0]) = { VersionType{1}, MosaicId(123), {levy.Type, levy.Recipient, levy.MosaicId, levy.Fee}};
			mocks::MockMemoryStream stream(buffer);
			
			// Act:
			auto entry = LevyEntrySerializer::Load(stream);
			
			// Assert:
			AssertLevyEntry(entry, MosaicId(123), levy);
		}
		
		// endregion
	}}
