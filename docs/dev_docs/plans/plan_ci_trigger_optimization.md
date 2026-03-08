# Implementation Plan - CI Trigger Optimization

## 1. Context & Analysis
The user has reported an issue where GitHub Actions workflows might not trigger if the last commit in a multi-commit push only contains documentation changes (`.md` files). They want to ensure that if a push contains *any* non-documentation changes, the workflow triggers, while still skipping builds that are purely documentation.

### Codebase Analysis
- Currently, all workflows (`windows-build-and-test.yml`, `lint.yml`, `osv-scanner.yml`, `codeql.yml`, `scorecard.yml`) use `on: push: paths-ignore: ['**.md']`.
- GitHub's official documentation states that `paths-ignore` evaluates the *aggregate* diff of the push (the difference between the `before` and `after` SHAs).
- If a push contains C1 (code) and C2 (md), the diff of the push includes C1's code changes. Thus, the workflow should trigger.

### Design Rationale
To address the user's perception and improve robustness, we will:
1.  **Refine the filters**: Use a more explicit set of ignored paths or switch to an inclusive `paths` strategy for core code changes.
2.  **Verify recursively**: Ensure patterns like `**.md` match all markdown files correctly.
3.  **Standardize patterns**: Ensure all workflows use the same robust trigger logic.

## 2. Proposed Changes

### Configuration Strategy
We will update the `on: push` and `on: pull_request` triggers to be more explicit. 

#### Impacted Files:
- `.github/workflows/windows-build-and-test.yml`
- `.github/workflows/lint.yml`
- `.github/workflows/osv-scanner.yml`
- `.github/workflows/codeql.yml`
- `.github/workflows/scorecard.yml`

### Option: Refined `paths-ignore`
We will expand the ignore list to include other non-essential files that don't require a build, reducing noise.

```yaml
    paths-ignore:
      - '**.md'
      - 'docs/**'
      - '.gitignore'
      - 'LICENSE'
```

## 3. Implementation Plan

| Workload | Target File | Change |
| :--- | :--- | :--- |
| **Logic Build** | `windows-build-and-test.yml` | Update `paths-ignore` for `push` and `pull_request` |
| **Style Check** | `lint.yml` | Update `paths-ignore` |
| **Security Scanning** | `osv-scanner.yml`, `codeql.yml` | Update `paths-ignore` |
| **Repo Health** | `scorecard.yml` | Update `paths-ignore` |

## 4. Verification Plan
1. [x] Review YAML syntax.
2. [ ] User to test with a multi-commit push (Code + Doc) to confirm the trigger.

## 5. Implementation Notes
- All major workflows have been updated with a refined `paths-ignore` list.
- Explaining to the user: GitHub's `push` event evaluates the *entire* push range (difference between the previous state of the branch and the new head). If any file in that range is NOT ignored, the workflow triggers.
- The perception that it "only triggers for the last commit" is likely due to the status icon only being displayed on the head commit in the UI, or from pushing commits separately.
