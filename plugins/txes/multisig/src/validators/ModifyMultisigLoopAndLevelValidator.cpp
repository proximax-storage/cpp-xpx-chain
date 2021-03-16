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

#include "Validators.h"
#include "src/cache/MultisigCache.h"
#include "src/cache/MultisigCacheUtils.h"
#include "src/config/MultisigConfiguration.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::ModifyMultisigNewCosignerNotification<1>;

	namespace {
		class LoopAndLevelChecker {
		public:
			explicit LoopAndLevelChecker(const cache::MultisigCache::CacheReadOnlyType& multisigCache, uint8_t maxMultisigDepth)
					: m_multisigCache(multisigCache)
					, m_maxMultisigDepth(maxMultisigDepth)
			{}

		public:
			ValidationResult validate(const Key& topKey, const Key& bottomKey) {
				utils::KeySet ancestorKeys;
				ancestorKeys.insert(topKey);
				auto numTopLevels = FindAncestors(m_multisigCache, topKey, ancestorKeys);

				utils::KeySet descendantKeys;
				descendantKeys.insert(bottomKey);
				auto numBottomLevels = FindDescendants(m_multisigCache, bottomKey, descendantKeys);

				if (numTopLevels + numBottomLevels + 1 > m_maxMultisigDepth)
					return Failure_Multisig_Modify_Max_Multisig_Depth;

				auto hasLoop = std::any_of(ancestorKeys.cbegin(), ancestorKeys.cend(), [&descendantKeys](const auto& key) {
					return descendantKeys.cend() != descendantKeys.find(key);
				});
				return hasLoop ? Failure_Multisig_Modify_Loop : ValidationResult::Success;
			}

		private:
			const cache::MultisigCache::CacheReadOnlyType& m_multisigCache;
			uint8_t m_maxMultisigDepth;
		};
	}

	DECLARE_STATEFUL_VALIDATOR(ModifyMultisigLoopAndLevel, Notification)() {
		return MAKE_STATEFUL_VALIDATOR(ModifyMultisigLoopAndLevel, [](
					const auto& notification,
					const ValidatorContext& context) {
			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::MultisigConfiguration>();
			auto checker = LoopAndLevelChecker(context.Cache.sub<cache::MultisigCache>(), pluginConfig.MaxMultisigDepth);
			return checker.validate(notification.MultisigAccountKey, notification.CosignatoryKey);
		});
	}
}}
