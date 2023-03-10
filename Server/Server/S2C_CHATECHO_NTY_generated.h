// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_S2CCHATECHONTY_H_
#define FLATBUFFERS_GENERATED_S2CCHATECHONTY_H_

#include "flatbuffers/flatbuffers.h"

// Ensure the included flatbuffers.h is the same version as when this file was
// generated, otherwise it may not be compatible.
static_assert(FLATBUFFERS_VERSION_MAJOR == 22 &&
              FLATBUFFERS_VERSION_MINOR == 12 &&
              FLATBUFFERS_VERSION_REVISION == 6,
             "Non-compatible flatbuffers version included");

struct S2C_CHATECHO_NTY;
struct S2C_CHATECHO_NTYBuilder;

struct S2C_CHATECHO_NTY FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef S2C_CHATECHO_NTYBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_SIZE = 4,
    VT_CODE = 6,
    VT_USERIDX = 8,
    VT_MSG = 12
  };
  int32_t size() const {
    return GetField<int32_t>(VT_SIZE, 0);
  }
  int32_t code() const {
    return GetField<int32_t>(VT_CODE, 0);
  }
  int32_t userIdx() const {
    return GetField<int32_t>(VT_USERIDX, 0);
  }
  const flatbuffers::String *msg() const {
    return GetPointer<const flatbuffers::String *>(VT_MSG);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int32_t>(verifier, VT_SIZE, 4) &&
           VerifyField<int32_t>(verifier, VT_CODE, 4) &&
           VerifyField<int32_t>(verifier, VT_USERIDX, 4) &&
           VerifyOffset(verifier, VT_MSG) &&
           verifier.VerifyString(msg()) &&
           verifier.EndTable();
  }
};

struct S2C_CHATECHO_NTYBuilder {
  typedef S2C_CHATECHO_NTY Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_size(int32_t size) {
    fbb_.AddElement<int32_t>(S2C_CHATECHO_NTY::VT_SIZE, size, 0);
  }
  void add_code(int32_t code) {
    fbb_.AddElement<int32_t>(S2C_CHATECHO_NTY::VT_CODE, code, 0);
  }
  void add_userIdx(int32_t userIdx) {
    fbb_.AddElement<int32_t>(S2C_CHATECHO_NTY::VT_USERIDX, userIdx, 0);
  }
  void add_msg(flatbuffers::Offset<flatbuffers::String> msg) {
    fbb_.AddOffset(S2C_CHATECHO_NTY::VT_MSG, msg);
  }
  explicit S2C_CHATECHO_NTYBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  flatbuffers::Offset<S2C_CHATECHO_NTY> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<S2C_CHATECHO_NTY>(end);
    return o;
  }
};

inline flatbuffers::Offset<S2C_CHATECHO_NTY> CreateS2C_CHATECHO_NTY(
    flatbuffers::FlatBufferBuilder &_fbb,
    int32_t size = 0,
    int32_t code = 0,
    int32_t userIdx = 0,
    flatbuffers::Offset<flatbuffers::String> msg = 0) {
  S2C_CHATECHO_NTYBuilder builder_(_fbb);
  builder_.add_msg(msg);
  builder_.add_userIdx(userIdx);
  builder_.add_code(code);
  builder_.add_size(size);
  return builder_.Finish();
}

inline flatbuffers::Offset<S2C_CHATECHO_NTY> CreateS2C_CHATECHO_NTYDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    int32_t size = 0,
    int32_t code = 0,
    int32_t userIdx = 0,
    const char *msg = nullptr) {
  auto msg__ = msg ? _fbb.CreateString(msg) : 0;
  return CreateS2C_CHATECHO_NTY(
      _fbb,
      size,
      code,
      userIdx,
      msg__);
}

inline const S2C_CHATECHO_NTY *GetS2C_CHATECHO_NTY(const void *buf) {
  return flatbuffers::GetRoot<S2C_CHATECHO_NTY>(buf);
}

inline const S2C_CHATECHO_NTY *GetSizePrefixedS2C_CHATECHO_NTY(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<S2C_CHATECHO_NTY>(buf);
}

inline bool VerifyS2C_CHATECHO_NTYBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<S2C_CHATECHO_NTY>(nullptr);
}

inline bool VerifySizePrefixedS2C_CHATECHO_NTYBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<S2C_CHATECHO_NTY>(nullptr);
}

inline void FinishS2C_CHATECHO_NTYBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<S2C_CHATECHO_NTY> root) {
  fbb.Finish(root);
}

inline void FinishSizePrefixedS2C_CHATECHO_NTYBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<S2C_CHATECHO_NTY> root) {
  fbb.FinishSizePrefixed(root);
}

#endif  // FLATBUFFERS_GENERATED_S2CCHATECHONTY_H_
