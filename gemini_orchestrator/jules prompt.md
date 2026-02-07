Implement a fix for this issue: 
 

Create a new branch to work on this task. 
 
In performing this task you have to do the following things: 
* Create an implementation plan, following these instructions: gemini_orchestrator\templates\A.1_architect_prompt.md 
 
* Do a review of the implementation plan, and updated it accordingly, following these instructions: gemini_orchestrator\templates\A.2_plan_reviewer_prompt.md. If possible (eg. among your tools), delegate the review to a sub agent that sees in isolation just the implementation plan and the initial query, without info about what you did to create the plan. 
 
* Implement the plan, following these instructions: gemini_orchestrator\templates\B_developer_prompt.md. Remember that you should verify that ALL tests pass before proceeding, including previous tests (not just the new ones). Remember that you have to follow a TDD development process: first write the new tests (and possibly some stubs of the main code to make it compile), verify that they fail, then implement the code to make them pass, and finally verify that all tests pass. If possible (eg. among your tools), delegate the code review to a sub agent that sees in isolation just the implementation plan, the initial query, and the files you have created, without info about what you did to create the files. 
 
* Do a code review of the implementation, following these instructions: gemini_orchestrator\templates\C_auditor_prompt_(code_review).md 
 


Make sure you meet these requirements:

* the implementation plan must include a section for the "Implementation Notes" as required here: gemini_orchestrator\templates\A.1_architect_prompt.md
* you must create all the deliverable requested in the implementation plan.
* you must incremented the version number (it should not be the same as before)
* you must update the changelog
* you must update the implementation plan filling the "Implementation Notes"
* the code review must be comprehensive, detailed, and catch any issue. When you do the code review, make sure that it has all the information necessary, it is comprehensive and effective.
