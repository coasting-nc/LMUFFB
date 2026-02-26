Add more tests to increase the code coverage. Consider coverage both in terms of code lines, branches, and functions. The coverage in terms of branches seems to be the lowest at the moment, with higher margins for increasing coverage.

See which tests are easier to add. If an intended test to address a particular coverage aspect proves to be too difficult, complex, or time consuming to make, you can skip it and don't write it. The overall goal is to increase the general code coverage to a good level, starting from the easiest tests to write.

The tests you write must be meaningful and test actual functionality, it is not acceptable to add tests that don't actually test anything, for the sake of only inflating the coverage numbers.

Avoid changing main code whenever possible. Prioritize writing tests that do not require chanes to the main code. If you encounter main code that should be changes to improve testability (eg. redundant or unnecessary code causing unreachable branches, changing the order of operators to make branches reachable, etc.), just leave comment in that code as TODO notes for later.

Also include in your patch a .md document report about what you did, strategies to increase code branches coverage, issues and challenges you encountered, and anything else worth noting about your task. The .md report must be a new file name (do not overwrite existing reports from past patches).

If the patch is building with no errors, but the code review says the project would not compile, explain in your report why it is compiling. In any case explain in your report if there was any issue building the project, and / or what the code review said about not building.

Also save the code reviews you receive as .md files under the docs/dev_docs/code_reviews directory. Use unique filenames with a prefix for this issue / task you are working on, and a suffix with the code review iteration number. You have to iterate repeating code reviews and addressing issues raised until you get a code review with a greenlight (correct rating).

I confirm that the code reviews must be saved to .md files and included in your patch. These must be the independent code reviews that you received from the code review tool, you must include them verbatim in the .md files.

The updated txt coverage summaries (for lines, branches, and functions) produced by scripts/coverage_summary.py must be included in your patch, as well as the code reviews .md files. This is to help me review the patch and track progress in the project.

Once you get a code review with a greenlight (correct rating), you can submit the patch (pull request).