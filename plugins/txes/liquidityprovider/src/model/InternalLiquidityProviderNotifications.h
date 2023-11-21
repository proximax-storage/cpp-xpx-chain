/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "catapult/model/Notifications.h"

namespace catapult { namespace model {

	DEFINE_NOTIFICATION_TYPE(All, LiquidityProvider, Create_Liquidity_Provider_v1, 0x0001);
	DEFINE_NOTIFICATION_TYPE(All, LiquidityProvider, Manual_Rate_Change_v1, 0x0002);

	template<VersionType version>
	struct CreateLiquidityProviderNotification;

	template<>
	struct CreateLiquidityProviderNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = LiquidityProvider_Create_Liquidity_Provider_v1_Notification;

	public:

		explicit CreateLiquidityProviderNotification(
				const Key& providerKey,
				const Key& owner,
				const UnresolvedMosaicId& providerMosaicId,
				const Amount& currencyDeposit,
				const Amount& initialMosaicsMinting,
				uint32_t slashingPeriod,
				uint16_t windowSize,
				const Key& slashingAccount,
				uint32_t alpha,
				uint32_t beta)
			: Notification(Notification_Type, sizeof(CreateLiquidityProviderNotification<1>))
			, ProviderKey(providerKey)
			, Owner(owner)
			, ProviderMosaicId(providerMosaicId)
			, CurrencyDeposit(currencyDeposit)
			, InitialMosaicsMinting(initialMosaicsMinting)
			, SlashingPeriod(slashingPeriod)
			, WindowSize(windowSize)
			, SlashingAccount(slashingAccount)
			, Alpha(alpha)
			, Beta(beta) {}

	public:
		Key ProviderKey;
		Key Owner;
		UnresolvedMosaicId ProviderMosaicId;
		Amount CurrencyDeposit;
		Amount InitialMosaicsMinting;
		uint32_t SlashingPeriod;
		uint16_t WindowSize;
		Key SlashingAccount;
		uint32_t Alpha;
		uint32_t Beta;
	};

	template<VersionType version>
	struct ManualRateChangeNotification;

	template<>
	struct ManualRateChangeNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = LiquidityProvider_Manual_Rate_Change_v1_Notification;

	public:

		explicit ManualRateChangeNotification(
				const Key& signer,
				const UnresolvedMosaicId& providerMosaicId,
				bool currencyBalanceIncrease,
				const Amount& currencyBalanceChange,
				bool mosaicBalanceIncrease,
				const Amount& mosaicBalanceChange)
			: Notification(Notification_Type, sizeof(ManualRateChangeNotification<1>))
			, Signer(signer)
			, ProviderMosaicId(providerMosaicId)
			, CurrencyBalanceIncrease(currencyBalanceIncrease)
			, CurrencyBalanceChange(currencyBalanceChange)
			, MosaicBalanceIncrease(mosaicBalanceIncrease)
			, MosaicBalanceChange(mosaicBalanceChange) {}

	public:
		Key Signer;
		UnresolvedMosaicId ProviderMosaicId;
		bool CurrencyBalanceIncrease;
		Amount CurrencyBalanceChange;
		bool MosaicBalanceIncrease;
		Amount MosaicBalanceChange;
	};
}}; // namespace catapult::model