#include <basis/sequence_bound_flag.hpp>

// Flag that can be used only on on sequence
//
// USAGE
//
// DEBUG_FLAG(object_valid);
// SET_DEBUG_FLAG(object_valid);
// DCHECK_HAS_FLAG(object_valid);
// UNSET_DEBUG_FLAG(object_valid);
// DCHECK_NO_FLAG(object_valid);

// Flags some object state as `set` or `not set`.
#if DCHECK_IS_ON()
  #define DEBUG_FLAG(OBJ) \
    SequenceBoundFlag OBJ
#else
  #define DEBUG_FLAG(OBJ)
#endif

#define DCHECK_HAS_FLAG(OBJ) \
  DCHECK(OBJ.IsSet()) \
    << FROM_HERE.ToString() \
    << " expected to be set: " STRINGIFY(OBJ);

#define DCHECK_NO_FLAG(OBJ) \
  DCHECK(!OBJ.IsSet()) \
    << FROM_HERE.ToString() \
    << " expected to be NOT set: " STRINGIFY(OBJ);

#if DCHECK_IS_ON()
  // Flags some object state as `valid`.
  #define SET_DEBUG_FLAG(OBJ) \
    OBJ.Set()
#else
  #define SET_DEBUG_FLAG(OBJ)
#endif

#if DCHECK_IS_ON()
  // Flags some object state as `valid`.
  #define UNSET_DEBUG_FLAG(OBJ) \
    OBJ.UnSet()
#else
  #define UNSET_DEBUG_FLAG(OBJ)
#endif
