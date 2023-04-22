#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>

#include "Peer.h"
#include "PieceManager.h"
#include "Tracker.h"

namespace simpletorrent {

enum class DownloadState { ACTIVE, PAUSED, STOPPED };

struct TorrentMetadata {
  std::string announce_url;
  std::vector<std::string> piece_hashes;
  long long piece_length;
  long long total_length;
  std::string output_file;
  std::string info_hash;
};

class TorrentClient {
 public:
  TorrentClient();

  void start_download(const std::string& torrent_file);

  void stop_download();

  ~TorrentClient();

 private:
  TorrentMetadata parse_torrent_file(const std::string& torrent_file);

  void connect_to_tracker();

  void connect_to_peers();

  std::string our_id_;

  // A map of peer ID to Peer objects, representing the connected peers
  std::unordered_map<int, Peer> peers_;

  std::mutex peers_mutex_;

  std::atomic<DownloadState> download_state_;
};

class ParseException : public std::runtime_error {
 public:
  explicit ParseException(const std::string& message)
      : std::runtime_error(message) {}
};

}  // namespace simpletorrent