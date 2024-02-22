/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include <cmath>
#include <utility>

namespace catapult { namespace state {

	struct AccountData {
	public:
		AccountData(
				Height lastSigningBlockHeight,
				Importance effectiveBalance,
				bool canHarvest,
				double activityObsolete,
				double greedObsolete,
				Timestamp expirationTime,
				int64_t activity,
				uint32_t feeInterest,
				uint32_t feeInterestDenominator,
				const Key& bootKey)
			: LastSigningBlockHeight(std::move(lastSigningBlockHeight))
			, EffectiveBalance(std::move(effectiveBalance))
			, CanHarvest(canHarvest)
			, ActivityObsolete(activityObsolete)
			, GreedObsolete(greedObsolete)
			, ExpirationTime(std::move(expirationTime))
			, Activity(activity)
			, FeeInterest(feeInterest)
			, FeeInterestDenominator(feeInterestDenominator)
			, BootKey(bootKey)
		{}

	public:
		/// Safely increases the account activity by \a delta.
		/// If data overflow is detected then activity is set to maximum value.
		void increaseActivity(int64_t delta) {
			if (delta < 0) {
				decreaseActivity(-delta);
			} else if (Activity > 0 && std::numeric_limits<int64_t>::max() - Activity < delta) {
				Activity = std::numeric_limits<int64_t>::max();
			} else {
				Activity += delta;
			}
		}

		/// Safely decreases the account activity by \a delta.
		/// If data underflow is detected then activity is set to minimum value.
		void decreaseActivity(int64_t delta) {
			if (delta < 0) {
				increaseActivity(-delta);
			} else if (Activity < 0 && Activity - std::numeric_limits<int64_t>::min() < delta) {
				Activity = std::numeric_limits<int64_t>::min();
			} else {
				Activity -= delta;
			}
		}

	public:
		/// The latest height of the account signing a block.
		Height LastSigningBlockHeight;

		/// The effective balance of the account.
		Importance EffectiveBalance;

		/// Whether the account is eligible harvester.
		bool CanHarvest;

		/// Obsolete account activity due to switching from double to uint64.
		double ActivityObsolete;

		/// Obsolete account greed due to switching from double to uint64.
		double GreedObsolete;

		/// The time after which the harvester is considered inactive and is not selected into committee.
		Timestamp ExpirationTime;

		/// Account activity. Account weight is 1 / (1 + exp(-Activity)).
		int64_t Activity;

		/// The part of the transaction fee harvester is willing to get.
		/// From 0 up to FeeInterestDenominator. The customer gets
		/// (FeeInterest / FeeInterestDenominator)'th part of the maximum transaction fee.
		uint32_t FeeInterest;

		/// Denominator of the transaction fee.
		uint32_t FeeInterestDenominator;

		/// Boot key of the node where the harvesters are set up.
		Key BootKey;
	};

	// Committee entry.
	class CommitteeEntry {
	public:
		CommitteeEntry(
				const Key& key,
				const Key& owner,
				const Height& lastSigningBlockHeight,
				const Importance& effectiveBalance,
				bool canHarvest,
				double activityObsolete,
				double greedObsolete,
				Height disabledHeight = Height(0),
				VersionType version = 1,
				Timestamp expirationTime = Timestamp(0),
				int64_t activity = 0u,
				uint32_t feeInterest = 0u,
				uint32_t feeInterestDenominator = 0u,
				const Key& bootKey = Key())
			: m_key(key)
			, m_owner(owner)
			, m_disabledHeight(std::move(disabledHeight))
			, m_data(lastSigningBlockHeight, effectiveBalance, canHarvest, activityObsolete, greedObsolete, std::move(expirationTime), activity, feeInterest, feeInterestDenominator, bootKey)
			, m_version(version)
		{}

		// Creates a committee entry around \a key and \a data.
		CommitteeEntry(const Key& key, const Key& owner, const AccountData& data, const Height& disabledHeight = Height(0), VersionType version = 1)
			: m_key(key)
			, m_owner(owner)
			, m_disabledHeight(disabledHeight)
			, m_data(data)
			, m_version(version)
		{}

