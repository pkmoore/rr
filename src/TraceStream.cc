/* -*- Mode: C++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil; -*- */

//#define DEBUGTAG "Trace"

#include "TraceStream.h"

#include <inttypes.h>
#include <sysexits.h>

#include <fstream>
#include <string>
#include <sstream>

#include "log.h"
#include "util.h"

using namespace std;

//
// This represents the format and layout of recorded traces.  This
// version number doesn't track the rr version number, because changes
// to the trace format will be rare.
//
// NB: if you *do* change the trace format for whatever reason, you
// MUST increment this version number.  Otherwise users' old traces
// will become unreplayable and they won't know why.
//
#define TRACE_VERSION 18

static string default_rr_trace_dir() { return string(getenv("HOME")) + "/.rr"; }

static string trace_save_dir() {
  const char* output_dir = getenv("_RR_TRACE_DIR");
  return output_dir ? output_dir : default_rr_trace_dir();
}

static string latest_trace_symlink() {
  return trace_save_dir() + "/latest-trace";
}

/**
 * Create the default ~/.rr directory if it doesn't already exist.
 */
static void ensure_default_rr_trace_dir() {
  string dir = default_rr_trace_dir();
  struct stat st;
  if (0 == stat(dir.c_str(), &st)) {
    if (!(S_IFDIR & st.st_mode)) {
      FATAL() << "`" << dir << "' exists but isn't a directory.";
    }
    if (access(dir.c_str(), W_OK)) {
      FATAL() << "Can't write to `" << dir << "'.";
    }
    return;
  }
  int ret = mkdir(dir.c_str(), S_IRWXU | S_IRWXG);
  int err = errno;
  // Another rr process can be concurrently attempting to create
  // ~/.rr, so the directory may have come into existence since
  // we checked above.
  if (ret && EEXIST != err) {
    FATAL() << "Failed to create directory `" << dir << "'";
  }
}

bool TraceWriter::good() const {
  return events.good() && data.good() && data_header.good() && mmaps.good() &&
         tasks.good();
}

bool TraceReader::good() const {
  return events.good() && data.good() && data_header.good() && mmaps.good() &&
         tasks.good();
}

void TraceWriter::write_frame(const TraceFrame& frame) {
  events.write(&frame.basic_info, sizeof(frame.basic_info));
  if (!events.good()) {
    FATAL() << "Tried to save " << sizeof(frame.basic_info)
            << " bytes to the trace, but failed";
  }
  // TODO: only store exec info for non-async-sig events when
  // debugging assertions are enabled.
  if (frame.event().has_exec_info == HAS_EXEC_INFO) {
    events.write(&frame.exec_info, sizeof(frame.exec_info));
    if (!events.good()) {
      FATAL() << "Tried to save " << sizeof(frame.exec_info)
              << " bytes to the trace, but failed";
    }

    int extra_reg_bytes = frame.extra_regs().data_size();
    char extra_reg_format = (char)frame.extra_regs().format();
    events.write(&extra_reg_format, sizeof(extra_reg_format));
    events.write((char*)&extra_reg_bytes, sizeof(extra_reg_bytes));
    if (!events.good()) {
      FATAL() << "Tried to save "
              << sizeof(extra_reg_bytes) + sizeof(extra_reg_format)
              << " bytes to the trace, but failed";
    }
    if (extra_reg_bytes > 0) {
      events.write((const char*)frame.extra_regs().data_bytes(),
                   extra_reg_bytes);
      if (!events.good()) {
        FATAL() << "Tried to save " << extra_reg_bytes
                << " bytes to the trace, but failed";
      }
    }
  }

  tick_time();
}

TraceFrame TraceReader::read_frame() {
  // Read the common event info first, to see if we also have
  // exec info to read.
  TraceFrame frame;
  events.read(&frame.basic_info, sizeof(frame.basic_info));
  if (frame.event().has_exec_info) {
    events.read(&frame.exec_info, sizeof(frame.exec_info));

    int extra_reg_bytes;
    char extra_reg_format;
    events.read(&extra_reg_format, sizeof(extra_reg_format));
    events.read((char*)&extra_reg_bytes, sizeof(extra_reg_bytes));
    if (extra_reg_bytes > 0) {
      std::vector<uint8_t> data;
      data.resize(extra_reg_bytes);
      events.read((char*)data.data(), extra_reg_bytes);
      frame.recorded_extra_regs.set_to_raw_data(
          (ExtraRegisters::Format)extra_reg_format, data);
    } else {
      assert(extra_reg_format == ExtraRegisters::NONE);
      frame.recorded_extra_regs = ExtraRegisters();
    }
  }

  tick_time();
  assert(time() == frame.time());
  return frame;
}

