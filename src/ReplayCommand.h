/* -*- Mode: C++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil; -*- */

#ifndef RR_REPLAY_COMMAND_H_
#define RR_REPLAY_COMMAND_H_

#include "Command.h"

namespace rr {

class ReplayCommand : public Command {
public:
  virtual int run(std::vector<std::string>& args) override;

  static ReplayCommand* get() { return &singleton; }

protected:
  ReplayCommand(const char* name, const char* help) : Command(name, help) {}

  static ReplayCommand singleton;
};

} // namespace rr

void rrdump_dump_state(int event);

void rrdump_write_to_pipe(int ft, int tid);

void rrdump_close_pipe();

#endif // RR_REPLAY_COMMAND_H_
