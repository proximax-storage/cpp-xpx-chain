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

	namespace {
		DEFINE_SUPPORTED_TRANSACTION_VERSION_SUPPLIER(Transfer, Transfer, "catapult.plugins.transfer")
	}

	void RegisterTransferSubsystem(PluginManager& manager) {
		const auto& config = manager.config();
		manager.addTransactionSupport(CreateTransferTransactionPlugin(TransferTransactionSupportedVersionSupplier(config)));

		manager.addStatelessValidatorHook([&config](auto& builder) {
			builder.add(validators::CreateTransferMessageValidator(config));
			builder.add(validators::CreateTransferMosaicsValidator());
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterTransferSubsystem(manager);
}
