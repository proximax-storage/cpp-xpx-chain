/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "catapult/model/Notifications.h"

namespace catapult { namespace model {

	/// Defines a data modification notification type.
	DEFINE_NOTIFICATION_TYPE(All, LiquidityProvider, Debit_Mosaic_v1, 0x0003);

	DEFINE_NOTIFICATION_TYPE(All, LiquidityProvider, Credit_Mosaic_v1, 0x0004);

	template<VersionType version>
	struct CreditMosaicNotification;

	template<>
	struct CreditMosaicNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = LiquidityProvider_Credit_Mosaic_v1_Notification;

	public:
		explicit CreditMosaicNotification(
				const Key& currencyDebtor,
				const Key& mosaicCreditor,
				const UnresolvedMosaicId& mosaicId,
				const UnresolvedAmount& mosaicAmount)
			: Notification(Notification_Type, sizeof(CreditMosaicNotification<1>))
			, CurrencyDebtor(currencyDebtor)
			, MosaicCreditor(mosaicCreditor)
			, MosaicId(mosaicId)
			, MosaicAmount(mosaicAmount)
		{}

		explicit CreditMosaicNotification(
				const Key& currencyDebtor,
				const Key& mosaicCreditor,
				const UnresolvedMosaicId& mosaicId,
				const Amount& mosaicAmount)
				: Notification(Notification_Type, sizeof(CreditMosaicNotification<1>))
				, CurrencyDebtor(currencyDebtor)
				, MosaicCreditor(mosaicCreditor)
				, MosaicId(mosaicId)
				, MosaicAmount { mosaicAmount.unwrap() }
				{}

	public:
		Key CurrencyDebtor;

		Key MosaicCreditor;

		UnresolvedMosaicId MosaicId;

		UnresolvedAmount MosaicAmount;
	};

	template<VersionType version>
	struct DebitMosaicNotification;

	template<>
	struct DebitMosaicNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = LiquidityProvider_Debit_Mosaic_v1_Notification;

	public:

		explicit DebitMosaicNotification(
				const Key& mosaicDebtor,
				const Key& currencyCreditor,
				const UnresolvedMosaicId& mosaicId,
				const UnresolvedAmount& mosaicAmount)
				: Notification(Notification_Type, sizeof(CreditMosaicNotification<1>))
				, MosaicDebtor(mosaicDebtor)
				, CurrencyCreditor(currencyCreditor)
				, MosaicId(mosaicId)
				, MosaicAmount(mosaicAmount) {}

		explicit DebitMosaicNotification(
				const Key& mosaicDebtor,
				const Key& currencyCreditor,
				const UnresolvedMosaicId& mosaicId,
				const Amount& mosaicAmount)
			: DebitMosaicNotification(
					  mosaicDebtor,
					  currencyCreditor,
					  mosaicId,
					  UnresolvedAmount { mosaicAmount.unwrap() }) {}

	public:
		Key MosaicDebtor;

		Key CurrencyCreditor;

		UnresolvedMosaicId MosaicId;

		UnresolvedAmount MosaicAmount;
	};
}}
