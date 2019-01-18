/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/handlers/ReputationDiagnosticHandlers.h"
#include "tests/test/cache/CacheEntryInfosHandlerTestTraits.h"
#include "tests/test/plugins/BatchHandlerTests.h"
#include "tests/TestHarness.h"

namespace catapult { namespace handlers {

	namespace {
		struct ReputationInfosTraits : public test::CacheEntryInfoHandlerTestTraits<Key, ionet::PacketType::Reputation_Infos> {
		public:
			static constexpr auto Valid_Request_Payload_Size = Key_Size;

		public:
			template<typename TAction>
			static void RegisterHandler(ionet::ServerPacketHandlers& handlers, TAction action) {
				RegisterReputationInfosHandler(handlers, test::BatchHandlerSupplierActionToProducer<ResponseType>(action));
			}
		};
	}

	DEFINE_BATCH_HANDLER_TESTS(ReputationDiagnosticHandlersTests, ReputationInfos)
}}
