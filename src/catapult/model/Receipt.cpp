/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Receipt.h"
#include "catapult/utils/MemoryUtils.h"

namespace catapult { namespace model {

	UniqueEntityPtr<OfferExchangeReceipt> CreateOfferExchangeReceipt(
			ReceiptType receiptType,
			const Key& sender,
			const std::pair<MosaicId, MosaicId>& mosaicsPair,
			const std::vector<ExchangeDetail>& exchangeDetails) {
		uint32_t size = sizeof(OfferExchangeReceipt) + exchangeDetails.size() * sizeof(ExchangeDetail);
		auto pReceipt = utils::MakeUniqueWithSize<OfferExchangeReceipt>(size);
		pReceipt->Size = size;
		pReceipt->Version = 1;
		pReceipt->Type = receiptType;
		pReceipt->Sender = sender;
		pReceipt->MosaicsPair = mosaicsPair;
		pReceipt->ExchangeDetailCount = utils::checked_cast<size_t, uint16_t>(exchangeDetails.size());
		auto pDetail = reinterpret_cast<ExchangeDetail*>(pReceipt.get() + 1);
		for (const auto& detail : exchangeDetails) {
			*pDetail = detail;
			pDetail++;
		}

		return pReceipt;
	}
}}
