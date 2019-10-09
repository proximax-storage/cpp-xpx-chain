/**
*** FOR TRAINING PURPOSES ONLY
**/

#include "src/plugins/HelloTransactionPlugin.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "src/model/HelloNotification.h"
#include "src/model/HelloTransaction.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/mocks/MockNotificationSubscriber.h"
#include "tests/test/plugins/TransactionPluginTestUtils.h"
#include "tests/TestHarness.h"
#include "tests/test/HelloTestUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

#define TEST_CLASS HelloTransactionPluginTests

        namespace {
            // defined in tests\test\plugins\TransactionPluginTestUtils.h
            //	might be needed when calling TTraits::CreatePlugin();
            DEFINE_TRANSACTION_PLUGIN_TEST_TRAITS(Hello, 1, 1,)

            constexpr auto Transaction_Version = MakeVersion(model::NetworkIdentifier::Mijin_Test, 1);

        }

        // defined in tests\test\plugins\TransactionPluginTestUtils.h
        // Entity_Type_Hello was defined in model\HelloEntityType.h
        DEFINE_BASIC_EMBEDDABLE_TRANSACTION_PLUGIN_TESTS(TEST_CLASS, , , Entity_Type_Hello)

        PLUGIN_TEST(ExecutePlugin) {
            // Arrange:
            mocks::MockNotificationSubscriber sub;
            auto pPlugin = TTraits::CreatePlugin();
            auto key = test::GenerateKeys(1);

            typename TTraits::TransactionType transaction;
            transaction.Version = Transaction_Version;  // must set, defined above
            transaction.MessageCount = 5;
            //transaction.SignerKey = key;                 // operator = not available

            // Act:
            test::PublishTransaction(*pPlugin, transaction, sub);

            EXPECT_EQ(1, sub.numNotifications());
        }

    }}
