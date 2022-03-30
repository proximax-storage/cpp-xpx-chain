/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/exceptions.h"
#include "catapult/model/Mosaic.h"
#include "catapult/utils/ArraySet.h"
#include "catapult/utils/IntegerMath.h"
#include <vector>
#include <deque>
#include <boost/multiprecision/cpp_int.hpp>

namespace catapult { namespace state {

	using BigUint = boost::multiprecision::uint128_t;

	struct ExchangeRate {
		Amount m_currencyAmount;
		Amount m_mosaicAmount;

		bool operator <(const ExchangeRate& other) const {
			BigUint aCurrency = m_currencyAmount.unwrap();
			BigUint bCurrency = other.m_currencyAmount.unwrap();

			return (aCurrency * other.m_mosaicAmount.unwrap()) < (bCurrency * other.m_mosaicAmount.unwrap());
		}
	};

	struct HistoryObservation {

		// Rate at the beginning of the observation
		ExchangeRate m_rate = {Amount{0}, Amount{0}};

		Amount m_turnover = Amount{0};
	};

	// Mixin for storing drive details.
	class LiquidityProviderMixin {
	public:
		LiquidityProviderMixin()
			: m_additionallyMinted(0)
			, m_slashingPeriod(0)
			, m_windowSize(0)
			, m_creationHeight(0)
			, m_alpha(0)
			, m_beta(0)
		{}

	public:
		const Key& providerKey() const {
			return m_providerKey;
		}
		void setProviderKey(const Key& providerKey) {
			m_providerKey = providerKey;
		}

		void setOwner(const Key& owner) {
			m_owner = owner;
		}

		const Key& owner() const {
			return m_owner;
		}

		const Amount& additionallyMinted() const {
			return m_additionallyMinted;
		}

		void setAdditionallyMinted(const Amount& additionallyMinted) {
			m_additionallyMinted = additionallyMinted;
		}

		const Key& slashingAccount() const {
			return m_slashingAccount;
		}

		void setSlashingAccount(const Key& slashingAccount) {
			m_slashingAccount = slashingAccount;
		}

		uint32_t slashingPeriod() const {
			return m_slashingPeriod;
		}

		void setSlashingPeriod(uint32_t slashingPeriod) {
			m_slashingPeriod = slashingPeriod;
		}

		uint16_t windowSize() const {
			return m_windowSize;
		}

		void setWindowSize(uint16_t windowSize) {
			m_windowSize = windowSize;
		}

		const Height& creationHeight() const {
			return m_creationHeight;
		}
		void setCreationHeight(const Height& creationHeight) {
			m_creationHeight = creationHeight;
		}

		const std::deque<HistoryObservation>& turnoverHistory() const {
			return m_turnoverHistory;
		}

		std::deque<HistoryObservation>& turnoverHistory() {
			return m_turnoverHistory;
		}

		const HistoryObservation& recentTurnover() const {
			return m_recentTurnover;
		}

		HistoryObservation& recentTurnover() {
			return m_recentTurnover;
		}

		uint32_t alpha() const {
			return m_alpha;
		}

		void setAlpha(uint32_t alpha) {
			m_alpha = alpha;
		}

		uint32_t beta() const {
			return m_beta;
		}

		void setBeta(uint32_t beta) {
			m_beta = beta;
		}

	private:
		Key m_providerKey;
		Key m_owner;
		Amount m_additionallyMinted;
		uint32_t m_slashingPeriod;
		uint16_t m_windowSize;
		Height m_creationHeight;
		Key m_slashingAccount;
		uint32_t m_alpha;
		uint32_t m_beta;

		std::deque<HistoryObservation> m_turnoverHistory;
		HistoryObservation m_recentTurnover;
	};

	// Drive entry.
	class LiquidityProviderEntry : public LiquidityProviderMixin {
	public:
		// Creates a drive entry around \a key.
		explicit LiquidityProviderEntry(const UnresolvedMosaicId& mosaicId) : m_mosaicId(mosaicId), m_version(1)
		{}

	public:
		// Gets the drive public key.
		const UnresolvedMosaicId& mosaicId() const {
			return m_mosaicId;
		}

		void setVersion(VersionType version) {
			m_version = version;
		}

		VersionType version() const {
			return m_version;
		}

	private:
		UnresolvedMosaicId m_mosaicId;
		VersionType m_version;
	};
}}
