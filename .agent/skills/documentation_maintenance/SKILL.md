---
name: documentation_maintenance
description: Systematic process for scanning and updating project documentation
---

# Documentation Maintenance

Use this skill to ensure all relevant documentation reflects the changes made in the code.

## The Process

1. **Scan Documentation**
   - List all `.md` files in the project to identify potentially relevant documents.
   ```powershell
   fd -e md
   ```

2. **Identify Impacted Docs**
   - **Math/Physics Changes** → `docs/dev_docs/FFB_formulas.md`
   - **New FFB Effects** → `docs/ffb_effects.md`, `docs/the_physics_of__feel_-_driver_guide.md`
   - **Telemetry Usage** → `docs/dev_docs/references/Reference - telemetry_data_reference.md`
   - **GUI Changes** → `README.md`
   - **Architecture Changes** → `docs/architecture.md`
   - **Configuration Changes** → `docs/ffb_customization.md`

3. **Update Documents**
   - Modify the identified files to maintain consistency.
   - Do not assume only one document needs updating.

## Critical Directories
- `docs/`: User-facing docs.
- `docs/dev_docs/`: Technical/Developer docs.
- Root: `README.md`, `CHANGELOG_DEV.md`, `USER_CHANGELOG.md`.
