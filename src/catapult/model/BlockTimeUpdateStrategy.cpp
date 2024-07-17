/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "BlockTimeUpdateStrategy.h"
#include "catapult/utils/ConfigurationValueParsers.h"

namespace catapult { namespace model {

	namespace {
		const std::array<std::pair<const char*, BlockTimeUpdateStrategy>, 3> String_To_Block_Time_Update_Strategy_Pairs{{
			{ "none", BlockTimeUpdateStrategy::None },
			{ "increase-decrease-coefficient", BlockTimeUpdateStrategy::IncreaseDecrease_Coefficient },
			{ "increase-coefficient", BlockTimeUpdateStrategy::Increase_Coefficient }
		}};
	}

	bool TryParseValue(const std::string& strategyName, BlockTimeUpdateStrategy& strategy) {
		return utils::TryParseEnumValue(String_To_Block_Time_Update_Strategy_Pairs, strategyName, strategy);
	}
}}
