#ifndef NET_BLOCKS_encryption_MODULE_H
#define NET_BLOCKS_encryption_MODULE_H

#include "core/framework.h"

namespace net_blocks {

class encryption_module : public module {
 public:
  void init_module(void);

  module::hook_status hook_send(builder::dyn_var<connection_t*> c, packet_t,
                                builder::dyn_var<char*> buff,
                                builder::dyn_var<unsigned int> len,
                                builder::dyn_var<int*> ret_len);

  module::hook_status hook_ingress(packet_t);

 private:
  encryption_module() = default;

 public:
  static encryption_module instance;
};

}  // namespace net_blocks

#endif
