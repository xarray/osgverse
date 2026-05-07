#include "gf/io/registry.h"

#include "gf/io/ksplat.h"
#include "gf/io/ply.h"
#include "gf/io/ply_auto.h"
#include "gf/io/ply_compressed.h"
#include "gf/io/splat.h"
#include "gf/io/spz.h"
#include "gf/io/sog.h"

namespace gf {


namespace {
std::string NormalizeExt(const std::string &ext) {
  if (!ext.empty() && ext[0] == '.')
    return ext.substr(1);
  return ext;
}
} // namespace

IORegistry::IORegistry() {
  // Built-in registrations
  RegisterReader({"spz"}, gf::MakeSpzReader());
  RegisterWriter({"spz"}, gf::MakeSpzWriter());
  RegisterReader({"ply"}, gf::MakePlyAutoReader());
  RegisterWriter({"ply"}, gf::MakePlyWriter());
  RegisterReader({"compressed.ply"}, gf::MakePlyCompressedReader());
  RegisterWriter({"compressed.ply"}, gf::MakePlyCompressedWriter());
  RegisterReader({"splat"}, gf::MakeSplatReader());
  RegisterWriter({"splat"}, gf::MakeSplatWriter());
  RegisterReader({"ksplat"}, gf::MakeKsplatReader());
  RegisterWriter({"ksplat"}, gf::MakeKsplatWriter());
  RegisterReader({"sog"}, gf::MakeSogReader());
  RegisterWriter({"sog"}, gf::MakeSogWriter());
}


void IORegistry::RegisterReader(const std::vector<std::string> &exts,
                                std::unique_ptr<IGaussReader> reader) {
  reader_store_.push_back(std::move(reader));
  IGaussReader *ptr = reader_store_.back().get();
  for (const auto &e : exts) {
    readers_[NormalizeExt(e)] = ptr;
  }
}

void IORegistry::RegisterWriter(const std::vector<std::string> &exts,
                                std::unique_ptr<IGaussWriter> writer) {
  writer_store_.push_back(std::move(writer));
  IGaussWriter *ptr = writer_store_.back().get();
  for (const auto &e : exts) {
    writers_[NormalizeExt(e)] = ptr;
  }
}

IGaussReader *IORegistry::ReaderForExt(const std::string &ext) const {
  const auto it = readers_.find(NormalizeExt(ext));
  return it == readers_.end() ? nullptr : it->second;
}

IGaussWriter *IORegistry::WriterForExt(const std::string &ext) const {
  const auto it = writers_.find(NormalizeExt(ext));
  return it == writers_.end() ? nullptr : it->second;
}

} // namespace gf
