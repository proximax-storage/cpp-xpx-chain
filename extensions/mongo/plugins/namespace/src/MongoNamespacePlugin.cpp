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

#include "AddressAliasMapper.h"
#include "MosaicAliasMapper.h"
#include "NamespaceExpiryReceiptMapper.h"
#include "RegisterNamespaceMapper.h"
#include "storages/MongoNamespaceCacheStorage.h"
#include "mongo/src/MongoPluginManager.h"
#include "mongo/src/MongoReceiptPluginFactory.h"
#include "plugins/txes/namespace/src/model/NamespaceReceiptType.h"

extern "C" PLUGIN_API
void RegisterMongoSubsystem(catapult::mongo::MongoPluginManager& manager) {
	// transaction support
	manager.addTransactionSupport(catapult::mongo::plugins::CreateAddressAliasTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateMosaicAliasTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateRegisterNamespaceTransactionMongoPlugin());

	// cache storage support
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoNamespaceCacheStorage(
			manager.mongoContext(),
			manager.configHolder()));

	// receipt support
	manager.addReceiptSupport(catapult::mongo::plugins::CreateNamespaceExpiryReceiptMongoPlugin());
	manager.addReceiptSupport(catapult::mongo::CreateBalanceTransferReceiptMongoPlugin(
			catapult::model::Receipt_Type_Namespace_Rental_Fee));
}
