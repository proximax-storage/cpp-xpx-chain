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

#include "MultisigDiagnosticHandlers.h"
#include "catapult/handlers/HandlerFactory.h"

namespace catapult { namespace handlers {

	namespace {
		struct MultisigInfosTraits {
			using RequestStructureType = Key;

			static constexpr auto Packet_Type = ionet::PacketType::Multisig_Infos;
		};
	}

	void RegisterMultisigInfosHandler(
			ionet::ServerPacketHandlers& handlers,
			const MultisigInfosProducerFactory& multisigInfosProducerFactory) {
		BatchHandlerFactory<MultisigInfosTraits>::RegisterOne(handlers, multisigInfosProducerFactory);
	}
}}
