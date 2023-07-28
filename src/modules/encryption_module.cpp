#include "modules/encryption_module.h"

#include <string.h>

#include "builder/static_var.h"
#include "modules/identifier_module.h"
#include "modules/network_module.h"
#include "modules/payload_module.h"

namespace net_blocks {
encryption_module encryption_module::instance;

void encryption_module::init_module(void) {
  m_establish_depends = {&identifier_module::instance};
  m_destablish_depends = {&identifier_module::instance};
  m_send_depends = {&payload_module::instance};
  m_ingress_depends = {&network_module::instance};
  framework::instance.register_module(this);
}

module::hook_status encryption_module::hook_send(
    builder::dyn_var<connection_t*> c, packet_t p, builder::dyn_var<char*> buff,
    builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len) {
  runtime::memcpy(net_packet["payload"]->get_addr(p), buff, len);
  return module::hook_status::HOOK_CONTINUE;
}

module::hook_status encryption_module::hook_ingress(packet_t _p) {
  // Calculate correct enc of packet and return HOOK_CONTINUE if correct
  builder::dyn_var<char*> p = runtime::to_void_ptr(_p);
  // Reset the checksum first
  // net_packet["checksum"]->set_integer(p, 0);
  builder::dyn_var<int> so = net_packet["payload"]->get_addr(p);
  builder::dyn_var<int> eo =
      net_packet["total_len"]->get_integer(p) - get_headroom() - 1;

  // Pad the bytes to be multiple of 2
  // if ((eo - so) % 2) {
  //   p[eo] = 0;
  //   eo = eo + 1;
  // }

  // Simple checksum by adding with wrap around
  builder::dyn_var<unsigned short> checksum = 0;
  for (builder::dyn_var<char*> ptr = p + so; ptr < p + eo; ptr = ptr + 2) {
    builder::dyn_var<unsigned short*> pp = runtime::to_void_ptr(ptr);
    pp[0] = pp[0] + 1;
  }

  return module::hook_status::HOOK_CONTINUE;
}

}  // namespace net_blocks
