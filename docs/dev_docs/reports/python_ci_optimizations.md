# Optimizing Python Dependency Installation on Windows CI

After optimizing the C++ side of the LMUFFB build process, the `Run Python Tests` step—specifically the `pip install -r requirements.txt` phase on Windows runners—has surfaced as a noticeable bottleneck. This step currently downloads and installs heavyweight packages like `scipy`, `numpy`, `pandas`, and `matplotlib` on every single workflow run.

Below is an analysis and actionable recommendations to drastically optimize this step, bringing Python dependency setup times down from minutes to seconds.

## 1. Leverage Virtual Environments with Direct Caching (The Fastest Approach)
Standard `actions/setup-python` using `cache: 'pip'` caches the **downloaded `.whl` files** but `pip` still spends time extracting and installing those wheels. The most significant speedup comes from explicitly caching an entire Virtual Environment (`.venv`), allowing the workflow to completely bypass the `pip install` command on cache hits.

**Implementation Example:**
```yaml
      - name: Set up Python
        uses: actions/setup-python@v5
        with:
          python-version: '3.x'

      - name: Cache Virtual Environment
        id: cached-poetry-dependencies
        uses: actions/cache@v4
        with:
          path: .venv
          key: venv-${{ runner.os }}-${{ hashFiles('requirements.txt') }}

      - name: Install dependencies
        if: steps.cached-poetry-dependencies.outputs.cache-hit != 'true'
        shell: pwsh
        run: |
          python -m venv .venv
          .venv\Scripts\Activate.ps1
          pip install -r requirements.txt

      - name: Run Python Tests
        shell: pwsh
        run: |
          .venv\Scripts\Activate.ps1
          $env:PYTHONPATH = "$(Get-Location)/tools"
          python -m pytest tests tools/lmuffb_log_analyzer/tests
```
*Expected Speedup: 90-95% (Near instant on cache hit).*

## 2. Using Astral's `uv` Instead of `pip`
`uv` is an incredibly fast Python package installer tightly written in Rust. It frequently resolves and installs packages 10-100x faster than standard `pip`. You can run this natively without touching virtual environments, simply replacing `pip`.

**Implementation Example:**
```yaml
      - name: Install uv
        uses: astral-sh/setup-uv@v5
        with:
           enable-cache: true
           cache-dependency-glob: 'requirements.txt'

      - name: Run Python Tests
        shell: pwsh
        run: |
          uv pip install --system -r requirements.txt
          $env:PYTHONPATH = "$(Get-Location)/tools"
          python -m pytest tests tools/lmuffb_log_analyzer/tests
```
*Expected Speedup: 70-85% (Due to uv's concurrent resolution and robust extraction cache).*

## 3. Utilize `actions/setup-python` Pip Caching
If you prefer not to use `uv` or Virtual Environments, at a bare minimum you should be using the official caching mechanism from `actions/setup-python`. Right now, the Windows workflow relies entirely on pre-installed runner Python and downloads the packages across the network each time.

**Implementation Example:**
```yaml
      - name: Set up Python
        uses: actions/setup-python@v5
        with:
          python-version: '3.11' 
          cache: 'pip' # Will cache the pip downloads based on requirements.txt

      - name: Run Python Tests
        shell: pwsh
        run: |
          pip install -r requirements.txt
          $env:PYTHONPATH = "$(Get-Location)/tools"
          python -m pytest tests tools/lmuffb_log_analyzer/tests
```
*Expected Speedup: 40-60% (Only prevents networking/download delays; still extracts zips natively).*

## 4. Run Python Tests in a Parallel Job
Currently, the `windows-release` job is sequential: MSVC Setup -> Ccache -> C++ Build -> Defender Scan -> C++ Tests -> **Python Tests**.
If the Python tests do not explicitly depend on the compiled Windows `LMUFFB.exe` generated in the parallel flow, you could spin out the `Run Python Tests` step into an entirely *separate parallel job* (`python-tests`). 
Since the C++ build alone takes a substantial amount of time, standardizing the Python tests parallel to it would mean the Python setup times are effectively "free" as they finish well before the C++ build finishes.

If the tests *do* require the DLL/Exe compiled in that run, this would require `actions/upload-artifact` and downloading it across jobs, which might add latency, offsetting the parallelization benefits.

## 5. Dependency Split (Optional)
The `requirements.txt` currently mandates `scipy`, `pandas`, `matplotlib`, and `pydantic`. The log analyzer needs these, but do the core tests under `tests/` natively need graphical stack installations? 
If there are two distinct realms:
1. `requirements-core-test.txt` (`pytest`)
2. `requirements-tools.txt` (`scipy`, `pandas`, etc.)

You could avoid fetching the heaviest UI/Data Science elements for some steps (like the Linux Coverage run) yielding further savings, or using `pytest` markers (`-m "not analyzer"`) if you are trying to rapidly loop core component changes.

## Recommended Action
For minimal intrusion and maximum speed on the Windows runner, I recommend moving to **Approach 2 (using `uv`)** or **Approach 1 (Caching the `.venv`)**. Either method requires less than 10 lines of YAML modification and will immediately alleviate the Python installation bottleneck.

## Approach Compatibility (Orthogonality vs Incompatibility)

When deciding how to combine these strategies, it's important to recognize which ones stack for maximum performance and which ones conflict or render each other redundant.

### Mutually Exclusive (Incompatible)
The caching approaches for dependencies are largely mutually exclusive. You should only choose **one** of the following core caching mechanisms to avoid duplicated efforts or pipeline conflicts:
- **Approach 1 (Virtual Environment Caching)** vs **Approach 2 (`uv`)** vs **Approach 3 (`setup-python` Pip Caching)**.
  - *Context*: If you cache the `.venv` (Approach 1), you bypass `pip` entirely on a cache hit, rendering `uv` or `setup-python` caching redundant. If you use `uv`, its internal cache makes manual Virtual Environment caching unnecessary overhead. Pick the one that fits your ecosystem best (we implemented **Approach 1** coupled with **Approach 4**). 

### Orthogonal (Cumulative)
The architectural optimizations can be paired directly with your chosen caching mechanism to compound performance gains.

- **Approach 4 (Parallel Job)** + **Any Caching Strategy (1, 2, or 3)**:
  - *Context*: Running the Python tests in parallel separates the runtime from the lengthy C++ build. Adding `.venv` caching (Approach 1) inside that parallel job ensures the job starts *and* finishes instantly. They are perfectly complementary.
- **Approach 5 (Dependency Split)** + **Any Caching Strategy** + **Approach 4**:
  - *Context*: Splitting `requirements.txt` into core testing and UI/analytics components is universally beneficial. Trimming the dependency tree makes any cache step smaller and faster, and requires less download bandwidth on cache misses, entirely orthogonal to whether the job is running in parallel or sequentially.
