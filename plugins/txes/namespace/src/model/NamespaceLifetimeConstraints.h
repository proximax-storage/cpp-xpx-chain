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
#include "NamespaceConstants.h"
#include "catapult/model/BlockChainConfiguration.h"
#include "catapult/plugins/PluginUtils.h"
#include "src/config/NamespaceConfiguration.h"

namespace catapult { namespace model {

	/// Constraints for a namespace's lifetime.
	struct NamespaceLifetimeConstraints {
	public:
		/// Creates constraints around \a maxDuration and \a gracePeriodDuration.
		constexpr NamespaceLifetimeConstraints(const model::BlockChainConfiguration& blockChainConfig)
				: m_blockChainConfig(blockChainConfig)
		{}

	public:
		/// Maximum lifetime a namespace may have including the grace period.
		BlockDuration maxNamespaceDuration() {
			const auto& pluginConfig = m_blockChainConfig.GetPluginConfiguration<config::NamespaceConfiguration>(PLUGIN_NAME(namespace));
			auto gracePeriodDuration = pluginConfig.NamespaceGracePeriodDuration.blocks(m_blockChainConfig.BlockGenerationTargetTime);
			auto maxDuration = pluginConfig.MaxNamespaceDuration.blocks(m_blockChainConfig.BlockGenerationTargetTime);
			return BlockDuration{maxDuration.unwrap() + gracePeriodDuration.unwrap()};
		}

	private:
		const model::BlockChainConfiguration& m_blockChainConfig;
	};
}}
