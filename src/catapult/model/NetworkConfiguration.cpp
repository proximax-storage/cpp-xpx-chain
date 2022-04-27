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

#include "NetworkConfiguration.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace model {

	// region NetworkConfiguration

	namespace {
		void CheckPluginName(const std::string& pluginName) {
			if (std::any_of(pluginName.cbegin(), pluginName.cend(), [](auto ch) { return (ch < '0' || ch > '9') && (ch < 'a' || ch > 'z') && '.' != ch && '_' != ch; }))
				CATAPULT_THROW_INVALID_ARGUMENT_1("plugin name contains unexpected character", pluginName);
		}
	}

	NetworkConfiguration NetworkConfiguration::Uninitialized() {
		return NetworkConfiguration();
	}

	NetworkConfiguration NetworkConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		NetworkConfiguration config;

#define LOAD_NETWORK_PROPERTY(NAME) utils::LoadIniProperty(bag, "network", #NAME, config.Info.NAME)

		LOAD_NETWORK_PROPERTY(PublicKey);

#undef LOAD_NETWORK_PROPERTY

#define LOAD_CHAIN_PROPERTY(NAME) utils::LoadIniProperty(bag, "chain", #NAME, config.NAME)

		LOAD_CHAIN_PROPERTY(BlockGenerationTargetTime);
		LOAD_CHAIN_PROPERTY(BlockTimeSmoothingFactor);

		LOAD_CHAIN_PROPERTY(GreedDelta);
		LOAD_CHAIN_PROPERTY(GreedExponent);

		LOAD_CHAIN_PROPERTY(ImportanceGrouping);
		LOAD_CHAIN_PROPERTY(MaxRollbackBlocks);
		LOAD_CHAIN_PROPERTY(MaxDifficultyBlocks);

		LOAD_CHAIN_PROPERTY(MaxTransactionLifetime);
		LOAD_CHAIN_PROPERTY(MaxBlockFutureTime);

		LOAD_CHAIN_PROPERTY(MaxMosaicAtomicUnits);

		LOAD_CHAIN_PROPERTY(TotalChainImportance);
		LOAD_CHAIN_PROPERTY(MinHarvesterBalance);
		LOAD_CHAIN_PROPERTY(HarvestBeneficiaryPercentage);

		LOAD_CHAIN_PROPERTY(BlockPruneInterval);
		LOAD_CHAIN_PROPERTY(MaxTransactionsPerBlock);

#undef LOAD_CHAIN_PROPERTY

#define TRY_LOAD_CHAIN_PROPERTY(NAME) utils::TryLoadIniProperty(bag, "chain", #NAME, config.NAME)

		config.EnableUnconfirmedTransactionMinFeeValidation = true;
		TRY_LOAD_CHAIN_PROPERTY(EnableUnconfirmedTransactionMinFeeValidation);
		config.EnableDeadlineValidation = true;
		TRY_LOAD_CHAIN_PROPERTY(EnableDeadlineValidation);

		config.EnableUndoBlock = true;
		TRY_LOAD_CHAIN_PROPERTY(EnableUndoBlock);
		config.EnableBlockSync = true;
		TRY_LOAD_CHAIN_PROPERTY(EnableBlockSync);

		config.EnableWeightedVoting = false;
		TRY_LOAD_CHAIN_PROPERTY(EnableWeightedVoting);
		config.CommitteeSize = 21;
		TRY_LOAD_CHAIN_PROPERTY(CommitteeSize);
		config.CommitteeApproval = 0.67;
		TRY_LOAD_CHAIN_PROPERTY(CommitteeApproval);
		config.CommitteePhaseTime = utils::TimeSpan::FromSeconds(5);
		TRY_LOAD_CHAIN_PROPERTY(CommitteePhaseTime);
		config.MinCommitteePhaseTime = utils::TimeSpan::FromSeconds(1);
		TRY_LOAD_CHAIN_PROPERTY(MinCommitteePhaseTime);
		config.MaxCommitteePhaseTime = utils::TimeSpan::FromMinutes(1);;
		TRY_LOAD_CHAIN_PROPERTY(MaxCommitteePhaseTime);
		config.CommitteeSilenceInterval = utils::TimeSpan::FromMilliseconds(100);
		TRY_LOAD_CHAIN_PROPERTY(CommitteeSilenceInterval);
		config.CommitteeRequestInterval = utils::TimeSpan::FromMilliseconds(500);
		TRY_LOAD_CHAIN_PROPERTY(CommitteeRequestInterval);
		config.CommitteeChainHeightRequestInterval = utils::TimeSpan::FromSeconds(30);
		TRY_LOAD_CHAIN_PROPERTY(CommitteeChainHeightRequestInterval);
		config.CommitteeTimeAdjustment = 1.1;
		TRY_LOAD_CHAIN_PROPERTY(CommitteeTimeAdjustment);
		config.CommitteeEndSyncApproval = 0.45;
		TRY_LOAD_CHAIN_PROPERTY(CommitteeEndSyncApproval);
		config.CommitteeBaseTotalImportance = 100;
		TRY_LOAD_CHAIN_PROPERTY(CommitteeBaseTotalImportance);
		config.CommitteeNotRunningContribution = 0.5;
		TRY_LOAD_CHAIN_PROPERTY(CommitteeNotRunningContribution);

#undef TRY_LOAD_CHAIN_PROPERTY

		size_t numPluginProperties = 0;
		std::string prefix("plugin:");
		for (const auto& section : bag.sections()) {
			if ("network" == section || "chain" == section)
				continue;

			if (section.size() <= prefix.size() || 0 != section.find(prefix))
				continue;

			auto pluginName = section.substr(prefix.size());
			CheckPluginName(pluginName);
			auto iter = config.Plugins.emplace(pluginName, utils::ExtractSectionAsBag(bag, section.c_str())).first;
			numPluginProperties += iter->second.size();
		}

		return config;
	}

	// endregion

	// region utils

	utils::TimeSpan CalculateFullRollbackDuration(const NetworkConfiguration& config) {
		return utils::TimeSpan::FromMilliseconds(config.BlockGenerationTargetTime.millis() * config.MaxRollbackBlocks);
	}

	utils::TimeSpan CalculateRollbackVariabilityBufferDuration(const NetworkConfiguration& config) {
		// use the greater of 25% of the rollback time or one hour as a buffer against block time variability
		return utils::TimeSpan::FromHours(4).millis() > CalculateFullRollbackDuration(config).millis()
				? utils::TimeSpan::FromHours(1)
				: utils::TimeSpan::FromMilliseconds(CalculateFullRollbackDuration(config).millis() / 4);
	}

	utils::TimeSpan CalculateTransactionCacheDuration(const NetworkConfiguration& config) {
		return utils::TimeSpan::FromMilliseconds(
				CalculateFullRollbackDuration(config).millis()
				+ CalculateRollbackVariabilityBufferDuration(config).millis());
	}

	uint64_t CalculateDifficultyHistorySize(const NetworkConfiguration& config) {
		return config.MaxRollbackBlocks + config.MaxDifficultyBlocks;
	}

	// endregion
}}
