cd ../vendor
cd fastgltf
mkdir build
cmake -A=x64 -S . -B build 
cd build 
cmake --build . --config Release
cmake --build . --config Debug
cd ..
cd ..
cd SDL2
mkdir build
cmake -A=x64 -S . -B build 
cd build 
cmake --build . --config Release
cmake --build . --config Debug
cd ..
cd ..

cd fmt
mkdir build
cmake -A=x64 -S . -B build 
del run-msbuild.bat
cd build 
cmake --build . --config Release
cmake --build . --config Debug
cd ..
cd ..
