/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "StreamingPlugin.h"
#include "src/observers/Observers.h"
#include "src/plugins/StreamStartTransactionPlugin.h"
#include "src/plugins/StreamFinishTransactionPlugin.h"
#include "src/plugins/StreamPaymentTransactionPlugin.h"
#include "src/plugins/UpdateDriveSizeTransactionPlugin.h"
#include "catapult/plugins/CacheHandlers.h"

namespace catapult { namespace plugins {

	namespace {
		template<typename TUnresolvedData>
		const TUnresolvedData* castToUnresolvedData(const UnresolvedAmountData* pData) {
			if (!pData)
				CATAPULT_THROW_RUNTIME_ERROR("unresolved amount data pointer is null")

			auto pCast = dynamic_cast<const TUnresolvedData*>(pData);
			if (!pCast)
				CATAPULT_THROW_RUNTIME_ERROR("unresolved amount data pointer is of unexpected type")

			return pCast;
		}
	}

	void RegisterStorageSubsystem(PluginManager& manager) {
		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::StreamingConfiguration>();
		});

		const auto& pConfigHolder = manager.configHolder();
		const auto& immutableConfig = manager.immutableConfig();
		manager.addTransactionSupport(CreateStreamStartTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateStreamFinishTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateStreamPaymentTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateUpdateDriveSizeTransactionPlugin(immutableConfig));

		manager.addStatefulValidatorHook([pConfigHolder, &immutableConfig](auto& builder) {
			builder
					.add(validators::CreateStreamStartFolderNameValidator());
		});
	}
}}


extern "C" PLUGIN_API
		void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterStorageSubsystem(manager);
}
