# Clang-Tidy Suppressions Backlog

This document tracks all `clang-tidy` rules that are currently suppressed in both the local build environments (`build_commands.txt`) and CI pipelines (`windows-build-and-test.yml`). 

The suppressed rules have been prioritized by severity. We recommend tackling the **Top 10 High Priority** items first, ideally using a test-driven approach where you re-enable the rule, build to expose the warnings, and commit fixes incrementally.

---

## 🛑 Top 10 High Priority Tasks

These warnings carry the highest risk because they deal with undefined behavior, mathematical or logical inaccuracies, or memory safety issues. 

### 1. ~~`bugprone-unchecked-optional-access`~~ (✅ **RESOLVED in 0.7.92**)
- **Why it matters**: Accessing an empty `std::optional` using `.value()` without checking if it contains a value will throw a `std::bad_optional_access` exception, which will crash the application immediately if unhandled.
- **Action**: Ensure all optionals are verified via `.has_value()` or `if (opt)` before dereferencing.

### 2. ~~`bugprone-exception-escape`~~ (✅ **RESOLVED in 0.7.94**)
- **Why it matters**: If an exception escapes a function marked `noexcept` or a destructor, the C++ runtime will instantly call `std::terminate()`, leading to an abrupt application crash with no chance of recovery.
- **Action**: Add `try/catch` blocks appropriately, or remove the `noexcept` marker if throwing is expected.

### 3. ~~`bugprone-empty-catch`~~ (✅ **RESOLVED in 0.7.95**)
- **Why it matters**: Catching exceptions and doing nothing completely hides failures. This creates "silent errors" where the system continues in an unstable or unexpected state, making debugging nearly impossible.
- **Action**: Add minimum logging or error handling within empty `catch (...) {}` blocks.

### 4. ~~`performance-no-int-to-ptr`~~ (✅ **RESOLVED in 0.7.96**)
- **Why it matters**: Direct integer-to-pointer casts can inhibit compiler optimizations because the compiler cannot easily track the lineage and alignment of the resulting pointer. 
- **Action**: Use `reinterpret_cast` with appropriate intermediate types (like `uintptr_t`) to make the operation explicit and optimization-friendly.

### 5. ~~`bugprone-incorrect-roundings`~~ (✅ **RESOLVED in 0.7.97**)
- **Why it matters**: Approximating floating-point numbers by adding `0.5` and truncating to an integer (e.g. `(int)(float_val + 0.5f)`) can cause numerical instability resulting in off-by-one errors.
- **Action**: Replace legacy rounding logic with `std::lround` from `<cmath>`, which provides consistent and mathematically sound rounding to the nearest integer.

### 6. ~~`bugprone-integer-division`~~ (✅ **RESOLVED in 0.7.99**)
- **Why it matters**: Performing division on integers truncates the decimal portions directly before it gets converted or stored in a float or double variable, unknowingly leading to lost precision (e.g., `float x = 5 / 2; // Sets x to 2.0`).
- **Action**: Perform calculations using explicit floating-point values (e.g. `5 / 2.0f`).

### 7. `bugprone-implicit-widening-of-multiplication-result`
- **Why it matters**: When multiplying two integers, the result can wrap around or overflow *before* being cast or widened into a larger type (like `size_t` or `long long`), destroying the value silently.
- **Action**: Explicitly cast the operands to standard width sizes prior to multiplication.

### 8. `bugprone-narrowing-conversions`
- **Why it matters**: Implicitly casting from a larger type to a smaller type (e.g., `double` into `float`, or `int` into `short`) discards precision. In physics, telemetry or GUI code, this lost data builds up and causes unexpected graphical or logical errors.
- **Action**: Refactor logic to match data bounds, or use explicit static casting.

### 9. `bugprone-branch-clone`
- **Why it matters**: Having multiple conditionals performing exactly the same branch code often indicates copy-paste errors, creating fragile, confusing logic operations.
- **Action**: Extract shared branch code into shared paths, or bridge conditionals together via logical `||` bounds.

### 10. `bugprone-easily-swappable-parameters`
- **Why it matters**: Functions that accept multiple parameters of the same type sequentially (e.g., `void SetCoordinates(float y, float x)`) are extremely prone to being passed data backward, creating difficult-to-catch logical problems.
- **Action**: Refactor method interfaces using descriptive structs, or enforce order strictly via strong types.

---

## 🟡 Medium Priority (Performance & Medium Complexity)

These primarily pertain to performance bottlenecks or notable readability degradation. Address these to refine efficiency.

*   `performance-unnecessary-value-param`: Passing heavy types by value copies the variable wastefully limitings speed. Use `const T&` (const reference) or `std::move`.
*   `performance-inefficient-string-concatenation`: Avoid standard appending strings together multiple times rapidly; it forces excessive and sluggish memory reallocations. 
*   `performance-avoid-endl`: Using `std::endl` redundantly flushes the I/O streams during logging, stalling threads. Rely on `\n` character linebreaks instead.
*   `performance-type-promotion-in-math-fn`: Overloading default C++ math functions with smaller data types accidentally scales types implicitly upwards, burning CPU cycles unnecessarily. 
*   `readability-container-size-empty`: Explicitly checking `items.size() == 0` is less readable and slightly less efficient than leveraging `items.empty()`.
*   `readability-magic-numbers`: Hardcoded numeric constants obscure the intent behind equations and drastically increases long-term maintenance burdens. Extract variables.
*   `readability-function-cognitive-complexity`: Functions bloated with excessive nesting loops and conditional jumps hinder comprehension exponentially.
*   `readability-static-accessed-through-instance`: Accessing static variables tied to instance objects masks their static origins.
*   `bugprone-unused-local-non-trivial-variable`: Variables initialized but never read incur unnecessary performance cost and memory allocations.

---

## 🟢 Low Priority (Stylistic & Pedantic Formatting)

These checks flag stylistic non-conformities or C++ semantics that do not influence performance or runtime safety. You can optionally re-enable and resolve them or keep them permanently suppressed.

*   `readability-redundant-string-cstr` (Redundant `.c_str()` calls when constructing `std::string` wrappers)
*   `readability-redundant-casting` (Using explicit C-style casting uniformly rather than ignoring default C++ type promotion)
*   `readability-make-member-function-const` (Pedantic rules around class modifications)
*   `readability-convert-member-functions-to-static` (Class members acting functionally pure but belonging physically scoped to dynamic objects)
*   `readability-isolate-declaration` (Isolating all individual variable allocations vertically instead of on single lines horizontally)
*   `readability-implicit-bool-conversion` (Casting variable instances silently instead of comparing them actively against `== nullptr` or `== false`)
*   `readability-else-after-return` 
*   `readability-redundant-string-init`
*   `readability-inconsistent-declaration-parameter-name`
*   `readability-qualified-auto`
*   `readability-braces-around-statements`
*   `readability-avoid-nested-conditional-operator`
*   `readability-identifier-length`
*   `readability-uppercase-literal-suffix`
*   `readability-redundant-declaration` (Redundant forward declarations clutter the translation unit negatively but silently)
