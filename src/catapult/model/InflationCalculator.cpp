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

#include "InflationCalculator.h"
#include "catapult/exceptions.h"

namespace catapult { namespace model {

	size_t InflationCalculator::size() const {
		return m_inflationMap.size();
	}

	bool InflationCalculator::contains(Height height, Amount amount) const {
		if(m_dirty)
			CATAPULT_THROW_RUNTIME_ERROR("attempt to modify dirty inflation calculator");
		auto iter = m_inflationMap.find(height);
		return m_inflationMap.cend() != iter && iter->second.Inflation == amount;
	}

	Amount InflationCalculator::getSpotAmount(Height height) const {
		if(m_dirty)
			CATAPULT_THROW_RUNTIME_ERROR("attempt to modify dirty inflation calculator");
		auto amount = Amount();
		std::pair<Height, InflationPair> correspondingPair;
		/// Find the interval in which this inflation reward belongs.
		for (const auto& pair : m_inflationMap) {
			if (height < pair.first)
				break;
			correspondingPair = pair;
		}
		auto expiryInfo = m_intervalMetadata.find(correspondingPair.first)->second;
		/// If inflation is non zero(optimization), check whether more currency can be created.
		if(correspondingPair.second.Inflation != Amount(0) && height >= expiryInfo.Height) {
			return height == expiryInfo.Height ? expiryInfo.Inflation : Amount(0);
		}
		return amount;
	}

	Amount InflationCalculator::sumAll() const {
		if(m_dirty)
			CATAPULT_THROW_RUNTIME_ERROR("attempt to modify dirty inflation calculator");
		Amount totalAmountRaw = Amount(0);
		for (const auto& pair : m_intervalMetadata) {
			totalAmountRaw = totalAmountRaw + pair.second.TotalInflationWithinInterval;
		}
		return totalAmountRaw;
	}
	namespace {
		Amount calculateTotalInflationAmount(uint64_t duration, Amount inflationValue) {
			return Amount(inflationValue.unwrap()*duration);
		}

		IntervalMetadata calculateExpiryPairForInterval(Height startHeight, InflationPair inflationValue, Amount totalUsedInflationSupply, Amount initialCurrencyAtomicUnits) {
			auto remainingSupply = inflationValue.MaxMosaicSupply - totalUsedInflationSupply - initialCurrencyAtomicUnits;
			auto remainingBlocks = remainingSupply.unwrap() / inflationValue.Inflation.unwrap();
			auto remainderInflation = remainingSupply.unwrap() % inflationValue.Inflation.unwrap();
			return IntervalMetadata {Amount(remainderInflation), startHeight + Height(remainingBlocks) - Height(1)  + (remainderInflation > 0 ? Height(1) : Height(0)), totalUsedInflationSupply};
		}
	}

	void InflationCalculator::init(Amount initialCurrencyAtomicUnits) {
		m_initialCurrencyAtomicUnits = initialCurrencyAtomicUnits;
		m_dirty = false;
	}

