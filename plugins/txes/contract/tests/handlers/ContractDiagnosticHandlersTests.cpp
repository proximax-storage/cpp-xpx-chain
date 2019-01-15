/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/handlers/ContractDiagnosticHandlers.h"
#include "tests/test/cache/CacheEntryInfosHandlerTestTraits.h"
#include "tests/test/plugins/BatchHandlerTests.h"
#include "tests/TestHarness.h"

namespace catapult { namespace handlers {

	namespace {
		struct ContractInfosTraits : public test::CacheEntryInfoHandlerTestTraits<Key, ionet::PacketType::Contract_Infos> {
		public:
			static constexpr auto Valid_Request_Payload_Size = Key_Size;

		public:
			template<typename TAction>
			static void RegisterHandler(ionet::ServerPacketHandlers& handlers, TAction action) {
				RegisterContractInfosHandler(handlers, test::BatchHandlerSupplierActionToProducer<ResponseType>(action));
			}
		};
	}

	DEFINE_BATCH_HANDLER_TESTS(ContractDiagnosticHandlersTests, ContractInfos)
}}
