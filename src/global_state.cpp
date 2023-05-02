#include "simpletorrent/GlobalState.h"

namespace simpletorrent {

bool GlobalState::stop_download_ = false;

bool GlobalState::is_stop_download() { return stop_download_; }

void GlobalState::set_continue_download() { stop_download_ = false; }

void GlobalState::set_stop_download() { stop_download_ = true; }
}  // namespace simpletorrent