template <typename T>
static CompressedWriter& operator<<(CompressedWriter& out, const T& value) {
  out.write(&value, sizeof(value));
  return out;
}

template <typename T>
static CompressedReader& operator>>(CompressedReader& in, T& value) {
  in.read(&value, sizeof(value));
  return in;
}

static CompressedWriter& operator<<(CompressedWriter& out,
                                    const string& value) {
  out.write(value.c_str(), value.size() + 1);
  return out;
}

static CompressedReader& operator>>(CompressedReader& in, string& value) {
  value.empty();
  while (true) {
    char ch;
    in.read(&ch, 1);
    if (ch == 0) {
      break;
    }
    value.append(1, ch);
  }
  return in;
}

template <typename T>
static CompressedWriter& operator<<(CompressedWriter& out,
                                    const std::vector<T>& value) {
  out << value.size();
  for (auto& i : value) {
    out << i;
  }
  return out;
}

template <typename T>
static CompressedReader& operator>>(CompressedReader& in,
                                    std::vector<T>& value) {
  size_t len;
  in >> len;
  value.resize(0);
  for (size_t i = 0; i < len; ++i) {
    T v;
    in >> v;
    value.push_back(v);
  }
  return in;
}

void TraceWriter::write_task_event(const TraceTaskEvent& event) {
  tasks << event.type() << event.tid();
  switch (event.type()) {
    case TraceTaskEvent::CLONE:
      tasks << event.parent_tid() << event.clone_flags();
      break;
    case TraceTaskEvent::EXEC:
      tasks << event.file_name() << event.cmd_line();
      break;
    case TraceTaskEvent::EXIT:
      break;
    case TraceTaskEvent::NONE:
      assert(0 && "Writing NONE TraceTaskEvent");
      break;
  }
}

TraceTaskEvent TraceReader::read_task_event() {
  TraceTaskEvent r;
  tasks >> r.type_ >> r.tid_;
  switch (r.type()) {
    case TraceTaskEvent::CLONE:
      tasks >> r.parent_tid_ >> r.clone_flags_;
      break;
    case TraceTaskEvent::EXEC:
      tasks >> r.file_name_ >> r.cmd_line_;
      break;
    case TraceTaskEvent::EXIT:
      break;
    case TraceTaskEvent::NONE:
      // Should be EOF only
      assert(!tasks.good());
      break;
  }
  return r;
}

string TraceWriter::try_hardlink_file(const string& file_name) {
  char link_name[] = "mmap_XXXXXXXXX_hardlink";
  sprintf(link_name, "mmap_%d_hardlink", mmap_count);

  size_t last_slash = file_name.rfind('/');
  string basename = (last_slash != file_name.npos)
                        ? file_name.substr(last_slash + 1)
                        : file_name;

  string link_path = dir() + "/" + link_name + "_" + basename;
  int ret = link(file_name.c_str(), link_path.c_str());
  if (ret < 0) {
    // maybe tried to link across filesystems?
    return file_name;
  }
  return link_path;
}

TraceWriter::RecordInTrace TraceWriter::write_mapped_region(
    const TraceMappedRegion& map, int prot, int flags) {
  TraceReader::MappedDataSource source;
  string backing_file_name;
  if (map.stat().st_ino == 0) {
    source = TraceReader::SOURCE_ZERO;
  } else {
    if (should_copy_mmap_region(map.file_name(), &map.stat(), prot, flags)) {
      source = TraceReader::SOURCE_TRACE;
    } else {
      source = TraceReader::SOURCE_FILE;
      // Try hardlinking file into the trace directory. This will avoid
      // replay failures if the original file is deleted or replaced (but not
      // if it is overwritten in-place). If try_hardlink_file fails it
      // just returns the original file name.
      // A relative backing_file_name is relative to the trace directory.
      backing_file_name = try_hardlink_file(map.file_name());
    }
  }
  mmaps << source << map.file_name() << map.stat() << map.start() << map.end()
        << map.offset_pages() << backing_file_name;
  ++mmap_count;
  return source == TraceReader::SOURCE_TRACE ? RECORD_IN_TRACE
                                             : DONT_RECORD_IN_TRACE;
}

