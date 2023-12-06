mkdir -p build
cd build
g++ -std=c++14 -shared -fPIC -static-libgcc -static-libstdc++ -I ../Crypto -I ../pybind11/include/ `python3.11 -m pybind11 --includes` ../AsconPy.cpp ../Ascon.cpp ../Classes.cpp ../Crypto/*.cpp -o AsconPy.pyd `python3.11-config --ldflags` -lmingw32 -mwindows -L libwinpthread-1.dll