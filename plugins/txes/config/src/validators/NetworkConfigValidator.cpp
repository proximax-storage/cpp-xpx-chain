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
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace validators {

	namespace {
		template<typename TNotification>
		auto ValidateUpgrade(const plugins::PluginManager& pluginManager) {
			return [&pluginManager](const TNotification& notification, const ValidatorContext& context){
				if constexpr(std::is_same_v<TNotification, model::NetworkConfigNotification<1>>) {
					if (BlockDuration(0) == notification.ApplyHeightDelta)
						return Failure_NetworkConfig_ApplyHeightDelta_Zero;
				}
				else {
					if(notification.UpdateType == model::NetworkUpdateType::Delta && notification.ApplyHeight == 0)
						return Failure_NetworkConfig_ApplyHeightDelta_Zero;
				}
				const auto& currentNetworkConfig = context.Config.Network;
				const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::NetworkConfigConfiguration>();

				if (notification.BlockChainConfigSize > pluginConfig.MaxBlockChainConfigSize.bytes32())
					return Failure_NetworkConfig_BlockChain_Config_Too_Large;

				if (notification.SupportedEntityVersionsSize > pluginConfig.MaxSupportedEntityVersionsSize.bytes32())
					return Failure_NetworkConfig_SupportedEntityVersions_Config_Too_Large;

				const auto& cache = context.Cache.sub<cache::NetworkConfigCache>();
				Height height;
				if constexpr(std::is_same_v<TNotification, model::NetworkConfigNotification<1>>)
					height = Height{context.Height.unwrap() + notification.ApplyHeightDelta.unwrap()};
				else {
					if(notification.UpdateType == model::NetworkUpdateType::Delta)
						height = Height{context.Height.unwrap() + notification.ApplyHeight};
					else {
						if(context.Height.unwrap() >= notification.ApplyHeight)
							return Failure_NetworkConfig_ApplyHeight_In_The_Past;
						height = Height(notification.ApplyHeight);
					}
				}
				auto networkConfigIter = cache.find(height);
				if (networkConfigIter.tryGet())
					return Failure_NetworkConfig_Config_Redundant;

				auto networkConfig = model::NetworkConfiguration::Uninitialized();
				try {
					std::istringstream configStream{std::string{(const char*)notification.BlockChainConfigPtr, notification.BlockChainConfigSize}};
					auto bag = utils::ConfigurationBag::FromStream(configStream);
					networkConfig = model::NetworkConfiguration::LoadFromBag(bag, context.Config.Immutable);

					if (context.Config.Immutable.NetworkIdentifier == model::NetworkIdentifier::Public &&
						networkConfig.BlockGenerationTargetTime.millis() == 0)
						return Failure_NetworkConfig_Block_Generation_Time_Zero_Public;

					if (2 * networkConfig.ImportanceGrouping <= networkConfig.MaxRollbackBlocks)
						return Failure_NetworkConfig_ImportanceGrouping_Less_Or_Equal_Half_MaxRollbackBlocks;

					if( networkConfig.AccountVersion < currentNetworkConfig.AccountVersion)
						return Failure_NetworkConfig_AccountVersion_Less_Than_Current;
					//TODO: Reconsider this definition to move to other plugin
					if(networkConfig.DockStakeRewardInterval == BlockDuration(0)) {
						return Failure_NetworkConfig_Interval_Must_Not_Be_Zero;
					}

					if(currentNetworkConfig.DockStakeRewardInterval != networkConfig.DockStakeRewardInterval && (height.unwrap()-1) % currentNetworkConfig.DockStakeRewardInterval.unwrap() != 0)
						return Failure_NetworkConfig_Interval_Must_Change_After_Reward_Tier;

					if(networkConfig.AccountVersion < networkConfig.MinimumAccountVersion)
						return Failure_NetworkConfig_AccountVersion_Less_Than_Minimum;

					if(networkConfig.MinimumAccountVersion < currentNetworkConfig.MinimumAccountVersion)
						return Failure_NetworkConfig_MinimumAccountVersion_Less_Than_Current;
					if (100u < networkConfig.HarvestBeneficiaryPercentage)
						return Failure_NetworkConfig_HarvestBeneficiaryPercentage_Exceeds_One_Hundred;
					auto totalInflationUpToConfigActivation = pluginManager.configHolder()->InflationCalculator().getCumulativeAmount(height);
					auto totalCurrency = context.Config.Immutable.InitialCurrencyAtomicUnits + totalInflationUpToConfigActivation;
					/// totalCurrency at height of validation must not be higher than the new maxmosaicatomicunits
					if (totalCurrency > networkConfig.MaxMosaicAtomicUnits)
						return Failure_NetworkConfig_MaxMosaicAtomicUnits_Invalid;
					if constexpr(std::is_same_v<TNotification, model::NetworkConfigNotification<2>>)
					{
						if(networkConfig.MaxCurrencyMosaicAtomicUnits > networkConfig.MaxMosaicAtomicUnits
							|| networkConfig.MaxCurrencyMosaicAtomicUnits < currentNetworkConfig.MaxCurrencyMosaicAtomicUnits
							|| networkConfig.MaxMosaicAtomicUnits < currentNetworkConfig.MaxMosaicAtomicUnits) {
							return Failure_NetworkConfig_MaxMosaicAtomicUnits_Invalid;
						}

					}
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
			};
		}
	}
	DECLARE_STATEFUL_VALIDATOR(NetworkConfig, model::NetworkConfigNotification<1>)(const plugins::PluginManager& pluginManager) {
		return MAKE_STATEFUL_VALIDATOR_WITH_TYPE(NetworkConfig, model::NetworkConfigNotification<1>, ValidateUpgrade<model::NetworkConfigNotification<1>>(pluginManager));
	}
	DECLARE_STATEFUL_VALIDATOR(NetworkConfigV2, model::NetworkConfigNotification<2>)(const plugins::PluginManager& pluginManager) {
		return MAKE_STATEFUL_VALIDATOR_WITH_TYPE(NetworkConfigV2, model::NetworkConfigNotification<2>, ValidateUpgrade<model::NetworkConfigNotification<2>>(pluginManager));
	}
}}
