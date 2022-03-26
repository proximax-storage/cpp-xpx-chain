/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/state/LPEntry.h"
#include "src/utils/MathUtils.h"

#include <boost/multiprecision/cpp_int.hpp>

namespace catapult { namespace observers {

	using Notification = model::BlockNotification<2>;
	using BigUint = boost::multiprecision::uint256_t;

	DECLARE_OBSERVER(Slashing, Notification)() {
		return MAKE_OBSERVER(Slashing, Notification, ([](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (Slashing)");

//			auto& queueCache = context.Cache.template sub<cache::QueueCache>();

			MosaicId mosaicId;

			state::LiquidityProviderEntry entry(MosaicId{mosaicId});

			if ((context.Height - entry.creationHeight()).unwrap() % entry.slashingPeriod() == 0) {
				auto& turnoverHistory = entry.turnoverHistory();
				turnoverHistory.push_back(entry.recentTurnover());
				if (turnoverHistory.size() > 5) {
					turnoverHistory.pop_back();
				}

				auto it = std::max_element(turnoverHistory.begin(), turnoverHistory.end(), [] (const auto& a, const auto& b) {
					return a.m_turnover < b.m_turnover;
				});

				const auto& bestRate = it->m_rate;


				const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;
				Key providerKey;
				auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
				auto& lpAccountEntry = accountStateCache.find(providerKey).get();
				Amount lpCurrencyBalance = lpAccountEntry.Balances.get(currencyMosaicId);

				Amount totalMosaicsMinted = entry.initiallyMinted() + entry.additionallyMinted();
				auto currentRate = state::ExchangeRate{lpCurrencyBalance, totalMosaicsMinted};

				constexpr uint64_t maxSteps = 512;
				std::bitset<maxSteps> randomMoves;

				// We are able only to decrease the exchange rate
				if (bestRate < currentRate) {

					uint64_t left = bestRate.computeCurrencyAmount(totalMosaicsMinted).unwrap();

					// Right bound is unreachable
					uint64_t right = lpCurrencyBalance.unwrap() + 1;

					for(int i = 0; left + 1 < right && i < 512; i++) {
						uint64_t leftSqrt = utils::sqrt(left);
						uint64_t rightSqrt = utils::sqrt(right);

						BigUint leftBig = left;
						BigUint rightBig = right;

						BigUint mBig = (rightBig * leftSqrt + leftBig * rightSqrt) / (leftSqrt + rightSqrt);

						auto m = mBig.convert_to<uint64_t>();

						//Due to flooring in sqrt and division m can occur to be less than left
						m = std::max(m, left);

						if (randomMoves[i] == 0) {
							left = m;
						}
						else {
							right = m;
						}
					}

					Amount currencyAfterSlashing = Amount{left};
					Amount slashAmount = lpCurrencyBalance - currencyAfterSlashing;

					auto& slashingEntry = accountStateCache.find(entry.slashingAccount()).get();
					slashingEntry.Balances.credit(currencyMosaicId, slashAmount, context.Height);
					lpAccountEntry.Balances.debit(currencyMosaicId, slashAmount, context.Height);
				}

				state::ExchangeRate finalRate = {lpAccountEntry.Balances.get(currencyMosaicId), totalMosaicsMinted};
				entry.recentTurnover() = {finalRate, Amount(0)};
			}
        }))
	};
}}