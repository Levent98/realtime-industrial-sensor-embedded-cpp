#ifndef TESTABILITY_HPP
#define TESTABILITY_HPP

/*
 * Host-side unit tests need a few production classes to be mockable.
 * On the embedded firmware build we keep these methods non-virtual to avoid
 * unnecessary vtable/vptr overhead in flash/RAM.
 */
#ifdef UNIT_TEST
#define TEST_VIRTUAL virtual
#define TEST_OVERRIDE override
#else
#define TEST_VIRTUAL
#define TEST_OVERRIDE
#endif

#endif /* TESTABILITY_HPP */
