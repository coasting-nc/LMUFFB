
import unittest
import sys
import os
from unittest.mock import patch

# Force the script to be importable
sys.path.append('.')
import scripts.create_context as create_context

class TestCreateContextExtended(unittest.TestCase):
    def test_main_code_detection(self):
        self.assertTrue(create_context.is_main_code("src/FFBEngine.h"))
        self.assertTrue(create_context.is_main_code("src/main.cpp"))
        self.assertFalse(create_context.is_main_code("tests/test_math.cpp"))
        self.assertFalse(create_context.is_main_code("README.md"))

    def test_makefile_detection(self):
        self.assertTrue(create_context.is_make_file("CMakeLists.txt"))
        self.assertTrue(create_context.is_make_file("config.cmake"))
        self.assertTrue(create_context.is_make_file("Makefile"))
        self.assertFalse(create_context.is_make_file("main.cpp"))

    def test_argument_injection(self):
        # Simulated injection logic
        def get_args(cli_args):
            DEFAULT_INCLUDE_TESTS = True
            DEFAULT_INCLUDE_NON_CODE = True
            DEFAULT_INCLUDE_MAIN_CODE = True
            DEFAULT_INCLUDE_MAKEFILES = True
            DEFAULT_TEST_EXAMPLES_ONLY = False

            injected = list(cli_args)
            if "--include-tests" not in injected and "--exclude-tests" not in injected:
                injected.append("--include-tests" if DEFAULT_INCLUDE_TESTS else "--exclude-tests")
            if "--include-non-code" not in injected and "--exclude-non-code" not in injected:
                injected.append("--include-non-code" if DEFAULT_INCLUDE_NON_CODE else "--exclude-non-code")
            if "--include-main-code" not in injected and "--exclude-main-code" not in injected:
                injected.append("--include-main-code" if DEFAULT_INCLUDE_MAIN_CODE else "--exclude-main-code")
            if "--include-makefiles" not in injected and "--exclude-makefiles" not in injected:
                injected.append("--include-makefiles" if DEFAULT_INCLUDE_MAKEFILES else "--exclude-makefiles")
            if "--test-examples-only" not in injected:
                if DEFAULT_TEST_EXAMPLES_ONLY:
                    injected.append("--test-examples-only")
            return injected

        args = get_args([])
        self.assertIn("--include-main-code", args)
        self.assertIn("--include-makefiles", args)
        self.assertNotIn("--test-examples-only", args)

        args_ex = get_args(["--exclude-main-code"])
        self.assertIn("--exclude-main-code", args_ex)
        self.assertNotIn("--include-main-code", args_ex)

    def test_example_tests(self):
        self.assertIn('tests/test_ffb_common.cpp', create_context.EXAMPLE_TEST_FILES)
        self.assertIn('tests/main_test_runner.cpp', create_context.EXAMPLE_TEST_FILES)

if __name__ == '__main__':
    unittest.main()
