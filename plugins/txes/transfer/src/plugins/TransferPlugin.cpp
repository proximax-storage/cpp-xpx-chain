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

#include <catapult/model/Address.h>
#include "TransferPlugin.h"
#include "TransferTransactionPlugin.h"
#include "src/config/TransferConfiguration.h"
#include "src/validators/Validators.h"
#include "catapult/plugins/PluginManager.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/config/CatapultDataDirectory.h"
#include "src/observers/Observers.h"

namespace catapult { namespace plugins {

	void RegisterTransferSubsystem(PluginManager& manager) {
		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::TransferConfiguration>();
		});

		manager.addTransactionSupport(CreateTransferTransactionPlugin());

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateTransferPluginConfigValidator());
		});

		manager.addStatefulValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateTransferMessageValidator())
				.add(validators::CreateTransferMosaicsValidator());
		});
		if (!manager.configHolder()->Config().User.EnableDelegatedHarvestersAutoDetection)
			return;


		//auto encryptionPrivateKeyPemFilename = config::GetNodePrivateKeyPemFilename(manager.userConfig().CertificateDirectory);
		//auto encryptionPublicKey = crypto::ReadPublicKeyFromPrivateKeyPemFile(encryptionPrivateKeyPemFilename);
		auto recipient = model::PublicKeyToAddress(crypto::KeyPair::FromString(manager.configHolder()->Config().User.BootKey).publicKey(), manager.configHolder()->Config().Immutable.NetworkIdentifier);
		auto dataDirectory = config::CatapultDataDirectory(manager.configHolder()->Config().User.DataDirectory);
		manager.addObserverHook([recipient, dataDirectory](auto& builder) {
		  builder.add(observers::CreateTransferMessageV2Observer(0xE201735761802AFE, recipient, dataDirectory.dir("transfer_message")));
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterTransferSubsystem(manager);
}
