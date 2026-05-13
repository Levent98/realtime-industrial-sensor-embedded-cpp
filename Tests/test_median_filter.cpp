#include "MedianFilter.hpp"
#include <cassert>

int main()
{
    MedianFilter filter;

    // TC-FILTER-001: Filter starts uninitialized.
    assert(!filter.initialized());

    // TC-FILTER-002: First sample is returned directly before the filter is fully initialized.
    assert(filter.apply(10) == 10);

    // TC-FILTER-001: Filter should still be uninitialized after a single sample.
    assert(!filter.initialized());

    // TC-FILTER-003: Seeding initializes the filter with a stable baseline value.
    filter.seed(25);
    assert(filter.initialized());

    // TC-FILTER-004: Outlier rejection / median stability.
    // 17 normal samples seeded as 25 + one outlier sample 1000 should keep median at 25.
    assert(filter.apply(1000) == 25);

    // TC-FILTER-005: Reset clears the initialized state.
    filter.reset();
    assert(!filter.initialized());

    // TC-FILTER-002: After reset, the first sample again becomes the immediate output.
    assert(filter.apply(5) == 5);

    // TC-FILTER-004: Median behavior remains stable with additional samples.
    assert(filter.apply(1) == 5);
    assert(filter.apply(9) == 5);

    return 0;
}
