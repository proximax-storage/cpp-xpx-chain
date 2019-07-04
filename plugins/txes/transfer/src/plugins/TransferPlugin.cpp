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

#include "TransferPlugin.h"
#include "TransferTransactionPlugin.h"
#include "src/config/TransferConfiguration.h"
#include "src/validators/Validators.h"
#include "catapult/plugins/PluginManager.h"

namespace catapult { namespace plugins {

	void RegisterTransferSubsystem(PluginManager& manager) {
		manager.addTransactionSupport(CreateTransferTransactionPlugin());

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateTransferMosaicsValidator())
				.add(validators::CreatePluginConfigValidator());
		});

		const auto& pConfigHolder = manager.configHolder();
		manager.addStatefulValidatorHook([&pConfigHolder](auto& builder) {
			builder.add(validators::CreateTransferMessageValidator(pConfigHolder));
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterTransferSubsystem(manager);
}
