Building IC Barcode Demo

Requirements:
- The Imaging Source Camera
- QT 5 dev package. Install with sudo apt install qtdeclarative5-dev 
- GStreamer dev packages

Building:
Change into the IC Barcode directory, then enter
mkdir build
cd build
cmake ..
make

After successful build, run in build directory:
./ICBarcode

