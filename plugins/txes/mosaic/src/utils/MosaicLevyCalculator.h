/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/model/MosaicLevy.h"
#include <functional>

namespace catapult { namespace utils {

	/// result after levy was computed
	struct MosaicLevyCalcResult
	{
		Amount finalAmount;
		Amount levyAmount;

		MosaicLevyCalcResult(Amount amount, Amount levy)
			: finalAmount(amount)
			, levyAmount(levy) {
		}
	};

	/// A factory for creating mosaic levy calculator.
	class MosaicLevyCalculatorFactory {
	public:
		using MosaicLevyCalculator = std::function<MosaicLevyCalcResult (Amount amount, Amount levy)>;

	public:
		/// Creates a mosaic levy rule factory.
		MosaicLevyCalculatorFactory();

		/// Creates a mosaic levy calculator around a type
		MosaicLevyCalculator getCalculator(model::LevyType type);

	private:
		std::vector<MosaicLevyCalculator> m_calculators;
	};
}}