/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "src/catapult/validators/ValidationResult.h"
#include "src/config/MosaicConfiguration.h"
#include "src/cache/LevyCache.h"
#include "src/model/MosaicLevy.h"
#include "catapult/validators/ValidatorContext.h"
#include "src/state/LevyEntry.h"

namespace catapult {
	namespace  utils {
		/// compute if levy fee is valid
		validators::ValidationResult CheckLevyOperationAllowed(const Key& signer, MosaicId id, const validators::ValidatorContext& context);
		bool IsMosaicLevyFeeValid(const model::MosaicLevyRaw &levy);
		bool IsMosaicIdValid(MosaicId id,  const validators::ValidatorContext& context);
		bool IsAddressValid(catapult::UnresolvedAddress address, const validators::ValidatorContext& context);
		
		template<typename TContext>
		const std::shared_ptr<state::LevyEntryData> GetLevy(const MosaicId& mosaicId, const TContext& context) {
			auto &pluginConfig = context.Config.Network.template GetPluginConfiguration<config::MosaicConfiguration>();
			if (!pluginConfig.LevyEnabled)
				return nullptr;
			
			auto& levyCache = context.Cache.template sub<cache::LevyCache>();
			auto iter = levyCache.find(mosaicId);
			if(!iter.tryGet())
				return nullptr;
			
			return iter.get().levy();
		}
	}
}