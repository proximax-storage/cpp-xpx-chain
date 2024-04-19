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

#include "MongoReceiptPluginFactory.h"
#include "mappers/MapperUtils.h"

namespace catapult { namespace mongo {

	namespace {
		void StreamBalanceTransferReceipt(bsoncxx::builder::stream::document& builder, const model::BalanceTransferReceipt& receipt) {
			builder
					<< "sender" << mappers::ToBinary(receipt.Sender)
					<< "recipient" << mappers::ToBinary(receipt.Recipient)
					<< "mosaicId" << mappers::ToInt64(receipt.MosaicId)
					<< "amount" << mappers::ToInt64(receipt.Amount);
		}

		void StreamBalanceChangeReceipt(bsoncxx::builder::stream::document& builder, const model::BalanceChangeReceipt& receipt) {
			builder
					<< "account" << mappers::ToBinary(receipt.Account)
					<< "mosaicId" << mappers::ToInt64(receipt.MosaicId)
					<< "amount" << mappers::ToInt64(receipt.Amount);
		}

		void StreamInflationReceipt(bsoncxx::builder::stream::document& builder, const model::InflationReceipt& receipt) {
			builder
					<< "mosaicId" << mappers::ToInt64(receipt.MosaicId)
					<< "amount" << mappers::ToInt64(receipt.Amount);
		}

		void StreamDriveStateReceipt(bsoncxx::builder::stream::document& builder, const model::DriveStateReceipt& receipt) {
			builder
					<< "driveKey" << mappers::ToBinary(receipt.DriveKey)
					<< "driveState" << receipt.DriveState;
		}

		void StreamOfferCreationReceipt(bsoncxx::builder::stream::document& builder, const model::OfferCreationReceipt& receipt) {
			builder
					<< "sender" << mappers::ToBinary(receipt.Sender)
					<< "mosaicIdGive" << mappers::ToInt64(receipt.MosaicsPair.first)
					<< "mosaicIdGet" << mappers::ToInt64(receipt.MosaicsPair.second)
					<< "mosaicAmountGive" << mappers::ToInt64(receipt.AmountGive)
					<< "mosaicAmountGet" << mappers::ToInt64(receipt.AmountGet);
		}

		void StreamOfferExchangeReceipt(bsoncxx::builder::stream::document& builder, const model::OfferExchangeReceipt& receipt) {
			builder
					<< "sender" << mappers::ToBinary(receipt.Sender)
					<< "mosaicIdGive" << mappers::ToInt64(receipt.MosaicsPair.first)
					<< "mosaicIdGet" << mappers::ToInt64(receipt.MosaicsPair.second);
			auto exchangeDetailArray = builder << "exchangeDetails" << mappers::bson_stream::open_array;
			auto pDetail = reinterpret_cast<const model::ExchangeDetail*>(&receipt + 1);
			for (auto i = 0u; i < receipt.ExchangeDetailCount; ++i, ++pDetail) {
				exchangeDetailArray
					<< mappers::bson_stream::open_document
					<< "recipient" << mappers::ToBinary(pDetail->Recipient)
					<< "mosaicIdGive" << mappers::ToInt64(pDetail->MosaicsPair.first)
					<< "mosaicIdGet" << mappers::ToInt64(pDetail->MosaicsPair.second)
					<< "mosaicAmountGive" << mappers::ToInt64(pDetail->AmountGive)
					<< "mosaicAmountGet" << mappers::ToInt64(pDetail->AmountGet)
					<< mappers::bson_stream::close_document;
			}
			exchangeDetailArray << mappers::bson_stream::close_array;
		}

		void StreamOfferRemovalReceipt(bsoncxx::builder::stream::document& builder, const model::OfferRemovalReceipt& receipt) {
			builder
					<< "sender" << mappers::ToBinary(receipt.Sender)
					<< "mosaicIdGive" << mappers::ToInt64(receipt.MosaicsPair.first)
					<< "mosaicIdGet" << mappers::ToInt64(receipt.MosaicsPair.second)
					<< "mosaicAmountGiveReturned" << mappers::ToInt64(receipt.AmountGiveReturned);
		}

		void StreamStorageReceipt(bsoncxx::builder::stream::document& builder, const model::StorageReceipt& receipt) {
			builder
					<< "sender" << mappers::ToBinary(receipt.Sender)
					<< "recipient" << mappers::ToBinary(receipt.Recipient)
					<< "mosaicIdSent" << mappers::ToInt64(receipt.MosaicsPair.first)
					<< "mosaicIdReceived" << mappers::ToInt64(receipt.MosaicsPair.second)
					<< "sentAmount" << mappers::ToInt64(receipt.SentAmount);
		}
	}

	DEFINE_MONGO_RECEIPT_PLUGIN_FACTORY(BalanceTransfer, StreamBalanceTransferReceipt)
	DEFINE_MONGO_RECEIPT_PLUGIN_FACTORY(BalanceChange, StreamBalanceChangeReceipt)
	DEFINE_MONGO_RECEIPT_PLUGIN_FACTORY(Inflation, StreamInflationReceipt)
	DEFINE_MONGO_RECEIPT_PLUGIN_FACTORY(DriveState, StreamDriveStateReceipt)
	DEFINE_MONGO_RECEIPT_PLUGIN_FACTORY(OfferCreation, StreamOfferCreationReceipt)
	DEFINE_MONGO_RECEIPT_PLUGIN_FACTORY(OfferExchange, StreamOfferExchangeReceipt)
	DEFINE_MONGO_RECEIPT_PLUGIN_FACTORY(OfferRemoval, StreamOfferRemovalReceipt)
	DEFINE_MONGO_RECEIPT_PLUGIN_FACTORY(Storage, StreamStorageReceipt)
}}
