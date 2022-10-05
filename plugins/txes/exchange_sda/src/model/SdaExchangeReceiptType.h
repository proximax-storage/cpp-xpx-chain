/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#ifndef CUSTOM_RECEIPT_TYPE_DEFINITION
#include "catapult/model/ReceiptType.h"

namespace catapult { namespace model {

#endif

	/// SDA-SDA offer created.
	DEFINE_RECEIPT_TYPE(OfferCreation, ExchangeSda, Sda_Offer_Created, 1);

	/// SDA-SDA offer exchanged.
	DEFINE_RECEIPT_TYPE(OfferExchange, ExchangeSda, Sda_Offer_Exchanged, 2);

	/// SDA-SDA offer removed.
	DEFINE_RECEIPT_TYPE(OfferRemoval, ExchangeSda, Sda_Offer_Removed, 3);

#ifndef CUSTOM_RECEIPT_TYPE_DEFINITION
}}
#endif