	Amount InflationCalculator::getCumulativeAmount(Height height) const {
		if(m_dirty)
			CATAPULT_THROW_RUNTIME_ERROR("attempt to modify dirty inflation calculator");
		if (Height() == height)
			return Amount();

		/// Calculate available inflation supply
		Amount totalUsedInflationSupply = Amount(0);

		/// Clear expiry map for intervals as it's going to be rebuilt.
		auto activeInflationIterator = m_inflationMap.cbegin();
		std::pair<Height, InflationPair> currentActiveInflationAmount = *activeInflationIterator;
		auto nextActiveInflationIterator = ++activeInflationIterator;
		while(true) {
			/// Figure out when the next change will happen
			auto nextInflationChange = Height();
			if(nextActiveInflationIterator != m_inflationMap.cend() && nextActiveInflationIterator->first <= height) nextInflationChange = activeInflationIterator->first;
			auto metadataForThisInterval = m_intervalMetadata.find(currentActiveInflationAmount.first);

			/// What we need is in this interval
			if(nextInflationChange == Height()) {
				/// If height is lesser or equal to expiry height
				if(height <= metadataForThisInterval->second.Height) {
					auto numBlocks = height.unwrap() - currentActiveInflationAmount.first.unwrap();
					totalUsedInflationSupply = totalUsedInflationSupply + Amount (numBlocks*currentActiveInflationAmount.second.Inflation.unwrap());
				} else {
					/// Includes all inflation for the interval as it includes the expiry block.
					totalUsedInflationSupply = totalUsedInflationSupply + metadataForThisInterval->second.TotalInflationWithinInterval;
				}
				break;
			} else { /// What we need is not in this interval yet
				totalUsedInflationSupply = totalUsedInflationSupply + metadataForThisInterval->second.TotalInflationWithinInterval;
			}
		}
		return totalUsedInflationSupply;
	}
	void InflationCalculator::updateExpiryData() {
		/// Calculate available inflation supply
		Amount totalUsedInflationSupply = Amount(0);

		/// Clear expiry map for intervals as it's going to be rebuilt.
		m_intervalMetadata.clear();
		auto activeInflationIterator = m_inflationMap.cbegin();
		std::pair<Height, InflationPair> currentActiveInflationAmount = *activeInflationIterator;
		auto nextActiveInflationIterator = ++activeInflationIterator;
		while(true) {
			/// Figure out when the next change will happen
			auto nextInflationChange = Height();
			if(nextActiveInflationIterator != m_inflationMap.cend()) nextInflationChange = nextActiveInflationIterator->first;

			/// We determine the interval we need to calculate
			if(nextInflationChange == Height()) {
				/// This is an open ended interval
				m_intervalMetadata[currentActiveInflationAmount.first] = calculateExpiryPairForInterval(currentActiveInflationAmount.first, currentActiveInflationAmount.second, totalUsedInflationSupply, m_initialCurrencyAtomicUnits);
				m_intervalMetadata[currentActiveInflationAmount.first].TotalInflationWithinInterval = currentActiveInflationAmount.second.MaxMosaicSupply - m_initialCurrencyAtomicUnits - totalUsedInflationSupply;
				break;
			}
			else {
				/// The next supply change
				uint64_t duration = nextInflationChange.unwrap() - currentActiveInflationAmount.first.unwrap();
				auto inflationForThisInterval = calculateTotalInflationAmount(duration, currentActiveInflationAmount.second.Inflation);
				/// We must verify that the total inflation doesn't outweight the maximum supply.
				if(inflationForThisInterval + m_initialCurrencyAtomicUnits + totalUsedInflationSupply > currentActiveInflationAmount.second.MaxMosaicSupply) {
					/// In this scenario this interval has reached the maximum inflation reward.
					inflationForThisInterval = currentActiveInflationAmount.second.MaxMosaicSupply - m_initialCurrencyAtomicUnits - totalUsedInflationSupply;
					/// We record the last reward and the height of it for this interval.
					m_intervalMetadata[currentActiveInflationAmount.first] = calculateExpiryPairForInterval(currentActiveInflationAmount.first, currentActiveInflationAmount.second, totalUsedInflationSupply, m_initialCurrencyAtomicUnits);
					m_intervalMetadata[currentActiveInflationAmount.first].TotalInflationWithinInterval = inflationForThisInterval;
				} else {
					m_intervalMetadata[currentActiveInflationAmount.first] = IntervalMetadata {Amount(0), nextInflationChange, totalUsedInflationSupply, inflationForThisInterval};
				}
				totalUsedInflationSupply = totalUsedInflationSupply + inflationForThisInterval;
				currentActiveInflationAmount = *nextActiveInflationIterator++;
			}
		}
	}
	void InflationCalculator::add(Height height, Amount amount, Amount maxMosaicSupply) {
		if(m_dirty)
			CATAPULT_THROW_RUNTIME_ERROR("attempt to modify dirty inflation calculator");

		if (Height() == height || (!m_inflationMap.empty() && (--m_inflationMap.cend())->first >= height))
			CATAPULT_THROW_INVALID_ARGUMENT_1("cannot add inflation entry (height)", height);

		m_inflationMap.emplace(height, InflationPair{amount, maxMosaicSupply});
		updateExpiryData();

	}

	void InflationCalculator::remove(Height height) {
		if(m_dirty)
			CATAPULT_THROW_RUNTIME_ERROR("attempt to modify dirty inflation calculator");
		m_inflationMap.erase(height);
		updateExpiryData();
	}
}}
