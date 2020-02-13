# Pipeline for debian build

Create a separate pipeline to build sirius chain for debian platform.

Pipeline shall be set as multibranch pipeline and set to discover tags.  The pipeline shall filter `develop` branch and `v*-buster` tag

The script path of build configuration shall be set to ci/debian/Jenkinsfile
