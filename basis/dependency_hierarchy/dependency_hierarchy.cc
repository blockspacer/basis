#include "basis/dependency_hierarchy/dependency_hierarchy.hpp" // IWYU pragma: associated

#include <base/recursion_checker.h>
#include <base/logging.h>
#include <base/path_service.h>
#include <base/files/file_util.h>
#include <base/i18n/icu_util.h>

#include <algorithm>

namespace basis {

namespace {

// detect infinite recursion while walking tree
static FUNCTION_RECURSION_CHECKER_LIMIT_999(hasNestedDependencyLimit);

} // namespace

bool DependencyComparator::operator()(
    const scoped_refptr<Dependency>& a,
    const scoped_refptr<Dependency>& b) const {
  /// \note Source of non-determinism is the reliance on pointer comparison.
  /// Tests need determinism.
  return a.get() < b.get();
}

bool DependenciesComparator::operator()(
    const scoped_refptr<Dependencies>& a,
    const scoped_refptr<Dependencies>& b) const {
  /// \note Source of non-determinism is the reliance on pointer comparison.
  /// Tests need determinism.
  return a.get() < b.get();
}

Dependency::Dependency()
  : dependencies_(base::MakeRefCounted<Dependencies>())
{
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

Dependency::~Dependency()
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

Dependencies::Dependencies()
{}

Dependencies::~Dependencies()
{
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  for(scoped_refptr<Dependency> dependency: storage_) {
    DCHECK(dependency);
  }
}

basis::Status Dependencies::addDependency(scoped_refptr<Dependency> dependency)
{
  using namespace basis::error;

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(dependency);
  if(!dependency) {
    RETURN_ERROR(INVALID_ARGUMENT)
      << "null can not be dependency";
  }

  storage_.emplace(dependency);

  RETURN_OK();
}

basis::Status Dependencies::removeDependency(scoped_refptr<Dependency> dependency)
{
  using namespace basis::error;
  using namespace basis::dependency_error_space;

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(dependency);
  if(!dependency) {
    RETURN_ERROR(INVALID_ARGUMENT)
      << "null can not be dependency";
  }

  auto found = std::find(storage_.begin(), storage_.end(), dependency);
  if (found == storage_.end()) {
    RETURN_ERROR(ERR_DEPENDENCY_NOT_FOUND).without_logging()
      << "Can not remove dependency that was not added before";
  }

  storage_.erase(found);

  RETURN_OK();
}

basis::Status Dependencies::addDependencies(scoped_refptr<Dependencies> other)
{
  using namespace basis::error;
  using namespace basis::dependency_error_space;

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(other);
  if(!other) {
    RETURN_ERROR(INVALID_ARGUMENT)
      << "null can not be dependency";
  }

  for (scoped_refptr<Dependency> dependency: other->storage()) {
    DCHECK(dependency);
    basis::Status result = addDependency(dependency);
    /// \note skips minor errors (`INVALID_ARGUMENT`, etc.)
    RETURN_IF_ERROR_CODE(result, ERR_CIRCULAR_DEPENDENCY);
  }

  RETURN_OK();
}

basis::Status Dependencies::removeDependencies(scoped_refptr<Dependencies> other)
{
  using namespace basis::error;

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(other);
  if(!other) {
    RETURN_ERROR(INVALID_ARGUMENT)
      << "null can not be dependency";
  }

  basis::Status result(FROM_HERE);

  for (scoped_refptr<Dependency> dependency: other->storage()) {
    DCHECK(dependency);
    APPEND_STATUS_IF_ERROR(result, removeDependency(dependency));
  }

  return result;
}

bool Dependencies::hasNestedDependency(scoped_refptr<Dependency> dependency) const
{
  using namespace basis::dependency_error_space;

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK_FUNCTION_RECURSION(hasNestedDependencyLimit);

  DCHECK(dependency);
  if(!dependency) {
    return false;
  }

  // will store dependencies at some hierarchy level
  DependenciesSet dependenciesToProcess{
    ::base::WrapRefCounted(const_cast<Dependencies*>(this))};

  /// \note recursion replaced with iteration
  do {
    DependenciesSet dependenciesToAdd;
    // iterate elements in current hierarchy level
    for(auto dep_it = dependenciesToProcess.begin(); dep_it != dependenciesToProcess.end(); dep_it++)
    {
      DCHECK(*dep_it);

      const DependencySet& storage = (*dep_it)->storage();
      // process dependencies from element in current hierarchy level
      for(auto store_it = storage.begin(); store_it != storage.end(); store_it++)
      {
        DCHECK(*store_it);

        if(*store_it == dependency) {
          return true;
        }

        // add dependencies from element in nested hierarchy level
        if((*store_it)->dependencies()) {
          dependenciesToAdd.insert((*store_it)->dependencies());
        }
      }
    }
    // reset dependencies to dependencies from nested hierarchy levels
    {
      dependenciesToProcess.clear();
      dependenciesToProcess = dependenciesToAdd;
    }
  } while (!dependenciesToProcess.empty());

  return false;
}

}  // namespace basis
