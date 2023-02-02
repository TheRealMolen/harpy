unit testing:

IDEALLY:
* hold button on boot to run tests
* tests output via daisy PrintLine

tried to use doctest, but it uses too much ostream and heap for the daisy

--> TODO: write something simple
* autoregister with linked list of static pointers
* lightweight assert/check mechanism just logs as it goes, mostly relying on __FILE__ & __LINE__ to identify issues (no fancy stringifying by default)
* log message system just sends to hw::Print -- perhaps could write to flash/SD as well in future but not now
* single #define to remove all testing code & test system
* no: test filters, multiple reporters, crash detection & recovery, timing, callstacks, 



here's the wip i had before abandoning doctest:
```
#define DOCTEST_CONFIG_IMPLEMENT
#define DOCTEST_CONFIG_NO_POSIX_SIGNALS
#define DOCTEST_CONFIG_NO_MULTITHREADING
#define DOCTEST_CONFIG_NO_EXCEPTIONS
#define DOCTEST_BREAK_INTO_DEBUGGER() (static_cast<void>(0))
#include "extern/doctest.h"

namespace mln::test
{
using namespace doctest;

struct DaisyReporter : public IReporter
{
    const ContextOptions& opt;
    const TestCaseData*   testCase = nullptr;

    DaisyReporter(const ContextOptions& in) : opt(in) { /**/ }

    void report_query(const QueryData& /*in*/) override { /**/ }	// for e.g. help, listing options, etc.

    void test_run_start() override
	{
		hw.PrintLine("TEST: run starting");
	}

    void test_run_end(const TestRunStats& stats) override
	{
		bool allGood = (stats.numTestCasesFailed + stats.numAssertsFailed) == 0;
		if (allGood)
			hw.PrintLine("TEST: all tests PASSED (%d cases with a total of %d checks)", stats.numTestCasesPassingFilters, stats.numAsserts);
		else
			hw.PrintLine("TEST: %d tests FAILED (%d cases tested, %d checks failed)", stats.numTestCasesFailed, stats.numTestCasesPassingFilters, stats.numAssertsFailed);
	}

    void test_case_start(const TestCaseData& in) override
	{
		testCase = &in;
	}

    // called when a test case is reentered because of unfinished subcases
    void test_case_reenter(const TestCaseData& /*in*/) override { /**/ }

    void test_case_end(const CurrentTestCaseStats& stats) override
	{
		if (stats.failure_flags != TestCaseFailureReason::None)
			hw.PrintLine("TEST:    case %s failed", testCase->m_name);
	}

    void test_case_exception(const TestCaseException& exception) override
	{
		hw.PrintLine("TEST:    case %s threw an exception o_O", testCase->m_name);
	}

    void subcase_start(const SubcaseSignature& /*in*/) override { /**/ }
    void subcase_end() override { /**/ }

    void log_assert(const AssertData& asserrt) override
	{
		if (asserrt.m_failed)
		{
			hw.PrintLine("TEST:    case %s: failed a check: %s(%s", testCase->m_name);
		}
    }

    void log_message(const MessageData& /*in*/) override
	{
        // ...
    }

    void test_case_skipped(const TestCaseData& /*in*/) override {}
};

REGISTER_LISTENER("daisySerial", -1, DaisyReporter);
} // namespace



TEST_SUITE_BEGIN("core");

TEST_CASE("square")
{
	CHECK_EQ(square(0.f), 0.f);
	CHECK_EQ(square(1.f), 1.f);
	CHECK_EQ(square(2.f), 4.f);
	CHECK_EQ(square(-2.f), 4.f);
}

TEST_SUITE_END();


void runUnitTests()
{
	doctest::Context context;
	const char* argv[] = {"daisy", "--reporters=daisySerial"};
	int argc = sizeof(argv)/sizeof(argv[0]);
	context.applyCommandLine(argc, argv);
	int returnCode = context.run();
	if (returnCode == 0)
		led2.SetRgb(0.f, 1.f, 0.f);
	else
		led2.SetRgb(1.f, 0.f, 0.f);
}
```
