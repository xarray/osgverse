#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "gf/io/reader.h"
#include "gf/io/writer.h"

namespace gf {

class IORegistry {
public:
  IORegistry();

  void RegisterReader(const std::vector<std::string> &exts,
                      std::unique_ptr<IGaussReader> reader);
  void RegisterWriter(const std::vector<std::string> &exts,
                      std::unique_ptr<IGaussWriter> writer);

  IGaussReader *ReaderForExt(const std::string &ext) const;
  IGaussWriter *WriterForExt(const std::string &ext) const;

private:
  std::vector<std::unique_ptr<IGaussReader>> reader_store_;
  std::vector<std::unique_ptr<IGaussWriter>> writer_store_;
  std::unordered_map<std::string, IGaussReader *> readers_;
  std::unordered_map<std::string, IGaussWriter *> writers_;
};

} // namespace gf
