cd master

cmake -B build/
make -C build -j -s

cd ../worker

cmake -B build/
make -C build -j -s