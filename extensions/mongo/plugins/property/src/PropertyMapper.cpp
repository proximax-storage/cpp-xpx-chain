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

#include "PropertyMapper.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "plugins/txes/property/src/model/AddressPropertyTransaction.h"
#include "plugins/txes/property/src/model/MosaicPropertyTransaction.h"
#include "plugins/txes/property/src/model/TransactionTypePropertyTransaction.h"
#include "plugins/txes/property/src/state/PropertyUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		void StreamModification(
				bson_stream::array_context& context,
				model::PropertyModificationType type,
				const std::vector<uint8_t>& value) {
			context
					<< bson_stream::open_document
						<< "type" << utils::to_underlying_type(type)
						<< "value" << ToBinary(value.data(), value.size())
					<< bson_stream::close_document;
		}

		template<typename TPropertyValue>
		void StreamModifications(
				bson_stream::document& builder,
				const model::PropertyModification<TPropertyValue>* pModifications,
				size_t numModifications) {
			auto modificationsArray = builder << "modifications" << bson_stream::open_array;
			for (auto i = 0u; i < numModifications; ++i)
				StreamModification(modificationsArray, pModifications[i].ModificationType, state::ToVector(pModifications[i].Value));

			modificationsArray << bson_stream::close_array;
		}

		template<typename TPropertyValue>
		struct PropertyTransactionStreamer {
			template<typename TTransaction>
			static void Stream(bson_stream::document& builder, const TTransaction& transaction) {
				builder << "propertyType" << utils::to_underlying_type(transaction.PropertyType);
				StreamModifications(builder, transaction.ModificationsPtr(), transaction.ModificationsCount);
			}
		};
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(AddressProperty, PropertyTransactionStreamer<Address>::Stream)
	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(MosaicProperty, PropertyTransactionStreamer<MosaicId>::Stream)
	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(TransactionTypeProperty, PropertyTransactionStreamer<model::EntityType>::Stream)
}}}
