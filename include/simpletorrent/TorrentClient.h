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
  TorrentClient(const std::string& torrent_file,
                const std::string& output_file);

  void start_download();

  void stop_download();

 private:
  void parse_torrent_file(const std::string& torrent_file);

  void connect_to_tracker();

  void connect_to_peers();

  Tracker tracker_;

  PieceManager piece_manager_;

  // A map of peer ID to Peer objects, representing the connected peers
  std::unordered_map<int, Peer> peers_;

  std::mutex peers_mutex_;

  std::atomic<DownloadState> download_state_;
};

enum class DownloadState { ACTIVE, PAUSED, STOPPED };

}  // namespace simpletorrent