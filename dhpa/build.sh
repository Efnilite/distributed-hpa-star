cd master

cmake -B build/ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS="-g3 -O0"
make -C build -j -s

cd ../worker

cmake -B build/ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS="-g3 -O0"
make -C build -j -s