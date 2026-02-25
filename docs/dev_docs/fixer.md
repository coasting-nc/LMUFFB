# The Fixer: Quality Enforcement Guidelines

This project maintains a high standard of code quality and reliability. As a developer/agent working on this codebase, you must adhere to the following enforcement rules.

## 1. Test-Driven Development (TDD)
- **Always write a test first**: Before implementing a feature or fixing a bug, write a test that fails.
- **Minimum implementation**: Write only the code necessary to make the test pass.
- **Refactor**: Clean up the code while keeping the tests passing.

## 2. The 99% Coverage Rule
We aim for near-perfect coverage of our core logic to prevent regressions in the physics engine.

- **Requirement**: Any new code added to `src/` must be covered by unit tests with at least **99% line coverage**.
- **Exception**: Platform-specific UI boilerplate (e.g., Win32 message loops) may be excluded if impossible to test headlessly, but all business logic must be platform-agnostic and tested.
- **Enforcement**: If a pull request or code change results in less than 99% coverage for the new feature, **it is not finished**. You must:
    1. Identify the uncovered lines in the coverage report.
    2. Add unit tests that exercise those specific branches/paths.
    3. Re-run coverage and repeat until 99% is achieved.

## 3. Cross-Platform Consistency
- Changes must not break the Linux headless build.
- All new features should have a platform-agnostic core and platform-specific implementations where necessary.
- Use the `LinuxMock.h` system for testing Windows-only logic paths on Linux.

## 4. Performance & Optimizations
- Keep tests fast. Compilation of tests should use minimal optimizations (`/Od` on MSVC, `-O0` on GCC) during development to speed up the iterative cycle.
- The test suite should execute in under 5 seconds to encourage frequent runs.

---

**Remember**: Code without tests is a bug waiting to happen. Code with partial tests is a false sense of security. **Aim for 99%!**