static void verify_backing_file(const TraceMappedRegion& map,
                                const string& backing_file_name) {
  struct stat backing_stat;
  if (stat(backing_file_name.c_str(), &backing_stat)) {
    FATAL() << "Failed to stat " << backing_file_name
            << ": replay is impossible";
  }
  if (backing_stat.st_ino != map.stat().st_ino ||
      backing_stat.st_mode != map.stat().st_mode ||
      backing_stat.st_uid != map.stat().st_uid ||
      backing_stat.st_gid != map.stat().st_gid ||
      backing_stat.st_size != map.stat().st_size ||
      backing_stat.st_mtime != map.stat().st_mtime) {
    LOG(error)
        << "Metadata of " << map.file_name()
        << " changed: replay divergence likely, but continuing anyway ...";
  }
}

TraceMappedRegion TraceReader::read_mapped_region(MappedData* data) {
  TraceMappedRegion map;
  string backing_file_name;
  mmaps >> data->source >> map.filename >> map.stat_ >> map.start_ >>
      map.end_ >> map.file_offset_pages >> backing_file_name;
  if (data->source == SOURCE_FILE) {
    if (backing_file_name[0] != '/') {
      backing_file_name = dir() + "/" + backing_file_name;
    }
    data->file_name = backing_file_name;
    data->file_data_offset_pages = map.file_offset_pages;
    verify_backing_file(map, backing_file_name);
  }
  return map;
}

static ostream& operator<<(ostream& out, const vector<string>& vs) {
  out << vs.size() << endl;
  for (auto& v : vs) {
    out << v << '\0';
  }
  return out;
}

static istream& operator>>(istream& in, vector<string>& vs) {
  size_t len;
  in >> len;
  in.ignore(1);
  for (size_t i = 0; i < len; ++i) {
    char buf[PATH_MAX];
    in.getline(buf, sizeof(buf), '\0');
    vs.push_back(buf);
  }
  return in;
}

void TraceWriter::write_raw(const void* d, size_t len, remote_ptr<void> addr) {
  data_header << global_time << addr.as_int() << len;
  data.write(d, len);
}

TraceReader::RawData TraceReader::read_raw_data() {
  TraceFrame::Time time;
  RawData d;
  size_t num_bytes;
  data_header >> time >> d.addr >> num_bytes;
  assert(time == global_time);
  d.data.resize(num_bytes);
  data.read((char*)d.data.data(), num_bytes);
  return d;
}

bool TraceReader::read_raw_data_for_frame(const TraceFrame& frame, RawData& d) {
  while (!data_header.at_end()) {
    TraceFrame::Time time;
    data_header.save_state();
    data_header >> time;
    data_header.restore_state();
    if (time == frame.time()) {
      d = read_raw_data();
      return true;
    }
    if (time > frame.time()) {
      return false;
    }
  }
  return false;
}

void TraceWriter::close() {
  events.close();
  data.close();
  data_header.close();
  mmaps.close();
  tasks.close();
}

static string make_trace_dir(const string& exe_path) {
  ensure_default_rr_trace_dir();

  // Find a unique trace directory name.
  int nonce = 0;
  int ret;
  string dir;
  do {
    stringstream ss;
    ss << trace_save_dir() << "/" << basename(exe_path.c_str()) << "-"
       << nonce++;
    dir = ss.str();
    ret = mkdir(dir.c_str(), S_IRWXU | S_IRWXG);
  } while (ret && EEXIST == errno);

  if (ret) {
    FATAL() << "Unable to create trace directory `" << dir << "'";
  }

  return dir;
}

