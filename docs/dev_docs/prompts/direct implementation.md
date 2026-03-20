Read docs\dev_docs\investigations\redesign presets system.md  and docs\dev_docs\investigations\redesign presets system - Phase 1.md.
Then follow the plan defined in "## Adopt the "Strangler Fig" pattern in refactoring" (and where necessary, also what is described in "## Logical Categories Plan"), and implement it just for the next logical category.

Note that the refactoring for these logical categories has already been performed: 
* GeneralConfig
* FrontAxleConfig

Only work on the next logical category, make sure all tests pass, and then stop. 

You also have to document your work by creating a .md document with implementation notes. 
Include these sections: encountered issues, deviations from the plan, and suggestions for the future. 
Document deviations and build issues.
Note any specific issues encountered during the Build/Test loop.
In the implementation notes also discuss any issues raised by the code reviews and how you addressed them; also discuss there any discrepancies between you and the code reviews (eg. the code review said the patch would not build, but it does).

See also the implementation notes that were created in previous iterations of this feature: 
* docs\dev_docs\implementation_plans\redesign_preset_system_phase1_general.md
* docs/dev_docs/implementation_plans/redesign_preset_system_phase1_front_axle.md



**⚠️ CRITICAL WORKFLOW CONSTRAINTS:**
1.  **Autonomous Execution:** **Do not stop** to ask the user for confirmation or permission to proceed. You must loop through the implementation and review process autonomously until the task is complete and the code is perfect.
