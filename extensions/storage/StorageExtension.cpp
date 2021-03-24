/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/BlockStorageSubscription.h"
#include "catapult/extensions/ProcessBootstrapper.h"

namespace catapult { namespace storage {

	namespace {
		void RegisterExtension(extensions::ProcessBootstrapper& bootstrapper) {
			auto& subscriptionManager = bootstrapper.subscriptionManager();
			validators::stateful::DemuxValidatorBuilder builder;
			// Here we can add observers(it is stateful validators because they can't modify cache)
			// Example of declaration you can find in DEFINE_STATEFUL_VALIDATOR
			// To add a new handler use builder.add();
			subscriptionManager.addPostBlockCommitSubscriber(CreateBlockStorageSubscription(
					bootstrapper,
					builder.build([](auto) {
					return false;
			})));
		}
	}
}}

extern "C" PLUGIN_API
void RegisterExtension(catapult::extensions::ProcessBootstrapper& bootstrapper) {
	catapult::storage::RegisterExtension(bootstrapper);
}
