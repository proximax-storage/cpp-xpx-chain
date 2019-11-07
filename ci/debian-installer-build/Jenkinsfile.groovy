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
        IMAGE = "proximax-catapult-server"
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
                                cd ..
                              
                            """
                        }
                        echo 'Leaving Container'

                        echo 'Compressing Artifacts'
                        // Creates an XZ compressed archive
                        sh "tar cJfv sirius-installer.tar.xz cpp* "
                    }
                }


            }
        }

        stage('Deploy to Testnet') {

            steps {
                echo 'Deploying to S3 temporary bucket'

                echo 'Managing S3'
                withAWS(credentials: 'jenkins-ecr', region: 'ap-southeast-2') {
                    echo 'Deleting old files'
                    s3Delete bucket: 'debian-installer-sirius', path: './'

                    echo 'Uploading new files'
                    s3Upload bucket: 'debian-installer-sirius', acl: 'PublicRead', file: 'sirius-installer.tar.xz', sseAlgorithm: 'AES256'
//                    s3Upload bucket: 'debian-installer-sirius', acl: 'PublicRead', file: '_build/', sseAlgorithm: 'AES256'
                }


            }
        }
    }
}