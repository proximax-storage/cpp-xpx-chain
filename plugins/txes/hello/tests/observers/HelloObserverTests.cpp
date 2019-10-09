//
// Created by ruell on 09/10/2019.
//


#include "src/observers/Observers.h"
#include "tests/test/HelloTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/test/HelloTestUtils.h"
#include "tests/TestHarness.h"
#include "src/model/HelloNotification.h"

namespace catapult { namespace observers {

#define TEST_CLASS HelloObserverTests

        //defined in \tests\test\plugins\ObserverTestUtils.h
        //expands to CanCreateHelloObserver
        DEFINE_COMMON_OBSERVER_TESTS(Hello, )

        namespace {
            // test::HelloCacheFactory is defined in tests/test/HelloTestUtils
            using ObserverTestContext = test::ObserverTestContextT<test::HelloCacheFactory>;
            using Notification = model::HelloMessageCountNotification<1>;

            struct HelloValues {
            public:
                catapult::Height Height;
                uint16_t messageCount;
                Key key;
            };

            std::unique_ptr<Notification> CreateNotification(const HelloValues& values) {
                return std::make_unique<Notification>(values.messageCount, values.key);
            }

            void RunTest(
                    NotifyMode mode,
                    const Notification& notification,
                    bool entryExists,
                    const HelloValues& expectedValues) {
                // Arrange:
                ObserverTestContext context(mode, expectedValues.Height);   //??? Why is height needed?, thus this mean create a context in current height?
                auto pObserver = CreateHelloObserver();

                // Act:
                test::ObserveNotification(*pObserver, notification, context);

                // Assert: check the cache
                auto& HelloCacheDelta = context.cache().sub<cache::HelloCache>();

                EXPECT_EQ(entryExists, HelloCacheDelta.contains(expectedValues.key));
                if (entryExists) {
                    const auto& entry = HelloCacheDelta.find(expectedValues.key).get();
                    EXPECT_EQ(expectedValues.messageCount, entry.messageCount());
                }


            }
        }

        TEST(TEST_CLASS, Hello_Commit) {
            // Arrange:
            auto key = test::GenerateKeys(1);
            auto values = HelloValues{ Height{1}, 2, key[0] };
            auto notification = CreateNotification(values);

            // Assert:
            RunTest(NotifyMode::Commit, *notification, true, values);
        }

        TEST(TEST_CLASS, Hello_Rollback) {
            // Arrange:
            auto key = test::GenerateKeys(1);
            auto values = HelloValues{ Height{1}, 2, key[0] };
            auto notification = CreateNotification(values);

            // Assert:
            RunTest(NotifyMode::Rollback, *notification, false, values);
        }
    }}
