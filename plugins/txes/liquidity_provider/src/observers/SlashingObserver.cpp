/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/state/LiquidityProviderEntry.h"
#include "src/utils/MathUtils.h"

#include <boost/multiprecision/cpp_int.hpp>
#include <src/cache/LiquidityProviderCache.h>
#include <random>

namespace catapult { namespace observers {

	using Notification = model::BlockNotification<1>;
	using BigUint = boost::multiprecision::uint256_t;

	DECLARE_OBSERVER(Slashing, Notification)(const std::shared_ptr<cache::LiquidityProviderKeyCollector>& pKeyCollector) {
		return MAKE_OBSERVER(Slashing, Notification, ([pKeyCollector] (const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (Slashing)");

			auto& liquidityProviderCache = context.Cache.sub<cache::LiquidityProviderCache>();

			for (const auto& mosaicId: pKeyCollector->keys()) {

				auto entryIter = liquidityProviderCache.find(mosaicId);
				auto& entry = entryIter.get();

				if ((context.Height - entry.creationHeight()).unwrap() % entry.slashingPeriod() == 0) {
					auto& turnoverHistory = entry.turnoverHistory();
					turnoverHistory.push_back(entry.recentTurnover());
					if (turnoverHistory.size() > entry.windowSize()) {
						turnoverHistory.pop_front();
					}

					auto it = std::max_element(
							turnoverHistory.begin(), turnoverHistory.end(), [](const auto& a, const auto& b) {
								return a.m_turnover < b.m_turnover;
							});

					const auto& bestRate = it->m_rate;

					const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;
					Key providerKey = entry.providerKey();
					auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();

					auto lpAccountEntryIter = accountStateCache.find(providerKey);
					auto& lpAccountEntry = lpAccountEntryIter.get();

					Amount lpCurrencyBalance = lpAccountEntry.Balances.get(currencyMosaicId);

					auto resolvedMosaicId = context.Resolvers.resolve(entry.mosaicId());
					Amount lpMosaicBalance = lpAccountEntry.Balances.get(resolvedMosaicId);

					Amount totalMosaicsMinted = lpMosaicBalance + entry.additionallyMinted();
					auto currentRate = state::ExchangeRate { lpCurrencyBalance, totalMosaicsMinted };

					constexpr uint64_t maxSteps = 256;
					std::bitset<maxSteps> randomMoves;

					Hash256 eventHash;
					crypto::Sha3_256_Builder sha3;
					const std::string salt = "LiquidityProvider";
					sha3.update({ utils::RawBuffer { reinterpret_cast<const uint8_t*>(&context.Height),
													 sizeof(context.Height) },
								  utils::RawBuffer(reinterpret_cast<const uint8_t*>(salt.data()), salt.size()),
								  utils::RawBuffer(reinterpret_cast<const uint8_t*>(&mosaicId), sizeof(mosaicId)),
								  context.Config.Immutable.GenerationHash });
					sha3.final(eventHash);

					std::seed_seq hashSeed(eventHash.begin(), eventHash.end());
					std::mt19937 rng(hashSeed);

					std::generate_n(reinterpret_cast<uint8_t *>(&randomMoves), sizeof(randomMoves), rng);

					// We are able only to decrease the exchange rate
					if (bestRate < currentRate) {
						BigUint currency = bestRate.m_currencyAmount.unwrap();
						auto left = ((currency * totalMosaicsMinted.unwrap()) / bestRate.m_mosaicAmount.unwrap()).convert_to<uint64_t>();

						// Right bound is unreachable
						uint64_t right = lpCurrencyBalance.unwrap() + 1;

						for (int i = 0; left + 1 < right && i < maxSteps; i++) {
							uint64_t leftSqrt = utils::sqrt(left);
							uint64_t rightSqrt = utils::sqrt(right);

							BigUint leftBig = left;
							BigUint rightBig = right;

							BigUint mBig = (rightBig * leftSqrt + leftBig * rightSqrt) / (leftSqrt + rightSqrt);

							auto m = mBig.convert_to<uint64_t>();

							// Due to flooring in sqrt and division m can occur to be less than left
							m = std::max(m, left);

							if (randomMoves[i] == 0) {
								left = m;
							} else {
								right = m;
							}
						}

						Amount currencyAfterSlashing = Amount { left };
						Amount slashAmount = lpCurrencyBalance - currencyAfterSlashing;

						auto slashingEntryIter = accountStateCache.find(entry.slashingAccount());
						auto& slashingEntry = slashingEntryIter.get();

						slashingEntry.Balances.credit(currencyMosaicId, slashAmount, context.Height);
						lpAccountEntry.Balances.debit(currencyMosaicId, slashAmount, context.Height);
					}

					state::ExchangeRate finalRate = { lpAccountEntry.Balances.get(currencyMosaicId),
													  totalMosaicsMinted };

					entry.recentTurnover() = { finalRate, Amount(0) };
				}
			}
		}))
	};
}}