/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
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

#include "AccountMetadataMapper.h"
#include "AccountExtendedMetadataMapper.h"
#include "MosaicMetadataMapper.h"
#include "MosaicExtendedMetadataMapper.h"
#include "NamespaceMetadataMapper.h"
#include "NamespaceExtendedMetadataMapper.h"
#include "storages/MongoMetadataCacheStorage.h"
#include "mongo/src/MongoPluginManager.h"

extern "C" PLUGIN_API
void RegisterMongoSubsystem(catapult::mongo::MongoPluginManager& manager) {
	// transaction support
	manager.addTransactionSupport(catapult::mongo::plugins::CreateAccountMetadataTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateAccountExtendedMetadataTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateMosaicMetadataTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateMosaicExtendedMetadataTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateNamespaceMetadataTransactionMongoPlugin());
	manager.addTransactionSupport(catapult::mongo::plugins::CreateNamespaceExtendedMetadataTransactionMongoPlugin());

	// cache storage support
	manager.addStorageSupport(catapult::mongo::plugins::CreateMongoMetadataCacheStorage(
			manager.mongoContext(),
			manager.configHolder()));
}
