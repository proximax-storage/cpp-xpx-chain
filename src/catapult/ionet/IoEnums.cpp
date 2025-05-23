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

#include "ConnectionSecurityMode.h"
#include "ConnectResult.h"
#include "PacketExtractor.h"
#include "SocketOperationCode.h"
#include "catapult/utils/ConfigurationValueParsers.h"

namespace catapult { namespace ionet {

#define DEFINE_ENUM ConnectionSecurityMode
#define EXPLICIT_VALUE_ENUM
#define ENUM_LIST CONNECTION_SECURITY_MODE_LIST
#include "catapult/utils/MacroBasedEnum.h"
#undef ENUM_LIST
#undef EXPLICIT_VALUE_ENUM
#undef DEFINE_ENUM

#define DEFINE_ENUM ConnectResult
#define ENUM_LIST CONNECT_RESULT_LIST
#include "catapult/utils/MacroBasedEnum.h"
#undef ENUM_LIST
#undef DEFINE_ENUM

#define DEFINE_ENUM PacketExtractResult
#define ENUM_LIST PACKET_EXTRACT_RESULT_LIST
#include "catapult/utils/MacroBasedEnum.h"
#undef ENUM_LIST
#undef DEFINE_ENUM

#define DEFINE_ENUM SocketOperationCode
#define ENUM_LIST SOCKET_OPERATION_CODE_LIST
#include "catapult/utils/MacroBasedEnum.h"
#undef ENUM_LIST
#undef DEFINE_ENUM

#define DEFINE_ENUM PacketType
#define EXPLICIT_VALUE_ENUM
#define ENUM_LIST PACKET_TYPE_LIST
#include "catapult/utils/MacroBasedEnum.h"
#undef ENUM_LIST
#undef EXPLICIT_VALUE_ENUM
#undef DEFINE_ENUM

	namespace {
		const std::array<std::pair<const char*, ConnectionSecurityMode>, 2> String_To_Connection_Security_Mode_Pairs{{
			{ "None", ConnectionSecurityMode::None },
			{ "Signed", ConnectionSecurityMode::Signed }
		}};
	}

	bool TryParseValue(const std::string& str, ConnectionSecurityMode& modes) {
		return utils::TryParseBitwiseEnumValue(String_To_Connection_Security_Mode_Pairs, str, modes);
	}
}}
