/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "NemesisConfiguration.h"
#include <string>

namespace catapult { namespace test {

	void GenerateAndSaveNemesisBlock(const std::string& resourcesPath, const NemesisConfiguration& nemesisConfig);
}}
