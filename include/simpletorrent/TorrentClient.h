#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>

#include "Peer.h"
#include "PieceManager.h"
#include "Tracker.h"

namespace simpletorrent {

class TorrentClient {
 public:
  TorrentClient();

  void start_download(const std::string& torrent_file);

 private:
  std::string our_id_;
};

}  // namespace simpletorrent