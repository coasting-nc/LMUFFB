# AI Agent Available Tools

This document provides a comprehensive list of tools available to the AI agent (Jules) for performing tasks in the LMUFFB repository. This information helps users give precise instructions on which tools to use for specific tasks.

## Table of Contents
1. [File System Operations](#file-system-operations)
2. [Planning & Submission](#planning--submission)
3. [Code Modification](#code-modification)
4. [Execution & Testing](#execution--testing)
5. [External Information & Memory](#external-information--memory)
6. [Communication & Review](#communication--review)
7. [Deprecated Tools](#deprecated-tools)

---

## File System Operations

### `list_files`
Lists all files and directories under the given directory.
- **Parameters:**
  - `path` (string, optional): The directory path to list files from. Defaults to the root of the repo.

### `read_file`
Reads the content of the specified file in the repo. Returns an error if the file does not exist.
- **Parameters:**
  - `filepath` (string): The path of the file to read, relative to the repo root.

### `delete_file`
Deletes the specified file. Returns an error if the file does not exist.
- **Parameters:**
  - `filepath` (string): The path of the file to delete.

### `rename_file`
Renames and/or moves files and directories.
- **Parameters:**
  - `filepath` (string): The original path of the file or directory.
  - `new_filepath` (string): The new path for the file or directory.

### `reset_all`
Resets the entire codebase to its original state. Use this tool to undo all changes and start over.

### `restore_file`
Restores the given file to its original state.
- **Parameters:**
  - `filepath` (string): The path of the file to restore.

### `view_image`
Loads an image from a provided URL, allowing for content analysis.
- **Parameters:**
  - `url` (string): The URL of the image to view.

### `read_image_file`
Reads an image file from the repository into the agent's context.
- **Parameters:**
  - `filepath` (string): The path of the image file to read.

---

## Planning & Submission

### `set_plan`
Sets the plan for solving the issue. Use it after initial exploration and plan review.
- **Parameters:**
  - `plan` (string): The plan in Markdown format.

### `plan_step_complete`
Marks the current plan step as complete, explaining the actions taken.
- **Parameters:**
  - `message` (string): A succinct, descriptive message explaining the actions.

### `request_plan_review`
Requests a review for the proposed plan before setting it.
- **Parameters:**
  - `plan` (string): The plan to be reviewed.

### `record_user_approval_for_plan`
Records the user's approval for the plan.

### `pre_commit_instructions`
Provides instructions for pre-commit checks to ensure proper testing, verification, review, and reflection.

### `submit`
Commits the current code and requests user approval to push to their branch.
- **Parameters:**
  - `branch_name` (string): The name of the branch to commit to.
  - `commit_message` (string): The commit message.
  - `title` (string): The title of the submission.
  - `description` (string): The description of the submission.

### `done`
Indicates that a subagent has completed its task.
- **Parameters:**
  - `summary` (string): A summary of what was accomplished.

---

## Code Modification

### `write_file`
Creates a new file or overwrites an existing file with the provided content.
- **Parameters:**
  - `filepath` (string): The path of the file to create or overwrite.
  - `content` (string): The content to write to the file.

### `replace_with_git_merge_diff`
Performs a targeted search-and-replace using a Git merge diff format (`<<<<<<< SEARCH`, `=======`, `>>>>>>> REPLACE`).
- **Parameters:**
  - `filepath` (string): The path of the file to modify.
  - `merge_diff` (string): The diff content to apply.

---

## Execution & Testing

### `run_in_bash_session`
Runs the given bash command in the sandbox. Successive calls use the same session.
- **Parameters:**
  - `command` (string): The bash command to run.

### `frontend_verification_instructions`
Provides instructions on how to write a Playwright script to verify frontend web applications.

### `frontend_verification_complete`
Indicates that frontend changes have been verified and provides a path to the screenshot.
- **Parameters:**
  - `screenshot_path` (string): The path to the screenshot of the frontend changes.

### `start_live_preview_instructions`
Provides instructions on how to start a live preview server.

---

## External Information & Memory

### `google_search`
Performs an online Google search to retrieve up-to-date information.
- **Parameters:**
  - `query` (string): The query to search for.

### `view_text_website`
Fetches the content of a website as plain text.
- **Parameters:**
  - `url` (string): The URL of the website to fetch.

### `knowledgebase_lookup`
Retrieves information from the knowledgebase to help with tools, environment setup, or technical problems.
- **Parameters:**
  - `query` (string): A description of the problem or information needed.

### `initiate_memory_recording`
Starts recording information that will be useful for future tasks.

---

## Communication & Review

### `message_user`
Sends a statement to the user to provide an update or respond to feedback.
- **Parameters:**
  - `message` (string): The message to send.
  - `continue_working` (boolean): Whether the agent intends to perform more actions immediately.

### `request_user_input`
Asks the user a question or asks for input and waits for a response.
- **Parameters:**
  - `message` (string): The question or prompt for the user.

### `request_code_review`
Requests an independent code review for the current changes.

### `read_pr_comments`
Reads any pending pull request comments sent by the user.

### `reply_to_pr_comments`
Replies to pull request comments.
- **Parameters:**
  - `replies` (string): A JSON string representing a list of objects with `comment_id` and `reply`.

---

## Deprecated Tools

The following tools are deprecated and should be avoided in favor of modern alternatives:
- `grep`: Use `run_in_bash_session` with the standard `grep` command.
- `create_file_with_block`: Use `write_file` instead.
- `overwrite_file_with_block`: Use `write_file` instead.
