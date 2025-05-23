/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <src/catapult/utils/Casting.h>
#include "MosaicLevyCalculator.h"

namespace catapult { namespace utils {

    MosaicLevyCalculatorFactory::MosaicLevyCalculatorFactory() {
        // No Levy
        m_calculators.push_back([](const Amount &, const Amount &) {
            return Amount(0);
        });

        // absolute or constant calculator
        m_calculators.push_back([](const Amount &, const Amount& levy) {
            return levy;
        });

        // percentile fee
        m_calculators.push_back([](const Amount amount, const Amount levy) {
            float pct = (levy.unwrap() / (float) model::MosaicLevyFeeDecimalPlace) / 100.0f;
            return Amount(amount.unwrap() * pct);
        });
    }

    MosaicLevyCalculatorFactory::MosaicLevyCalculator MosaicLevyCalculatorFactory::getCalculator(model::LevyType type) {
        auto id = utils::to_underlying_type(type);
        if (m_calculators.size() <= id)
            CATAPULT_THROW_INVALID_ARGUMENT_1("unknown type with id",static_cast<uint8_t>(type));

        return m_calculators[id];
    }
}}
