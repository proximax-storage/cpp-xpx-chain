rm -R _build
mkdir _build
cd _build

cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-pthread" ..

make publish

# First we build extensions which are required by tools
make \
	catapult.plugins.namespace \
	catapult.plugins.transfer \
	-j4

# Then we build all tools
make \
	catapult.tools.address \
	catapult.tools.health \
	catapult.tools.nemgen \
	catapult.tools.network \
	-j4
cd ..

# Now we want to create a docker image,
# so we need to create it with shared libs which is required by tools
mkdir ./temp
# We copy all libs to temp folder
./scripts/release-script/copyDeps.sh ./_build/bin/ ./temp
