cd master

cmake -B build/
make -C build -j

cd ../worker

cmake -B build/
make -C build -j