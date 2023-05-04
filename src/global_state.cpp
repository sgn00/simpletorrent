#include "simpletorrent/GlobalState.h"

namespace simpletorrent {

namespace {
bool stop_download_ = false;
}

namespace globalstate {
bool is_stop_download() { return stop_download_; }

void set_stop_download() { stop_download_ = true; }

void set_continue_download() { stop_download_ = false; }
}  // namespace globalstate
}  // namespace simpletorrent