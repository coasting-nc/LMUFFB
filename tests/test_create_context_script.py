
import os
import sys
import unittest
from unittest.mock import patch, MagicMock

# Import the script
import scripts.create_context as create_context

class TestCreateContext(unittest.TestCase):
    def test_file_classification(self):
        # Code files
        self.assertTrue(create_context.is_code_file("main.cpp"))
        self.assertTrue(create_context.is_code_file("CMakeLists.txt"))
        self.assertTrue(create_context.is_code_file("script.py"))
        self.assertFalse(create_context.is_code_file("README.md"))
        
        # Doc files
        self.assertTrue(create_context.is_doc_file("README.md"))
        self.assertTrue(create_context.is_doc_file("notes.txt"))
        self.assertFalse(create_context.is_doc_file("main.cpp"))

    def test_is_test_file(self):
        self.assertTrue(create_context.is_test_file("tests/engine/test_math.cpp", "test_math.cpp"))
        self.assertTrue(create_context.is_test_file("src/test_logic.py", "test_logic.py"))
        self.assertTrue(create_context.is_test_file("src/utils_test.cpp", "utils_test.cpp"))
        self.assertFalse(create_context.is_test_file("src/main.cpp", "main.cpp"))
        self.assertTrue(create_context.is_test_file("tests/anyfile.h", "anyfile.h"))
        self.assertTrue(create_context.is_test_file("some/tests/sub/anyfile.h", "anyfile.h"))

    def test_argument_parsing_defaults(self):
        with patch.object(sys, 'argv', ['create_context.py']):
            # Simulating main's injection logic isn't perfect here but let's test parse_args directly first
            args = create_context.parse_args([])
            self.assertTrue(args.include_tests)
            self.assertTrue(args.include_non_code)

    def test_argument_parsing_explicit(self):
        args = create_context.parse_args(['--exclude-tests', '--exclude-non-code'])
        self.assertFalse(args.include_tests)
        self.assertFalse(args.include_non_code)

    def test_injection_logic(self):
        # Test the injection logic in a simulated way
        def get_injected_args(cli_args):
            DEFAULT_INCLUDE_TESTS = True
            DEFAULT_INCLUDE_NON_CODE = True
            
            injected = list(cli_args)
            if "--include-tests" not in injected and "--exclude-tests" not in injected:
                injected.append("--include-tests" if DEFAULT_INCLUDE_TESTS else "--exclude-tests")
            if "--include-non-code" not in injected and "--exclude-non-code" not in injected:
                injected.append("--include-non-code" if DEFAULT_INCLUDE_NON_CODE else "--exclude-non-code")
            return injected

        self.assertIn("--include-tests", get_injected_args([]))
        self.assertIn("--include-non-code", get_injected_args([]))
        self.assertNotIn("--include-tests", get_injected_args(["--exclude-tests"]))
        self.assertIn("--exclude-tests", get_injected_args(["--exclude-tests"]))

    def test_exclusions(self):
        # File exclusions
        self.assertIn("USER_CHANGELOG.md", create_context.EXCLUDE_FILES)
        self.assertIn("FULL_PROJECT_CONTEXT.md", create_context.EXCLUDE_FILES)
        
        # Directory exclusions
        self.assertIn("docs/dev_docs/done implementations reports", create_context.EXCLUDE_DIR_PATHS)
        self.assertIn("tools", create_context.EXCLUDE_DIR_PATHS)

if __name__ == '__main__':
    unittest.main()
