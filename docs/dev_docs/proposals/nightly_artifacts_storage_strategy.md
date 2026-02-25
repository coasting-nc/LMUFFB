# Strategy Report: Automated Nightly Artifact Storage & Distribution

## Overview
The goal is to provide a reliable, historical archive of automated "nightly" builds (Windows `.exe` packages) without cluttering the primary "Releases" page (reserved for stable versions) or bloating the Git repository history with binary data.

## Current Implementation (Phase 1)
As of the current configuration, the project uses a **Hybrid CI/CD approach**:
1.  **Windows Release Build**: Produced on every push.
2.  **Linux Coverage Report**: Produced on every push for high-quality LCOV metrics.
3.  **Nightly Hub (GitHub Pages)**: A central landing page that hosts:
    *   The **latest** nightly ZIP for immediate download.
    *   The **latest** code coverage report.
4.  **Short-term Retention**: GitHub Actions Artifacts store these builds for **90 days** (maximum for Free plan).

---

## Long-term Strategy: Historical Archive
Since GitHub Pages deployments overwrite the entire site, it does not natively maintain a "history" of files. Below are the evaluated strategies for implementing a multi-version archive.

### Strategy A: The GitHub API Approach (Internal)
Users can dynamically view older builds by fetching data from the GitHub Actions API.
*   **Mechanism**: A JavaScript script on the Pages site queries `api.github.com` for the last N successful runs.
*   **Pros**: Zero additional storage cost; no repository bloat.
*   **Cons**: **Universal Access Barrier.** GitHub requires users to be logged in to download artifacts via API links. This makes it unsuitable for sharing links with the general community.

### Strategy B: The "Object Storage" Approach (Recommended)
This is the most scalable and professional solution.
*   **Mechanism**: Builds are pushed to a dedicated Cloud Storage bucket (e.g., **Cloudflare R2**, **Backblaze B2**, or **AWS S3**).
*   **Implementation**:
    1.  Extend the workflow to upload the ZIP to the bucket using a tool like `rclone`.
    2.  Use a "Public Access" bucket for these specific artifacts.
    3.  The Nightly Hub (`index.html`) is updated to pull a JSON manifest from the bucket or simply link to the known URL pattern.
*   **Feasibility**: Very High. At **~700KB per build**, storing 1,000 versions requires only **~700MB**. Most free tiers (Cloudflare R2, B2) offer **10GB free**, allowing for years of history at zero cost.
*   **Verdict**: Best for user experience and repository health.

### Strategy C: The Dedicated Archive Branch (The "Git-LFS" or "Orphan" approach)
Storing binaries in a separate Git branch (e.g., `nightly-archive`).
*   **Mechanism**: A CI job checks out an orphan branch, adds the new binary, commits, and pushes.
*   **Pros**: Everything stays within GitHub; easy to index via shell scripts.
*   **Cons**: **Repository Bloat.** Even on a separate branch, the `.git` folder of anyone who clones the repo will eventually grow as it tracks the binary history. This is generally discouraged for long-term project health.

---

## Technical Feasibility Summary

| Feature | Action Artifacts | GitHub Releases | Dedicated Branch | Object Storage (R2/S3) |
| :--- | :--- | :--- | :--- | :--- |
| **Persistence** | 90 Days (Free) | Permanent | Permanent | Permanent |
| **Download Access** | Must be Logged In | Public | Public | Public |
| **Repo Size Impact** | None | None | **High** (over time) | None |
| **Custom Branding** | No | Limited | Yes (via Pages) | Yes (via Pages) |
| **Separation** | Good | **Poor** (mixes Nightly/Stable) | Good | Excellent |

## Recommendation for Future Implementation
To achieve a "Nightly Archive" that doesn't interfere with stable releases:
1.  **Maintain the Landing Page** on GitHub Pages as the user-facing "Hub."
2.  **Integrate Cloudflare R2 or Backblaze B2** for storage.
3.  Modify the CI to upload the versioned ZIP (`lmuffb-VERSION-nBUILD.zip`) to the bucket.
4.  Update the Hub's `index.html` to either link to the bucket's file listing or maintain a small `history.json` manifest.

This ensures the repository remains light, the stable release page remains clean, and power users can always find that one specific revert version they need.
