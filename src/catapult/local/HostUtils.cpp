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

#include "HostUtils.h"
#include "catapult/extensions/ProcessBootstrapper.h"
#include "catapult/plugins/PluginLoader.h"

namespace catapult { namespace local {

	namespace {
		class BootstrapperPluginLoader {
		public:
			explicit BootstrapperPluginLoader(extensions::ProcessBootstrapper& bootstrapper)
					: m_bootstrapper(bootstrapper)
			{}

		public:
			const std::vector<plugins::PluginModule>& modules() {
				return m_pluginModules;
			}

		public:
			void loadAll() {
				for (const auto& pluginName : m_bootstrapper.extensionManager().systemPluginNames())
					loadOne(pluginName);

				for (const auto& pair : m_bootstrapper.pluginManager().config(Height{0}).Plugins)
					loadOne(pair.first);
			}

		private:
			void loadOne(const std::string& pluginName) {
				auto& pluginManager = m_bootstrapper.pluginManager();
				LoadPluginByName(pluginManager, m_pluginModules, pluginManager.configHolder()->Config(Height{0}).User.PluginsDirectory, pluginName);
			}

		private:
			extensions::ProcessBootstrapper& m_bootstrapper;

			std::vector<plugins::PluginModule> m_pluginModules;
		};
	}

	std::vector<plugins::PluginModule> LoadAllPlugins(extensions::ProcessBootstrapper& bootstrapper) {
		BootstrapperPluginLoader loader(bootstrapper);
		loader.loadAll();
		return loader.modules();
	}
}}
