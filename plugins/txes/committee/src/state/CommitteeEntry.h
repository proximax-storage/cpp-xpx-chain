/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include <cmath>

namespace catapult { namespace state {

	struct AccountData {
	public:
		// Creates a account data around \a lastSigningBlockHeight, \a effectiveBalance, \a canHarvest,
		// \a activity and \a greed.
		explicit AccountData(
				const Height& lastSigningBlockHeight,
				const Importance& effectiveBalance,
				bool canHarvest,
				double activity,
				double greed)
			: LastSigningBlockHeight(lastSigningBlockHeight)
			, EffectiveBalance(effectiveBalance)
			, CanHarvest(canHarvest)
			, Activity(activity)
			, Greed(greed)
		{}

	public:
		/// The latest height of the account signing a block.
		Height LastSigningBlockHeight;

		/// The effective balance of the account.
		Importance EffectiveBalance;

		/// Whether the account is eligible harvester.
		bool CanHarvest;

		/// Account activity. Account weight is 1 / (1 + exp(-Activity)).
		double Activity;

		/// The account greed.
		double Greed;
	};

	// Committee entry.
	class CommitteeEntry {
	public:
		// Creates a committee entry around \a key, \a lastSigningBlockHeight, \a effectiveBalance, \a canHarvest
		//	\a activity and \a greed.
		explicit CommitteeEntry(
				const Key& key,
				const Key& owner,
				const Height& lastSigningBlockHeight,
				const Importance& effectiveBalance,
				bool canHarvest,
				double activity,
				double greed,
				const Height& disabledHeight = Height(0))
			: m_key(key)
			, m_owner(owner)
			, m_disabledHeight(disabledHeight)
			, m_data(lastSigningBlockHeight, effectiveBalance, canHarvest, activity, greed)
		{}

		// Creates a committee entry around \a key and \a data.
		explicit CommitteeEntry(const Key& key, const Key& owner, const AccountData& data, const Height& disabledHeight = Height(0))
			: m_key(key)
			, m_owner(owner)
			, m_disabledHeight(disabledHeight)
			, m_data(data)
		{}

		// Creates a committee entry around \a key and \a data.
		explicit CommitteeEntry(const Key& key, const Key& owner, AccountData&& data, const Height& disabledHeight = Height(0))
			: m_key(key)
			, m_owner(owner)
			, m_disabledHeight(disabledHeight)
			, m_data(std::move(data))
		{}

	public:
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

		/// Gets the account activity.
		double activity() const {
			return m_data.Activity;
		}

		/// Sets the account \a activity.
		void setActivity(double activity) {
			m_data.Activity = activity;
		}

		/// Gets the account greed.
		double greed() const {
			return m_data.Greed;
		}

		/// Sets the account \a greed.
		void setGreed(double greed) {
			m_data.Greed = greed;
		}

	private:
		Key m_key;
		Key m_owner;
		Height m_disabledHeight;
		AccountData m_data;
	};
}}