		// Creates a committee entry around \a key and \a data.
		CommitteeEntry(const Key& key, const Key& owner, AccountData&& data, const Height& disabledHeight = Height(0), VersionType version = 1)
			: m_key(key)
			, m_owner(owner)
			, m_disabledHeight(disabledHeight)
			, m_data(std::move(data))
			, m_version(version)
		{}

	public:
		/// Gets the entry version.
		VersionType version() const {
			return m_version;
		}

		/// Sets the entry version.
		void setVersion(VersionType version) {
			m_version = version;
		}

		/// Gets the harvester public key.
		const Key& key() const {
			return m_key;
		}

		/// Gets the harvester owner public key.
		const Key& owner() const {
			return m_owner;
		}

		void setDisabledHeight(Height height) {
			m_disabledHeight = height;
		}

		Height disabledHeight() const {
			return m_disabledHeight;
		}

		/// Gets the account data.
		const AccountData& data() const {
			return m_data;
		}

		/// Gets the latest height of the account signing a block.
		const Height& lastSigningBlockHeight() const {
			return m_data.LastSigningBlockHeight;
		}

		/// Sets the latest \a height of the account signing a block.
		void setLastSigningBlockHeight(const Height& height) {
			m_data.LastSigningBlockHeight = height;
		}

		/// Gets the effective balance of the account.
		const Importance& effectiveBalance() const {
			return m_data.EffectiveBalance;
		}

		/// Sets the effective \a balance of the account.
		void setEffectiveBalance(const Importance& balance) {
			m_data.EffectiveBalance = balance;
		}

		/// Gets whether the account can harvest.
		const bool& canHarvest() const {
			return m_data.CanHarvest;
		}

		/// Sets whether the account \a canHarvest.
		void setCanHarvest(bool canHarvest) {
			m_data.CanHarvest = canHarvest;
		}

		/// Gets the account activity (obsolete).
		double activityObsolete() const {
			return m_data.ActivityObsolete;
		}

		/// Sets the account \a activityObsolete.
		void setActivityObsolete(double activityObsolete) {
			m_data.ActivityObsolete = activityObsolete;
		}

		/// Gets the account greed (obsolete).
		double greedObsolete() const {
			return m_data.GreedObsolete;
		}

		/// Sets the account \a greedObsolete.
		void setGreedObsolete(double greedObsolete) {
			m_data.GreedObsolete = greedObsolete;
		}

		const Timestamp& expirationTime() const {
			return m_data.ExpirationTime;
		}

		void setExpirationTime(const Timestamp& expirationTime) {
			m_data.ExpirationTime = expirationTime;
		}

		/// Gets the account activity.
		int64_t activity() const {
			return m_data.Activity;
		}

		/// Sets the account \a activity.
		void setActivity(int64_t activity) {
			m_data.Activity = activity;
		}

		/// Gets the fee interest.
		uint32_t feeInterest() const {
			return m_data.FeeInterest;
		}

		/// Sets \a feeInterest.
		void setFeeInterest(uint32_t feeInterest) {
			m_data.FeeInterest = feeInterest;
		}

		/// Gets the fee interest denominator.
		uint32_t feeInterestDenominator() const {
			return m_data.FeeInterestDenominator;
		}

		/// Sets \a feeInterestDenominator.
		void setFeeInterestDenominator(uint32_t feeInterestDenominator) {
			m_data.FeeInterestDenominator = feeInterestDenominator;
		}

		/// Gets the boot key.
		const Key& bootKey() const {
			return m_data.BootKey;
		}

		void setBootKey(const Key& bootKey) {
			m_data.BootKey = bootKey;
		}

	private:
		Key m_key;
		Key m_owner;
		Height m_disabledHeight;
		AccountData m_data;
		VersionType m_version;
	};
}}
