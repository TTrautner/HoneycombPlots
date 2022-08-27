SET BUILD_ENVIRONMENT="Visual Studio 15 2017 Win64"

rd /s /q build
md build
cd build

cmake .. -G %BUILD_ENVIRONMENT%
cd ..
