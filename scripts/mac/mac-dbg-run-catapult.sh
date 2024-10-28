#!/bin/bash

osascript -e 'tell application "Terminal" to activate' \
  -e 'tell application "System Events" to tell process "Terminal" to keystroke "n" using command down' \
  -e 'tell application "Terminal" to do script "cd ~/Proj/cpp-xpx-chain/cmake-build-debug/; rm -f data/*/data/server.lock; ./bin/sirius.bc data/api-node-0" in selected tab of the front window'

osascript -e 'tell application "Terminal" to activate' \
  -e 'tell application "System Events" to tell process "Terminal" to keystroke "t" using command down' \
  -e 'tell application "Terminal" to do script "cd ~/Proj/cpp-xpx-chain/cmake-build-debug/; rm -f data/*/data/server.lock; ./bin/sirius.bc data/peer-node-0" in selected tab of the front window'

osascript -e 'tell application "Terminal" to activate' \
  -e 'tell application "System Events" to tell process "Terminal" to keystroke "t" using command down' \
  -e 'tell application "Terminal" to do script "cd ~/Proj/cpp-xpx-chain/cmake-build-debug/; rm -f data/*/data/server.lock; ./bin/sirius.bc data/peer-node-1" in selected tab of the front window'

osascript -e 'tell application "Terminal" to activate' \
  -e 'tell application "System Events" to tell process "Terminal" to keystroke "t" using command down' \
  -e 'tell application "Terminal" to do script "cd ~/Proj/cpp-xpx-chain/cmake-build-debug/; rm -f data/*/data/server.lock; ./bin/sirius.bc data/peer-node-2" in selected tab of the front window'

osascript -e 'tell application "Terminal" to activate' \
  -e 'tell application "System Events" to tell process "Terminal" to keystroke "t" using command down' \
  -e 'tell application "Terminal" to do script "cd ~/Proj/cpp-xpx-chain/cmake-build-debug/; rm -f data/*/data/server.lock; ./bin/sirius.bc data/peer-node-3" in selected tab of the front window'

osascript -e 'tell application "Terminal" to activate' \
  -e 'tell application "System Events" to tell process "Terminal" to keystroke "t" using command down' \
  -e 'tell application "Terminal" to do script "cd ~/Proj/cpp-xpx-chain/cmake-build-debug/; rm -f data/*/data/server.lock; ./bin/sirius.bc data/peer-node-4" in selected tab of the front window'

exit
