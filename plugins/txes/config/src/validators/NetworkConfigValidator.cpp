/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "catapult/validators/ValidatorContext.h"
#include "src/cache/NetworkConfigCache.h"
#include "src/config/NetworkConfigConfiguration.h"
#include "src/model/NetworkConfigEntityType.h"

namespace catapult { namespace validators {

	using Notification = model::NetworkConfigNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(NetworkConfig, Notification)(const plugins::PluginManager& pluginManager) {
		return MAKE_STATEFUL_VALIDATOR(NetworkConfig, ([&pluginManager](const Notification& notification, const ValidatorContext& context) {
			if (BlockDuration(0) == notification.ApplyHeightDelta)
				return Failure_NetworkConfig_ApplyHeightDelta_Zero;

			const auto& currentNetworkConfig = context.Config.Network;
			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::NetworkConfigConfiguration>();

			if (notification.BlockChainConfigSize > pluginConfig.MaxBlockChainConfigSize.bytes32())
				return Failure_NetworkConfig_BlockChain_Config_Too_Large;

			if (notification.SupportedEntityVersionsSize > pluginConfig.MaxSupportedEntityVersionsSize.bytes32())
				return Failure_NetworkConfig_SupportedEntityVersions_Config_Too_Large;

			const auto& cache = context.Cache.sub<cache::NetworkConfigCache>();
			auto height = Height{context.Height.unwrap() + notification.ApplyHeightDelta.unwrap()};
			if (cache.find(height).tryGet())
				return Failure_NetworkConfig_Config_Redundant;

			auto networkConfig = model::NetworkConfiguration::Uninitialized();
			try {
				std::istringstream configStream{std::string{(const char*)notification.BlockChainConfigPtr, notification.BlockChainConfigSize}};
				auto bag = utils::ConfigurationBag::FromStream(configStream);
				networkConfig = model::NetworkConfiguration::LoadFromBag(bag);

				if (context.Config.Immutable.NetworkIdentifier == model::NetworkIdentifier::Public &&
					networkConfig.BlockGenerationTargetTime.millis() == 0)
					return Failure_NetworkConfig_Block_Generation_Time_Zero_Public;

				if (2 * networkConfig.ImportanceGrouping <= networkConfig.MaxRollbackBlocks)
					return Failure_NetworkConfig_ImportanceGrouping_Less_Or_Equal_Half_MaxRollbackBlocks;

				if( networkConfig.AccountVersion < currentNetworkConfig.AccountVersion)
					return Failure_NetworkConfig_AccountVersion_Less_Than_Current;

				if(networkConfig.AccountVersion < networkConfig.MinimumAccountVersion)
					return Failure_NetworkConfig_AccountVersion_Less_Than_Minimum;

				if(networkConfig.MinimumAccountVersion < networkConfig.AccountVersion)
					return Failure_NetworkConfig_MinimumAccountVersion_Less_Than_Current;
				if (100u < networkConfig.HarvestBeneficiaryPercentage)
					return Failure_NetworkConfig_HarvestBeneficiaryPercentage_Exceeds_One_Hundred;

				auto totalInflation = context.Config.Inflation.InflationCalculator.sumAll();
				auto totalCurrency = context.Config.Immutable.InitialCurrencyAtomicUnits + totalInflation.first;
				if (totalCurrency > networkConfig.MaxMosaicAtomicUnits)
					return Failure_NetworkConfig_MaxMosaicAtomicUnits_Invalid;
			} catch (...) {
				return Failure_NetworkConfig_BlockChain_Config_Malformed;
			}

			for (const auto& pair : currentNetworkConfig.Plugins) {
				if (!networkConfig.Plugins.count(pair.first))
					return Failure_NetworkConfig_Plugin_Config_Missing;
			}

			auto pValidator = pluginManager.createStatelessValidator();
			for (const auto& pair : networkConfig.Plugins) {
				auto result = pValidator->validate(model::PluginConfigNotification<1>{pair.first, pair.second});
				if (IsValidationResultFailure(result)) {
                    return result;
                }
			}

			try {
				std::istringstream configStream{std::string{(const char*)notification.SupportedEntityVersionsPtr, notification.SupportedEntityVersionsSize}};
				auto supportedEntityVersions = config::LoadSupportedEntityVersions(configStream);
				if (!supportedEntityVersions[model::Entity_Type_Network_Config].size()) {
                    return Failure_NetworkConfig_Network_Config_Trx_Cannot_Be_Unsupported;
                }
			} catch (...) {
				return Failure_NetworkConfig_SupportedEntityVersions_Config_Malformed;
			}

			return ValidationResult::Success;
		}));
	}
}}
