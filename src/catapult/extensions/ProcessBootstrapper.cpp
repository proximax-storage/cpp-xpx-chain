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

#include "ProcessBootstrapper.h"
#include "PluginUtils.h"
#include "catapult/plugins/PluginExceptions.h"
#include <boost/exception_ptr.hpp>

namespace catapult { namespace extensions {

	ProcessBootstrapper::ProcessBootstrapper(
		const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
			const std::string& resourcesPath,
			ProcessDisposition disposition,
			const std::string& servicePoolName)
			: m_pConfigHolder(pConfigHolder)
			, m_resourcesPath(resourcesPath)
			, m_disposition(disposition)
			, m_pMultiServicePool(std::make_unique<thread::MultiServicePool>(
					servicePoolName,
					thread::MultiServicePool::DefaultPoolConcurrency(),
					m_pConfigHolder->Config().Node.ShouldUseSingleThreadPool
							? thread::MultiServicePool::IsolatedPoolMode::Disabled
							: thread::MultiServicePool::IsolatedPoolMode::Enabled))
			, m_subscriptionManager(m_pConfigHolder->Config())
			, m_pluginManager(m_pConfigHolder, CreateStorageConfiguration(m_pConfigHolder->Config()))
	{}

	const config::BlockchainConfiguration& ProcessBootstrapper::config(const Height& height) const {
		return m_pConfigHolder->Config(height);
	}

	const config::BlockchainConfiguration& ProcessBootstrapper::config() const {
		return m_pConfigHolder->Config();
	}

	const std::shared_ptr<config::BlockchainConfigurationHolder>& ProcessBootstrapper::configHolder() const {
		return m_pConfigHolder;
	}

	const std::string& ProcessBootstrapper::resourcesPath() const {
		return m_resourcesPath;
	}

	ProcessDisposition ProcessBootstrapper::disposition() const {
		return m_disposition;
	}

	const std::vector<ionet::Node>& ProcessBootstrapper::staticNodes() const {
		return m_nodes;
	}

	thread::MultiServicePool& ProcessBootstrapper::pool() {
		return *m_pMultiServicePool;
	}

	ExtensionManager& ProcessBootstrapper::extensionManager() {
		return m_extensionManager;
	}

	subscribers::SubscriptionManager& ProcessBootstrapper::subscriptionManager() {
		return m_subscriptionManager;
	}

	plugins::PluginManager& ProcessBootstrapper::pluginManager() {
		return m_pluginManager;
	}

	CacheHolder& ProcessBootstrapper::cacheHolder() {
		return m_cacheHolder;
	}

	const CacheHolder& ProcessBootstrapper::cacheHolder() const {
		return m_cacheHolder;
	}

	namespace {
		using RegisterExtensionFunc = void (*)(ProcessBootstrapper&);

		void LoadExtension(const plugins::PluginModule& module, ProcessBootstrapper& bootstrapper) {
			auto registerExtension = module.symbol<RegisterExtensionFunc>("RegisterExtension");

			try {
				registerExtension(bootstrapper);
			} catch (...) {
				// since the module will be unloaded after this function exits, throw a copy of the exception that
				// is not dependent on the (soon to be unloaded) module
				auto exInfo = boost::diagnostic_information(boost::current_exception());
				CATAPULT_THROW_AND_LOG_0(plugins::plugin_load_error, exInfo.c_str());
			}
		}
	}

	void ProcessBootstrapper::loadExtensions() {
		for (const auto& extension : config().Extensions.Names) {
			m_extensionModules.emplace_back(config().User.PluginsDirectory, extension);

			CATAPULT_LOG(info) << "registering dynamic extension " << extension;
			LoadExtension(m_extensionModules.back(), *this);
		}
	}

	void ProcessBootstrapper::addStaticNodes(const std::vector<ionet::Node>& nodes) {
		m_nodes.insert(m_nodes.end(), nodes.cbegin(), nodes.cend());
	}

	void ProcessBootstrapper::addConfigValidator(ConfigValidatorFunc validator) {
		m_configValidators.push_back(validator);
	}

	void ProcessBootstrapper::validateConfig(const config::BlockchainConfiguration& config) {
		for (const auto& validator: m_configValidators)
			validator(config);

		m_configValidators.clear();
	}

	void AddStaticNodesFromPath(ProcessBootstrapper& bootstrapper, const std::string& path) {
		auto nodes = config::LoadPeersFromPath(path, bootstrapper.config().Immutable.NetworkIdentifier, false);
		bootstrapper.addStaticNodes(nodes);
	}
}}
