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

#include "SynchronizationFilters.h"
#include "filter_constants.h"
#include "catapult/utils/ContainerHelpers.h"
#include <algorithm>
#include <cmath>

namespace catapult { namespace timesync { namespace filters {

	namespace {
		int64_t ToInt64(const utils::TimeSpan& timeSpan) {
			return static_cast<int64_t>(timeSpan.millis());
		}
	}

	SynchronizationFilter CreateClampingFilter() {
		return [](const auto& sample, auto nodeAge) {
			auto ageToUse = std::max<int64_t>(nodeAge.unwrap() - Start_Decay_After_Round, 0);
			auto toleratedDeviation = std::max(
					static_cast<int64_t>(std::exp(-Decay_Strength * ageToUse) * ToInt64(Tolerated_Deviation_Start)),
					ToInt64(Tolerated_Deviation_Minimum));
			auto timeOffsetToRemote = sample.timeOffsetToRemote();
			return timeOffsetToRemote > toleratedDeviation || -toleratedDeviation > timeOffsetToRemote;
		};
	}
}}}
