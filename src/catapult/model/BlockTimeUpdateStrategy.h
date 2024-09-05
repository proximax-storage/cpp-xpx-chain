/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <string>

namespace catapult { namespace model {

	/// Strategy for updating block time after both successful and failed block confirmation.
	enum class BlockTimeUpdateStrategy {
		/// Block time is not changing.
		None,

		/// Increase block time by CommitteeTimeAdjustment coefficient on failed block confirmation,
		/// and decrease it by the same coefficient on successful block confirmation.
		IncreaseDecrease_Coefficient,

		/// Increase block time by CommitteeTimeAdjustment coefficient on failed block confirmation,
		/// and reset it to the minimum value on successful block confirmation.
		Increase_Coefficient
	};

	/// Tries to parse \a strategyName into a block time update \a strategy.
	bool TryParseValue(const std::string& strategyName, BlockTimeUpdateStrategy& strategy);
}}
