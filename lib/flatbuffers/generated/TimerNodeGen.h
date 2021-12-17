// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_TIMERNODE_RLOGIC_SERIALIZATION_H_
#define FLATBUFFERS_GENERATED_TIMERNODE_RLOGIC_SERIALIZATION_H_

#include "flatbuffers/flatbuffers.h"

#include "PropertyGen.h"

namespace rlogic_serialization {

struct TimerNode;
struct TimerNodeBuilder;

struct TimerNode FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef TimerNodeBuilder Builder;
  struct Traits;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_NAME = 4,
    VT_ID = 6,
    VT_ROOTINPUT = 8,
    VT_ROOTOUTPUT = 10
  };
  const flatbuffers::String *name() const {
    return GetPointer<const flatbuffers::String *>(VT_NAME);
  }
  uint64_t id() const {
    return GetField<uint64_t>(VT_ID, 0);
  }
  const rlogic_serialization::Property *rootInput() const {
    return GetPointer<const rlogic_serialization::Property *>(VT_ROOTINPUT);
  }
  const rlogic_serialization::Property *rootOutput() const {
    return GetPointer<const rlogic_serialization::Property *>(VT_ROOTOUTPUT);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_NAME) &&
           verifier.VerifyString(name()) &&
           VerifyField<uint64_t>(verifier, VT_ID) &&
           VerifyOffset(verifier, VT_ROOTINPUT) &&
           verifier.VerifyTable(rootInput()) &&
           VerifyOffset(verifier, VT_ROOTOUTPUT) &&
           verifier.VerifyTable(rootOutput()) &&
           verifier.EndTable();
  }
};

struct TimerNodeBuilder {
  typedef TimerNode Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_name(flatbuffers::Offset<flatbuffers::String> name) {
    fbb_.AddOffset(TimerNode::VT_NAME, name);
  }
  void add_id(uint64_t id) {
    fbb_.AddElement<uint64_t>(TimerNode::VT_ID, id, 0);
  }
  void add_rootInput(flatbuffers::Offset<rlogic_serialization::Property> rootInput) {
    fbb_.AddOffset(TimerNode::VT_ROOTINPUT, rootInput);
  }
  void add_rootOutput(flatbuffers::Offset<rlogic_serialization::Property> rootOutput) {
    fbb_.AddOffset(TimerNode::VT_ROOTOUTPUT, rootOutput);
  }
  explicit TimerNodeBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  TimerNodeBuilder &operator=(const TimerNodeBuilder &);
  flatbuffers::Offset<TimerNode> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<TimerNode>(end);
    return o;
  }
};

inline flatbuffers::Offset<TimerNode> CreateTimerNode(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::String> name = 0,
    uint64_t id = 0,
    flatbuffers::Offset<rlogic_serialization::Property> rootInput = 0,
    flatbuffers::Offset<rlogic_serialization::Property> rootOutput = 0) {
  TimerNodeBuilder builder_(_fbb);
  builder_.add_id(id);
  builder_.add_rootOutput(rootOutput);
  builder_.add_rootInput(rootInput);
  builder_.add_name(name);
  return builder_.Finish();
}

struct TimerNode::Traits {
  using type = TimerNode;
  static auto constexpr Create = CreateTimerNode;
};

inline flatbuffers::Offset<TimerNode> CreateTimerNodeDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const char *name = nullptr,
    uint64_t id = 0,
    flatbuffers::Offset<rlogic_serialization::Property> rootInput = 0,
    flatbuffers::Offset<rlogic_serialization::Property> rootOutput = 0) {
  auto name__ = name ? _fbb.CreateString(name) : 0;
  return rlogic_serialization::CreateTimerNode(
      _fbb,
      name__,
      id,
      rootInput,
      rootOutput);
}

}  // namespace rlogic_serialization

#endif  // FLATBUFFERS_GENERATED_TIMERNODE_RLOGIC_SERIALIZATION_H_