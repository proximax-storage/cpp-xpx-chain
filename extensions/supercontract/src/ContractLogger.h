/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include <logging/ExternalLogger.h>

namespace catapult::contract {

class ContractLogger: public sirius::logging::ExternalLogger {
public:
	void trace(const std::string& msg) override;
	void debug(const std::string& msg) override;
	void info(const std::string& msg) override;
	void warn(const std::string& msg) override;
	void err(const std::string& msg) override;
	void critical(const std::string& msg) override;

	void stop() override;
};

}