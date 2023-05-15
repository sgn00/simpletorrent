# simpletorrent
simpletorrent is a simple torrent client written in C++. It supports downloading (no uploading) of torrent files, including multi-file torrents. This project was meant for me to learn more about the bittorrent protocol as well as experiment with modern C++ features and design. For detailed features of the implementation, refer to [Technical Design](TechnicalDesign.md).

Resources referenced while writing simpletorrent include:
* [Building a BitTorrent client from the ground up in Go](https://blog.jse.li/posts/torrent/)
* https://github.com/ss16118/torrent-client-cpp

## Quick Start
simpletorrent requires a C++20 compiler and CMake >=3.16 to build, and is written to be used on a Linux platform.  
simpletorrent has OpenSSL dependencies so install the `libssl-dev` package:
`sudo apt-get install libssl-dev`
1. Clone the repository
2. `mkdir build_release`
3. `cd build_release`
4. `cmake -DCMAKE_BUILD_TYPE=Release ..`
5. `./simpletorrent <path to torrent file>`

