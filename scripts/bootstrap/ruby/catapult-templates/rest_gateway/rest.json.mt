{
  "network": {
    "name": "mijinTest",
    "description": "catapult development network"
  },

  "port": 3000,
  "crossDomainHttpMethods": ["GET", "POST", "PUT", "OPTIONS"],
  "clientPrivateKey": "{{rest_gateway_private_key}}",
  "extensions": ["aggregate", "lock", "multisig", "namespace", "reputation", "transfer"],
  "db": {
    "url": "mongodb://{{mongo_host}}:27017/",
    "name": "catapult",
    "pageSizeMin": 10,
    "pageSizeMax": 100,
    "pageSizeStep": 25,
    "maxConnectionAttempts": 5,
    "baseRetryDelay": 500
  },

  "apiNode": {
    "host": "{{api_node_host}}",
    "port": 7900,
    "publicKey": "{{api_node_public_key}}"
  },

  "websocket": {
    "mq": {
      "host": "{{api_node_host}}",
      "port": 7901,
      "monitorInterval": 500,
      "connectTimeout": 10000,
      "monitorLoggingThrottle": 60000
    },
    "allowOptionalAddress": true
  },

  "logging": {
    "console": {
      "colorize": true,
      "level": "verbose",
      "handleExceptions": true,
      "prettyPrint": true,
      "timestamp": true
    },
    "file": {
      "level": "verbose",
      "handleExceptions": true,
      "prettyPrint": true,
      "timestamp": true,
      "filename": "catapult-rest.log",
      "maxsize": 20971520,
      "maxFiles": 100
    }
  }
}
