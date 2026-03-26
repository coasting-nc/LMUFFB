---
name: refactoring
description: Guidelines for preserving comments and documentation during code refactoring
---

# Refactoring and Comment Preservation

Use this skill when performing any refactoring task, such as moving code between files, renaming variables or functions, or restructuring existing logic.

## Core Principle

**Comments are a vital, crucial piece of documentation.** They must be treated with the same importance as the code itself. Every time you edit code or other files, you must ensure that existing comments are preserved and updated as necessary.

## Guidelines

### 1. Move Comments with Code
When moving a block of code (e.g., a function, a class, or a logic block) to a new location or a different file:
- **Relocate associated comments**: Ensure that block comments, docstrings, and inline comments that explain the moved code are relocated along with it.
- **Maintain Context**: The comments should remain adjacent to the code they describe to preserve their meaning and utility.

### 2. Preserve During Renaming
When renaming variables, functions, or types:
- **Do not delete explanatory comments**: Even if the name change makes the code "self-documenting," do not remove existing comments that provide additional context, rationale, or warnings.
- **Update references**: If a comment explicitly mentions the old name, update it to the new name to maintain consistency.

### 3. Do Not Delete Comments
During any edit (bug fix, feature addition, or cleanup):
- **Avoid accidental deletion**: Be careful when cutting and pasting or using automated refactoring tools that might strip comments.
- **Retain "Why" and "How"**: Comments often explain *why* a certain approach was taken or *how* a complex algorithm works. Deleting these makes the codebase harder to maintain and understand.

### 4. Review for Documentation Integrity
After a refactoring session:
- **Verify alignment**: Check that the comments still accurately describe the code in its new state/location.
- **Check for orphaned comments**: Ensure that no comments were left behind that no longer refer to any code in the original location.

## Importance
Refactoring without preserving comments is a destructive action. Comments provide the "tribal knowledge" and design intent that aren't always captured in the code structure itself. Preserving them ensures the long-term health and maintainability of the project.
