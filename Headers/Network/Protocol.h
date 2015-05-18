/*!
 * \file Protocols.h
 *
 * \author Clément Bossut
 * \author Théo de la Hogue
 *
 * This code is licensed under the terms of the "CeCILL-C"
 * http://www.cecill.info
 */

#ifndef PROTOCOLS_H_
#define PROTOCOLS_H_

#include <string>
#include <vector>

namespace OSSIA {

class Protocol {
public:
  virtual ~Protocol() = default;
};


struct Local : public Protocol {};


struct Minuit : public Protocol {
  Minuit(std::string ip, int in_port, int out_port)
    :ip(ip), in_port(in_port), out_port(out_port) {} //TODO what if only in or out ?

  std::string ip;
  int in_port;
  int out_port;
};


struct OSC : public Protocol {
  OSC(std::string ip, int in_port, int out_port)
    :ip(ip), in_port(in_port), out_port(out_port) {} //TODO what if only in or out ?

  std::string ip;
  int in_port;
  int out_port;
};

struct Midi : public Protocol {
  // to see IPs of connected Midi devices
  static std::vector<Midi> scan();//TODO options
  //TODO members
};

}

#endif /* PROTOCOLS_H_ */
