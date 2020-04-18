/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "LevyTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {
	
	model::MosaicLevy CreateValidMosaicLevy()
	{
		auto levy = model::MosaicLevy();
		levy.Type = model::LevyType::Absolute;
		levy.Recipient = test::GenerateRandomByteArray<UnresolvedAddress>();
		levy.MosaicId = MosaicId(1000);
		levy.Fee = test::CreateMosaicLevyFeePercentile(10); // 10% levy fee
		return levy;
	}
	
	Amount CreateMosaicLevyFeePercentile(float percentage) {
		return Amount( percentage * model::MosaicLevyFeeDecimalPlace);
	}
		
	void AddMosaicWithLevy(cache::CatapultCacheDelta& cache, MosaicId id, Height height, model::MosaicLevy levy) {
		auto& mosaicCacheDelta = cache.sub<cache::MosaicCache>();
		auto definition = state::MosaicDefinition(height, Key(), 1, model::MosaicProperties::FromValues({ { 1, 2, 20 } }));
		auto entry = state::MosaicEntry(id, definition);
		mosaicCacheDelta.insert(entry);
		
		auto& levyCacheDelta = cache.sub<cache::LevyCache>();
		auto levyEntry = state::LevyEntry(id, levy);
		levyCacheDelta.insert(levyEntry);
	}
	
	void AddMosaicWithLevy(cache::CatapultCacheDelta& cache, MosaicId id, Height height, model::MosaicLevy levy, const Key& owner) {
		auto& mosaicCacheDelta = cache.sub<cache::MosaicCache>();
		auto definition = state::MosaicDefinition(height, owner, 1, model::MosaicProperties::FromValues({ { 1, 2, 20 } }));
		auto entry = state::MosaicEntry(id, definition);
		mosaicCacheDelta.insert(entry);
		
		auto& levyCacheDelta = cache.sub<cache::LevyCache>();
		auto levyEntry = state::LevyEntry(id, levy);
		levyCacheDelta.insert(levyEntry);
	}
}}
