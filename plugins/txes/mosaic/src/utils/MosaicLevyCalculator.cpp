#include <src/catapult/utils/Casting.h>
#include "MosaicLevyCalculator.h"

namespace catapult { namespace utils {

	MosaicLevyCalculatorFactory::MosaicLevyCalculatorFactory() {
			// No Levy
			m_calculators.push_back([](Amount amount, Amount) {
				return MosaicLevyCalcResult(amount, Amount(0));
			});

			// absolute or constant calculator
			m_calculators.push_back([](Amount amount, Amount levy) {
				Amount finalAmount = amount - levy;
				return MosaicLevyCalcResult(finalAmount.unwrap() ? finalAmount : Amount(0), levy);
			});

			// percentile fee
			m_calculators.push_back([](Amount amount, Amount levy) {
				float pct = (levy.unwrap() / (float) model::MosaicLevyFeeDecimalPlace) / 100.0f;
				Amount tax = Amount(amount.unwrap() * pct);
				Amount finalAmount = amount - tax;
				
				return MosaicLevyCalcResult(finalAmount.unwrap() ? finalAmount : Amount(0), tax);
			});
		}

		MosaicLevyCalculatorFactory::MosaicLevyCalculator MosaicLevyCalculatorFactory::getCalculator(model::LevyType type) {
			auto id = utils::to_underlying_type(type);
			if (m_calculators.size() <= id)
				CATAPULT_THROW_INVALID_ARGUMENT_1("unknown type with id", static_cast<uint16_t>(type));

			return m_calculators[id];
		}
	}
}