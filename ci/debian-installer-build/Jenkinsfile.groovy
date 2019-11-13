pipeline {
    agent {
        label 'catapult-build-node'
    }

    // using the Timestamper plugin we can add timestamps to the console log
    options {
        timestamps()
    }

    environment {
        DOCKER_REGISTRY = "249767383774.dkr.ecr.ap-southeast-1.amazonaws.com"
        CREDENTIAL_ID = "ecr:ap-southeast-1:jenkins-ecr"
        IMAGE = "proximax-catapult-server-toolless"
        BUILD_IMAGE = "catapult-server-build-image:1.0"
    }

    stages {
        stage('Build') {
            steps {
                echo 'Building catapult-server inside a docker'
                script {
                    def buildImage = docker.image("${BUILD_IMAGE}")
                    docker.withRegistry("https://${DOCKER_REGISTRY}", "${CREDENTIAL_ID}") {
                        buildImage.inside {
                            sh """
                                echo 'Building catapult-server'
                                rm -rf _build
                                mkdir _build 
                                cd _build
                                cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-pthread" -DPYTHON_EXECUTABLE=/usr/bin/python3 -DBSONCXX_LIB=/usr/local/lib/libbsoncxx.so -DMONGOCXX_LIB=/usr/local/lib/libmongocxx.so .. 
                                    make -j4 publish
                                make -j4
                                cd ..
                                ./scripts/release-script/copyDeps.sh _build/bin/ ./deps
                                rm -rf tools/*
                            """
                        }
                    }
                }
            }
        }


        stage('Build and Publish Release Image') {

            steps {
                echo 'Build and Publish Image'
                script {
                    def newImage = docker.build("${IMAGE}")
                    docker.withRegistry("https://${DOCKER_REGISTRY}", "${CREDENTIAL_ID}") {
                        newImage.push("${env.GIT_BRANCH}")
                        // if a tag commit, then env.GIT_BRANCH returns the tag name instead of a branch
                    }
                }
            }

        }

    }