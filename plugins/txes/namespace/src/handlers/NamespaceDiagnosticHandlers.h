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

#pragma once
#include "src/types.h"
#include "catapult/handlers/HandlerTypes.h"
#include "catapult/model/CacheEntryInfo.h"

namespace catapult { namespace ionet { class ServerPacketHandlers; } }

namespace catapult { namespace handlers {

	/// Alias for a mosaic infos producer factory.
	using MosaicInfosProducerFactory = SharedPointerProducerFactory<MosaicId, model::CacheEntryInfo<MosaicId>>;

	/// Alias for a namespace infos producer factory.
	using NamespaceInfosProducerFactory = SharedPointerProducerFactory<NamespaceId, model::CacheEntryInfo<NamespaceId>>;

	/// Registers a mosaic infos handler in \a handlers that responds with mosaic infos
	/// returned by a producer from \a mosaicInfosProducerFactory.
	void RegisterMosaicInfosHandler(ionet::ServerPacketHandlers& handlers, const MosaicInfosProducerFactory& mosaicInfosProducerFactory);

	/// Registers a namespace infos handler in \a handlers that responds with namespace infos
	/// returned by a producer from \a namespaceInfosProducerFactory.
	void RegisterNamespaceInfosHandler(
			ionet::ServerPacketHandlers& handlers,
			const NamespaceInfosProducerFactory& namespaceInfosProducerFactory);
}}
