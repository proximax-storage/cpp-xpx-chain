/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ContractLogger.h"
#include <catapult/utils/Logging.h>

namespace catapult::contract {

	void ContractLogger::trace(const std::string& msg) {
		CATAPULT_LOG(trace) << msg;
	}

	void ContractLogger::debug(const std::string& msg) {
		CATAPULT_LOG(debug) << msg;
	}

	void ContractLogger::info(const std::string& msg) {
		CATAPULT_LOG(info) << msg;
	}

	void ContractLogger::warn(const std::string& msg) {
		CATAPULT_LOG(warning) << msg;
	}

	void ContractLogger::err(const std::string& msg) {
		CATAPULT_LOG(error) << msg;
	}

	void ContractLogger::critical(const std::string& msg) {
		CATAPULT_LOG(fatal) << msg;
	}

	void ContractLogger::stop() {}
}