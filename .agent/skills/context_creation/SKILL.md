---
name: context_creation
description: How to refresh the AI agent's understanding of the full codebase
---

# Context Creation

Use this skill if you need to refresh your understanding of the full codebase or provided a comprehensive context to another agent session.

## Command
Run the context creation script from the root directory:

```powershell
python scripts/create_context.py
```

## When to Use
- When starting a major new feature.
- When the codebase has undergone significant structural changes.
- When you are unsure about cross-module dependencies.

## Output
This script typically generates a consolidated file containing the relevant parts of the codebase, which can be read to gain a holistic view.
