# About

Basis is pulled into many projects.

So the bar for adding stuff is that it must have demonstrated wide applicability.

Pull into basis only when needed.

In a project our size, sometimes even duplication is OK and inevitable.

## Build

```bash
# NOTE: change `build_type=Debug` to `build_type=Release` in production
CONAN_REVISIONS_ENABLED=1 \
    CONAN_VERBOSE_TRACEBACK=1 \
    CONAN_PRINT_RUN_COMMANDS=1 \
    CONAN_LOGGING_LEVEL=10 \
    GIT_SSL_NO_VERIFY=true \
    conan create . \
        conan/stable \
        -s build_type=Debug \
        --profile clang \
        --build missing \
        -e basis:enable_tests=True \
        -o openssl:shared=True
```


## Qualifications for being in basis OWNERS

interest and ability to learn low level/high detail/complex c++ stuff

inclination to always ask why and understand everything (including external interactions like win32) rather than just hoping the author did it right
mentorship/experience

demonstrated good judgement (esp with regards to public APIs) over a length of time

Owners are added when a contributor has shown the above qualifications and when they express interest.

There isn't an upper bound on the number of OWNERS.

## Design and naming

Be sure to use the basis namespace.

STL-like constructs should adhere as closely to STL as possible.

Functions and behaviors not present in STL should only be added when they are related to the specific data structure implemented by the container.

For STL-like constructs our policy is that they should use STL-like naming even when it may conflict with the style guide.

So functions and class names should be lower case with underscores.
