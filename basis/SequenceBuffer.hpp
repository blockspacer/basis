#pragma once

#include <cstdint>
#include <string>
#include <optional>
#include <functional>
#include <unordered_map>

#include <glm/vec3.hpp>

#include "common/ECS/ecs.hpp"
#include "common/ECS/tags.hpp"

#include <base/logging.h>
#include <base/trace_event/trace_event.h>

namespace backend {

//enum class PlayerLoadState {
//  LOADING
//  , LOADED
//  , UNLOADING
//  , UNLOADED
//  , TOTAL
//};

/// \note uint64_t range 0 .. 18,446,744,073,709,551,615
typedef std::uint64_t TickNumType;

typedef size_t TickBufferSizeType;

/// \note keep that data type small,
/// otherwise reserved buffer will take a lot of memory
struct UserCommand
{
  TickNumType tickSeqId = 0;

  /// \note detect missing packets at the end of the tick
  /// and fill 'holes', for example - by predicting movement
  /// based on client's input from prev. tick
  bool isDropped = false;
};

/// \note expected to work with ordeded sequence without holes or duplication
/// i.e. server snapshot generation using SequenceBuffer
/// will store N last snapshots made by server
/// \note conatainer expects that mapping tick num. to value
/// is not frequent operation
// stores two buffers:
// 1) buffer of values i.e.
//    [Snapshot4, Snapshot2, Snapshot3]
// 2) buffer that maps tick num. to value i.e.
//    [tick4, tick2, tick3]
template<typename Type, TickBufferSizeType Size>
class SequenceBuffer {
public:
  typedef std::function<
    void(const Type&, const TickNumType&)
  > EachCb;

public:
  SequenceBuffer() {
    clear();
  }

  void each(const EachCb& cb) {
    DCHECK(cb);
    for(size_t i = 0; i < buffer_.size(); i++)
    {
      const bool has_value
        = sequences_[i].has_value();
      if(has_value) {
        cb(buffer_[i], sequences_[i].value());
      }
    }
  }

  size_t size() const {
    return buffer_.size();
  }

  void clear()
  {
    for (int i = 0; i < sequences_.size(); ++i) {
      sequences_[i] = std::nullopt;
    }
  }

  TickNumType latestTick() const noexcept
  {
    return latestTick_;
  }

  void setByTickNum(const Type& value, const TickNumType tickNum)
  {
    const size_t tickIndex = tickNumToBufferIndex(tickNum);

    DCHECK(tickIndex >= 0);
    DCHECK(tickIndex < buffer_.size());
    buffer_[tickIndex] = value;
    sequences_[tickIndex] = tickNum;
    if(tickNum > latestTick_) {
      latestTick_ = tickNum;
    }
  }

  [[nodiscard]] /* don't ignore return value */
  bool hasValue(TickNumType tickNum) const
  {
    const size_t tickIndex = tickNumToBufferIndex(tickNum);

    const bool has_value
      = sequences_[tickIndex].has_value();

    // if we have value, than we expect valid seq. num.
    DCHECK(has_value
      ? (sequences_[tickIndex].value() == tickNum)
      : true)
      << " something went wrong for tickIndex = "
      << tickIndex;

    return has_value;
  }

  [[nodiscard]] /* don't ignore return value */
  bool tryGetValue(TickNumType tickNum, Type* result)
  {
    const size_t tickIndex = tickNumToBufferIndex(tickNum);
    DCHECK(tickIndex >= 0);
    DCHECK(tickIndex < buffer_.size());
    *result = buffer_[tickIndex];

    if(!sequences_[tickIndex].has_value()
       // if we have value, than we expect valid seq. num.
       || sequences_[tickIndex].value() != tickNum)
    {
      DCHECK(false)
        << " something went wrong for tickIndex = "
        << tickIndex;
      return false;
    }

    return true;
  }

  [[nodiscard]] /* don't ignore return value */
  size_t tickNumToBufferIndex(const TickNumType tickNum) const
  {
    /// \note tickNum must start from 1
    DCHECK(tickNum > 0);
    DCHECK(tickNum < std::numeric_limits<TickNumType>::max());

    /// \n example:
    ///  when buffer size = 6:
    ///    tickNum = 1 => buffer[1 % 6] => buffer[1]
    ///    tickNum = 6 => buffer[6 % 6] => buffer[0]
    ///    tickNum = 7 => buffer[7 % 6] => buffer[1]
    ///    tickNum = 133 => buffer[63 % 6] => buffer[3]
    const TickNumType tickIndex = tickNum % buffer_.size();
    return tickIndex;
  }

private:
  TickBufferSizeType maxBufferSize_ = Size;

  TickNumType latestTick_ = 0;

  std::array<std::optional<TickNumType>, Size> sequences_{};
  std::array<Type, Size> buffer_{};
};

} // namespace backend
