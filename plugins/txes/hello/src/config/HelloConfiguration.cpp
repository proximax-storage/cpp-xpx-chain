/**
*** FOR TRAINING PURPOSES ONLY
**/
#include "HelloConfiguration.h"
#include "catapult/utils/ConfigurationBag.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace config {
    HelloConfiguration HelloConfiguration::Uninitialized() {
		return HelloConfiguration();
	}

	HelloConfiguration HelloConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		HelloConfiguration config;
		utils::LoadIniProperty(bag, "", "MessageCount", config.messageCount);   //The code inside lowers the first letter ex" messageCount
		utils::VerifyBagSizeLte(bag, PluginConfiguration::CommonPropertyNumber()+1);

		return config;
	}
}}