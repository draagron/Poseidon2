# Test Helpers

This directory contains shared test utilities that can be included by any test.

**Note**: This directory does NOT have the `test_` prefix, so it will not be discovered as a test by PlatformIO. It's purely for shared code.

## Usage

```cpp
// In any test file
#include "../helpers/test_mocks.h"
#include "../helpers/test_fixtures.h"
#include "../helpers/test_utilities.h"
```

## Files (to be created as needed)

- `test_mocks.h` - Mock implementations of HAL interfaces
- `test_fixtures.h` - Common test data and fixtures
- `test_utilities.h` - Helper functions for tests

## Example Mock

```cpp
// test_mocks.h
#ifndef TEST_MOCKS_H
#define TEST_MOCKS_H

#include "../../src/hal/interfaces/IBoatDataStore.h"

class MockBoatDataStore : public IBoatDataStore {
    // Mock implementation
};

#endif
```
