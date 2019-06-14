/* -*- Mode: C++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil; -*- */

#ifndef RR_REPLAY_COMMAND_H_
#define RR_REPLAY_COMMAND_H_

#include <fstream>

#include "Command.h"

#include "ReplayTask.h"

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
static std::ofstream rrdump_pipe;

bool file_exists(const std::string& name);

void rrdump_write_to_pipe(int ft, rr::ReplayTask* t, bool inject);

void rrdump_close_pipe();

#endif // RR_REPLAY_COMMAND_H_
