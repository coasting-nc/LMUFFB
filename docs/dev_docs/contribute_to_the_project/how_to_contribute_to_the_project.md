This document provides step-by-step guides for contributing to the project using AI-assisted coding tools. Even if you prefer to implement changes manually, AI will be needed to perform code reviews of your work.


# Workflow for AI-assisted coding

Note: some of the steps of the workflow are suggestions, meaning that you might use other models or IDEs. However, the deliverables at the end are required, because they serve as quality control checks that everything works as expected.

## IDE and other tools

* As IDE, Antigravity is recommended in this case, because it is basically free, and you can use Opus 4.5 for free (which is usually very expensive but sometimes necessary for more complex things when other models fail).

* The use of Google AI Studio and Gemini Deep Research is also recommended.

## Step 0: Deep Research (optional, needed for complex features that require expertise information)

* If it's a complex new feature that requires expertise information (like how to implement a particular effect), use Gemini Deep Research to gather the information that there is out there (eg. how TC control systems in cars use certain formulas to determine when the rear is stepping out).

* After that, the Deep Research Report needs to be exported into markdown format and saved to the project, so that AI coding agents can read it. To export a Gemini Deep Research report into markdown, you can first export it to Google docs, and from Google docs (if you have enabled markdown) you can select the whole document and do "copy as markdown", and then paste it into a new file in the lmuFFB project.

## Step 1: Implementation plan
* After you have the report from the deep research, you need to produce an implementation plan for the feature you want to implement, as a separate markdown file. To create the file, you could either:
   * In Antigravity: Ask Opus 4.5 or Gemini 3 Pro to create it, based on the deep research report and a description of what you want to implement.
   * Paste the whole codebase into Google AI Studio, and ask Gemini 3 Pro to create it. You can then use "copy as markdown" and paste the plan into a new .md file in the project.
       * In order to have the whole project as context in AI Studio, you need to create a markdown file that contains the whole codebase of the project (code, documents, and tests), by running the Python script that is here: "scripts\create_context.py". This updates the file docs\dev_docs\FULL_PROJECT_CONTEXT.md, which then you can paste into Google AI studio.

       * Note that Google AI studio can be used in this way (pasting the whole codebase and asking questions) for many more things, not just to create implementation plan. Eg. also to investigate bugs (ask what might be wrong with the code), or ask any kind of questions and suggestions. AI Studio is also free, with larger rate limits than Antigravity.

* When you ask AI to create an implementation plan, always ask also: 
    * to include in the plan instruction to create additional automated tests (the plan should include a test description and some code snippets) that comprehensively cover the new features and changes;
    * to include instructions to update the documentation in the project. The project has many documents, and it is better to keep them up to date as the code evolves.
    * to include in the implementation plan an introduction and overview to explain the motivation for implementing that particular feature.
    * to inlcude only code snippets of (some of) the changes proposed in the plan. The implementation plan should not include whole code files if they are too large and mostly unchanged.

* You should review the implementation plan to make sure it is aligned with the  requirements and that there are no issues that require further investigation or refinement.

## Step 2: Implementation

Use a slightly less capable model in Antigravity (Gemini 3 flash or Sonnet 4.5) and ask it to implement the plan. 
* The coding agent should also make sure all tests pass, and if they don't, it should iterate fixing the code and the tests until they all pass.
* At the end, it should also increase the app version number and add an entry to the changelog.

Once the model finishes the implementation, quickly read / skim through the diff (to make sure no apparent gross issues are there).
* Also make sure that the app and test compile, and that all tests pass (see the following section about this). 

## Make sure the app and tests compile and pass

* There is a build_commands.txt file in the root directory, and you need only the first 2 commands listed there: the first is to build the app and tests, and the second to run the tests.

* The first time you try to compile the app you need the additional commands (from the same file, build_commands.txt) to download Imgui in the vendor directory.

## Step 3: Code review

Open a new chat in Antigravity, and ask Sonnet 4.5 to do a code review of the changes, producing a markdown file with a detailed code review and recommended things to improve.

* Then skim through the code review, and read in detail only the recommended improvements to the current changes. You should usually ask Sonnet 4.5 in the same chat to implement most if not all of those recommendations.

* After all the recommended improvements are implemented, make sure again that the app and test compile, and that all tests pass.

## Step 4: Commit and release

Make sure the app version number is updated and there is a new entry in changelog.md file. 

Note that all markdown files produced in this process (eg. deep research, implementation plan, code review), must be included in the commit / merge request. This is to facilitate / speed up the review on my side before I can accept the merge request.

At this point the changes should be ready for commit and making a new release.

# Contributing manually written code

If you manually write code, you still need to ask AI to perform a code review of the changes.

You need to include in your commit / merge request a markdown file generated by AI with a detailed code review and recommended things to improve.

You basically need to follow all the previous steps described for AI-assisted coding, except that the "Implementation" part is done manually by you. This also means that your commit / merge request should include an implementation plan (in markdown format), and a deep research report (if advisable depending on the type of feature you are implementing).

If you write the implementation plan manually yourself, you must ask AI to review it and generate a review document as a markdown file.
This is to facilitate / speed up the review on my side before I can accept the merge request.


# Other use cases

## Investigating a bug

If you want to investigate a bug, or some issue that a user reported, and need suggestions on what might be causing it and how to fix it, you can do any of the following:

* **Ask Google AI Studio:** Paste the whole codebase into Google AI Studio (update and use docs\dev_docs\FULL_PROJECT_CONTEXT.md as described above), and ask Gemini 3 Pro to investigate the bug. After the model has given you a solution that you find satisfying / plausible, ask it to produce an implemenation plan for the fix. Then use "copy as markdown" and paste the plan into a new .md file in the project.
* **Ask in the IDE (Antigravity):** Use one of the best models (Opus 4.5 or Gemini 3 Pro) to investigate the bug and create a markdown report about it. 

If you are persuaded of the suggested cause and proposed solution to the issue, go ahead and produce an implementation plan as described above, and follow all subsequent steps (implement the implementation plan, do a code review, etc.). 

In this case the implementation plan must include, in the tests section, one or more regression tests (with a a description of the tests and some code snippets) that check that the same issue being fixed is not reintroduced in the future. 

## Simple use case: add a FFB preset to the app

* In Antigravity, Cursor, or other IDE, paste the portion of the ini file with your preset settings in the AI chat, and ask to create a new preset in the app.

* Also add an entry to the changelog and update the app version (either manually or with AI).

* When the model is finished, double check the diff and the git changes in the IDE to make sure no other files or presets were changed.

* Compile the app and tests, run the tests and make sure they pass (see above).

* You don't need any of the more complex steps described above (deep research, implementation plan, code review).

# TODOs 

TODO: add a list of planned features that contributors can choose from for things to implement.

TODO: clarify the branching strategy. Any guidelines regarding the creation of feature branches? Which branch should they target for merge requests?
