/**
*** FOR TRAINING PURPOSES ONLY
**/

#pragma once
#include "src/model/HelloNotification.h"
#include "catapult/observers/ObserverTypes.h"

namespace catapult { namespace observers {

        /// Observes changes triggered by blockchain hello notifications
        // expands to CreateHelloObserver
        DECLARE_OBSERVER(Hello, model::HelloMessageCountNotification<1>)();
    }}
