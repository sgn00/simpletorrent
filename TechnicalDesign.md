# Technical Design

## Parsing of torrent files
Made use of [bencoding parser](https://github.com/jimporter/bencode.hpp) library. 

## Connecting to UDP and HTTP trackers
Typically, a more robust torrent client will continue fetch peers information from trackers even while downloading. For simplicity, we only connect to our trackers once at program start to fetch peers. simpletorrent supports connecting to both UDP and HTTP trackers. It will connect to HTTP trackers only if there were no successful connections to UDP trackers. In simpletorrrent, UDP connections are implemented asynchronously with `asio` library while HTTP connections are implemented with `CPR` library.

## Downloading pieces from peers
Peer connections are handled asynchronously using `asio` and C++20 coroutines. We do not use multithreading for peers.   Each peer connection once successful, contains 2 loops, one for receiving messages and one for sending messages.  
Each torrent file is split into pieces. Each piece is split into blocks. We request data from peers at the block level, ie. each request to a peer requests for a block. We make pipelined requests to peers (up to 5 in-flight requests) to increase network throughput.

### PieceManager class
The `PieceManager` class manages all things related to the download process. It contains metadata involving which peers have which pieces, which pieces have been completed, the `Buffer` which buffers a fixed number of pieces which are in the process of being downloaded (and have not be written to file).

### Selecting next block
We call `select_next_block(uint32_t peer_id)` in `PieceManager` to determine which block to request from for a given peer.  
`select_next_block(uint32_t peer_id)` first chooses a piece for the peer via a piece selection algorithm. Afterwards we select a block from this piece via a block selection algorithm to determine the block from the piece for the peer to request.
#### Piece selection algorithm:
1. Check if peer has an affinity piece. If a piece is already bound to a piece, then we can just choose that bound piece.
2. If peer has no affinity piece, and buffer is not full, try to add a piece (not yet completed and peer has) to the buffer. Then, we bind this piece to the peer, making it the affinity piece of the peer.
If a peer still does not have a piece assigned to it, then either buffer is full, or peer has no piece to add to buffer.

#### Block selection algorithm
We call `get_block_index_to_retrieve(uint32_t piece_index)` in `Buffer`. This function returns the block to request, given a piece. We do a simple scan of all blocks in the piece and select the first block which has not been requested or downloaded.

### Writing completed pieces to disk
The `add_block(uint32_t peer_id, const Block& block)` function in `PieceManager` is called each time a peer adds downloade block data. This updates various metadata information, and checks if the piece has been completely downloaded.
Once a piece has been completely downloaded, we will verify its hash. If it matches, we will add it to a queue to be written by the writer thread. The writer thread writes the data to file, and will need to calculate the file and file offset to write to for multifile torrents.

## Managing Peer State
As network connections to peers can be unreliable and may drop, we implemented a naive simple reconnection protocol managed by `PeerManager` class. We allow one retry connection to peers, meaning if our initial connection to the peer drops, we will attempt to reconnect once. We automatically disconnect from a peer if we do not receive any messages from the peer after a certain timeout. We do not differentiate between various connection drop types (eg. handshake fail, timeout). `PeerManager` checks every 10 seconds to remove disconnected peers and attempt reconnection or connection to new peers.
