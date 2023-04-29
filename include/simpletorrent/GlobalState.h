#pragma once

namespace simpletorrent {

class GlobalState {
 public:
  static bool is_stop_download();
  static void set_stop_download();

  GlobalState() = delete;
  GlobalState(const GlobalState&) = delete;
  void operator=(const GlobalState&) = delete;

 private:
  static bool stop_download_;
};

}  // namespace simpletorrent