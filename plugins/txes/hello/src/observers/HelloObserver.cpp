//
// Created by ruell on 08/10/2019.
//


#include "Observers.h"
#include "src/cache/HelloCache.h"


// Comment/notes: Observer also listens to notification, on this example, it listens to model::HelloMessageCountNotification
// which was triggered by Publish() in HelloTransactionPlugin
// when this notification is recieved, observerver insert data to cache

namespace catapult { namespace observers {

        using Notification = model::HelloMessageCountNotification<1>;

        DECLARE_OBSERVER(Hello, Notification)() {
            return MAKE_OBSERVER(Hello, Notification, [](const auto& notification, const ObserverContext& context) {

                // Hello Count notification recieved (triggered in Publish), insert to cache new data

                auto& cache = context.Cache.sub<cache::HelloCache>();           //? what does this do? does it instantiate or retrive current cache?

                if (NotifyMode::Commit == context.Mode) {
                    cache.insert(state::HelloEntry(notification.SignerKey, notification.MessageCount));
                }
            });
        }
    }}
