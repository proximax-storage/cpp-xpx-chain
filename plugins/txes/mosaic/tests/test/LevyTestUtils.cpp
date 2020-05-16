/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "LevyTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {
		
	state::LevyEntryData CreateValidMosaicLevy() {
		auto levy = state::LevyEntryData();
		levy.Type = model::LevyType::Absolute;
		levy.Recipient = test::GenerateRandomByteArray<Address>();
		levy.MosaicId = MosaicId((test::Random() % 100 + 1));
		levy.Fee = Amount(test::Random() % 10000);
		return levy;
	}
	
	state::LevyEntry CreateLevyEntry(const MosaicId& mosaicId, state::LevyEntryData& levy,
		bool withLevy, bool withHistory, size_t historyCount,  const Height& baseHeight) {
		
		std::shared_ptr<state::LevyEntryData> pLevy(nullptr);
		if(withLevy)
			pLevy = std::make_shared<state::LevyEntryData>(levy);
		
		auto entry = state::LevyEntry(mosaicId, std::move(pLevy));
		
		if(withHistory) {
			for( auto i = 0u; i < historyCount; i++)
				entry.updateHistory().push_back(
					std::make_pair(Height(baseHeight.unwrap() + i), test::CreateValidMosaicLevy()));
		}
		
		return entry;
	}
	
	state::LevyEntry CreateLevyEntry(bool withLevy, bool withHistory) {
		auto mosaicId = MosaicId(test::Random());
		auto levy = test::CreateValidMosaicLevy();
		return CreateLevyEntry(mosaicId, levy, withLevy, withHistory);
	}
	
	Amount CreateMosaicLevyFeePercentile(float percentage) {
		return Amount( percentage * model::MosaicLevyFeeDecimalPlace);
	}
		
	void AddMosaicWithLevy(cache::CatapultCacheDelta& cache, MosaicId id, Height height, state::LevyEntryData levy) {
		auto& mosaicCacheDelta = cache.sub<cache::MosaicCache>();
		auto definition = state::MosaicDefinition(height, Key(), 1, model::MosaicProperties::FromValues({ { 1, 2, 20 } }));
		auto entry = state::MosaicEntry(id, definition);
		mosaicCacheDelta.insert(entry);
		
		auto& levyCacheDelta = cache.sub<cache::LevyCache>();
		auto levyEntry = CreateLevyEntry(id, levy, true, false);
		levyCacheDelta.insert(levyEntry);
	}
	
	void AddMosaicWithLevy(cache::CatapultCacheDelta& cache, MosaicId id, Height height, state::LevyEntryData levy, const Key& owner) {
		auto& mosaicCacheDelta = cache.sub<cache::MosaicCache>();
		auto definition = state::MosaicDefinition(height, owner, 1, model::MosaicProperties::FromValues({ { 1, 2, 20 } }));
		auto entry = state::MosaicEntry(id, definition);
		mosaicCacheDelta.insert(entry);
		
		auto& levyCacheDelta = cache.sub<cache::LevyCache>();
		auto levyEntry = CreateLevyEntry(id, levy, true, false);
		levyCacheDelta.insert(levyEntry);
	}
		
	void AssertLevy(const state::LevyEntryData& rhs, const state::LevyEntryData& lsh ) {
		EXPECT_EQ(rhs.Type, lsh.Type);
		EXPECT_EQ(rhs.Recipient, lsh.Recipient);
		EXPECT_EQ(rhs.MosaicId, lsh.MosaicId);
		EXPECT_EQ(rhs.Fee, lsh.Fee);
	}
		
	void AssertLevy(const state::LevyEntryData& rhs, const model::MosaicLevyRaw& raw, model::ResolverContext resolver) {
		EXPECT_EQ(rhs.Type, raw.Type);
		EXPECT_EQ(rhs.Recipient, resolver.resolve(raw.Recipient));
		EXPECT_EQ(rhs.MosaicId, resolver.resolve(raw.MosaicId));
		EXPECT_EQ(rhs.Fee, raw.Fee);
	}
	
	state::LevyEntry& GetLevyEntryFromCache(cache::CatapultCacheDelta& cache, const MosaicId& mosaicId) {
		auto& levyCache = cache.sub<cache::LevyCache>();
		auto iter = levyCache.find(mosaicId);
		return iter.get();
	}
}}