TraceWriter::TraceWriter(const std::vector<std::string>& argv,
                         const std::vector<std::string>& envp,
                         const string& cwd, int bind_to_cpu)
    : TraceStream(make_trace_dir(argv[0]),
                  // Somewhat arbitrarily start the
                  // global time from 1.
                  1),
      events(events_path(), 1024 * 1024, 1),
      data(data_path(), 8 * 1024 * 1024, 3),
      data_header(data_header_path(), 1024 * 1024, 1),
      mmaps(mmaps_path(), 64 * 1024, 1),
      tasks(tasks_path(), 64 * 1024, 1) {
  this->argv = argv;
  this->envp = envp;
  this->cwd = cwd;
  this->bind_to_cpu = bind_to_cpu;

  string ver_path = version_path();
  fstream version(ver_path.c_str(), fstream::out);
  if (!version.good()) {
    FATAL() << "Unable to create " << ver_path;
  }
  version << TRACE_VERSION << endl;

  string link_name = latest_trace_symlink();
  // Try to update the symlink to |this|.  We only try attempt
  // to set the symlink once.  If the link is re-created after
  // we |unlink()| it, then another rr process is racing with us
  // and it "won".  The link is then valid and points at some
  // very-recent trace, so that's good enough.
  unlink(link_name.c_str());
  int ret = symlink(trace_dir.c_str(), link_name.c_str());
  if (ret < 0 && errno != EEXIST) {
    FATAL() << "Failed to update symlink `" << link_name << "' to `"
            << trace_dir << "'.";
  }

  if (!probably_not_interactive(STDOUT_FILENO)) {
    printf("rr: Saving the execution of `%s' to trace directory `%s'.\n",
           argv[0].c_str(), trace_dir.c_str());
  }

  ofstream out(args_env_path());
  out << cwd << '\0';
  out << argv;
  out << envp;
  out << bind_to_cpu;
  assert(out.good());
}

TraceFrame TraceReader::peek_frame() {
  events.save_state();
  auto saved_time = global_time;
  TraceFrame frame;
  if (!at_end()) {
    frame = read_frame();
  }
  events.restore_state();
  global_time = saved_time;
  return frame;
}

TraceFrame TraceReader::peek_to(pid_t pid, EventType type,
                                SyscallEntryOrExit state) {
  TraceFrame frame;
  events.save_state();
  auto saved_time = global_time;
  while (good() && !at_end()) {
    frame = read_frame();
    if (frame.tid() == pid && frame.event().type == type &&
        frame.event().state == state) {
      events.restore_state();
      global_time = saved_time;
      return frame;
    }
  }
  FATAL() << "Unable to find requested frame in stream";
  // Unreachable
  return frame;
}

void TraceReader::rewind() {
  events.rewind();
  data.rewind();
  data_header.rewind();
  mmaps.rewind();
  tasks.rewind();
  global_time = 0;
  assert(good());
}

TraceReader::TraceReader(const string& dir)
    : TraceStream(dir.empty() ? latest_trace_symlink() : dir,
                  // Initialize the global time at 0, so
                  // that when we tick it when reading
                  // the first trace, it matches the
                  // initial global time at recording, 1.
                  0),
      events(events_path()),
      data(data_path()),
      data_header(data_header_path()),
      mmaps(mmaps_path()),
      tasks(tasks_path()) {
  string path = version_path();
  fstream vfile(path.c_str(), fstream::in);
  if (!vfile.good()) {
    fprintf(
        stderr,
        "\n"
        "rr: error: Version file for recorded trace `%s' not found.  Did you "
        "record\n"
        "           `%s' with an older version of rr?  If so, you'll need to "
        "replay\n"
        "           `%s' with that older version.  Otherwise, your trace is\n"
        "           likely corrupted.\n"
        "\n",
        path.c_str(), path.c_str(), path.c_str());
    exit(EX_DATAERR);
  }
  int version = 0;
  vfile >> version;
  if (vfile.fail() || TRACE_VERSION != version) {
    fprintf(stderr, "\n"
                    "rr: error: Recorded trace `%s' has an incompatible "
                    "version %d; expected\n"
                    "           %d.  Did you record `%s' with an older version "
                    "of rr?  If so,\n"
                    "           you'll need to replay `%s' with that older "
                    "version.  Otherwise,\n"
                    "           your trace is likely corrupted.\n"
                    "\n",
            path.c_str(), version, TRACE_VERSION, path.c_str(), path.c_str());
    exit(EX_DATAERR);
  }

  ifstream in(args_env_path());
  assert(in.good());
  char buf[PATH_MAX];
  in.getline(buf, sizeof(buf), '\0');
  cwd = buf;
  in >> argv;
  in >> envp;
  in >> bind_to_cpu;
}

uint64_t TraceReader::uncompressed_bytes() const {
  return events.uncompressed_bytes() + data.uncompressed_bytes() +
         data_header.uncompressed_bytes() + mmaps.uncompressed_bytes() +
         tasks.uncompressed_bytes();
}

uint64_t TraceReader::compressed_bytes() const {
  return events.compressed_bytes() + data.compressed_bytes() +
         data_header.compressed_bytes() + mmaps.compressed_bytes() +
         tasks.uncompressed_bytes();
}