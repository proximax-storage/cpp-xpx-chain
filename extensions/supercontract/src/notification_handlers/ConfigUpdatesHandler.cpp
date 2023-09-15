#include "NotificationHandlers.h"

namespace catapult { namespace notification_handlers {

	using Notification = model::NetworkConfigNotification<1>;

	DECLARE_HANDLER(ConfigUpdate, Notification)(const std::weak_ptr<contract::ExecutorService>& pExecutorServiceWeak) {
		return MAKE_HANDLER(ConfigUpdate, [pExecutorServiceWeak](const Notification& notification, const HandlerContext& context) {
			auto pExecutorService = pExecutorServiceWeak.lock();
			if (!pExecutorService) {
				return;
			}
			try {
				pExecutorService->updateConfig(notification.ApplyHeightDelta);
			}
			catch (...) {
				CATAPULT_LOG(warning) << "An exception has occurred in the executor";
				pExecutorService->restart();
			}
		});
	}
}}
