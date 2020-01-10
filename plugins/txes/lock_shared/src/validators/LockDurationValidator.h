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
#include "catapult/validators/ValidationResult.h"

namespace catapult { namespace validators {

/// Defines a lock duration validator with name \a VALIDATOR_NAME for notification \a NOTIFICATION_TYPE
/// that returns \a FAILURE_RESULT on failure.
#define DEFINE_LOCK_DURATION_VALIDATOR(NAME, FAILURE_RESULT, PLUGIN_NAME) \
	DECLARE_STATEFUL_VALIDATOR(NAME##Duration, model::NAME##DurationNotification<1>)() { \
		using ValidatorType = stateful::FunctionalNotificationValidatorT<model::NAME##DurationNotification<1>>; \
		return std::make_unique<ValidatorType>(#NAME "DurationValidator", [](const auto& notification, const auto& context) { \
			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::NAME##Configuration>(); \
			auto maxDuration = pluginConfig.Max##NAME##Duration.blocks(context.Config.Network.BlockGenerationTargetTime); \
			return BlockDuration(0) != notification.Duration && notification.Duration <= maxDuration \
					? ValidationResult::Success \
					: FAILURE_RESULT; \
		}); \
	}
}}
