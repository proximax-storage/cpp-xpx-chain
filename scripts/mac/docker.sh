#################
# Docker
#################
https://jenkins.infra.proximax.io/view/Sirius%20Chain/job/cpp-xpx-chain/configure
!!!select_check_box "Update tracking submodules to tip of branch"

#################
# Create git TAG
#################
git tag <tagname>
git push origin --tags

#################
# Update git TAG
#################

git tag -f <tagname>
git push -f --tags


#################
# Build
#################
docker build -t alextsar/dbgchain:v1 -f -f ./scripts/catapult-server-docker/DockerfileDebian .
