#pragma once

namespace simpletorrent {

namespace globalstate {
bool is_stop_download();
void set_stop_download();
void set_continue_download();
}  // namespace globalstate

}  // namespace simpletorrent