/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <plugins/txes/config/src/model/CatapultConfigEntityType.h>
#include "Validators.h"
#include "catapult/config/SupportedEntityVersions.h"
#include "catapult/model/BlockChainConfiguration.h"
#include "catapult/plugins/PluginManager.h"
#include "catapult/validators/ValidatorContext.h"
#include "src/cache/CatapultConfigCache.h"
#include "src/config/CatapultConfigConfiguration.h"

namespace catapult { namespace validators {

	using Notification = model::CatapultConfigNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(CatapultConfig, Notification)(plugins::PluginManager& pluginManager) {
		return MAKE_STATEFUL_VALIDATOR(CatapultConfig, ([&pluginManager](const Notification& notification, const ValidatorContext& context) {
			const auto& pluginConfig = pluginManager.config(context.Height).GetPluginConfiguration<config::CatapultConfigConfiguration>("catapult.plugins.config");
			if (notification.BlockChainConfigSize > pluginConfig.MaxBlockChainConfigSize.bytes32())
				return Failure_CatapultConfig_BlockChain_Config_Too_Large;

			if (notification.SupportedEntityVersionsSize > pluginConfig.MaxSupportedEntityVersionsSize.bytes32())
				return Failure_CatapultConfig_SupportedEntityVersions_Config_Too_Large;

			const auto& cache = context.Cache.sub<cache::CatapultConfigCache>();
			if (cache.find(context.Height).tryGet())
				return Failure_CatapultConfig_Config_Redundant;

			auto blockChainConfig = model::BlockChainConfiguration::Uninitialized();
			try {
				std::istringstream configStream{std::string{(const char*)notification.BlockChainConfigPtr, notification.BlockChainConfigSize}};
				auto bag = utils::ConfigurationBag::FromStream(configStream);
				blockChainConfig = model::BlockChainConfiguration::LoadFromBag(bag);
			} catch (...) {
				return Failure_CatapultConfig_BlockChain_Config_Malformed;
			}

			auto pValidator = pluginManager.createStatelessValidator();
			for (const auto& pair : blockChainConfig.Plugins) {
				auto result = pValidator->validate(model::PluginConfigNotification<1>{pair.first, pair.second});
				if (IsValidationResultFailure(result))
					return result;
			}

			try {
				std::istringstream configStream{std::string{(const char*)notification.SupportedEntityVersionsPtr, notification.SupportedEntityVersionsSize}};
				auto supportedEntityVersions = config::LoadSupportedEntityVersions(configStream);
				if (!supportedEntityVersions[model::Entity_Type_Catapult_Config].size())
					return Failure_CatapultConfig_Catapult_Config_Trx_Cannot_Be_Unsupported;
			} catch (...) {
				return Failure_CatapultConfig_SupportedEntityVersions_Config_Malformed;
			}

			return ValidationResult::Success;
		}));
	}
}}
