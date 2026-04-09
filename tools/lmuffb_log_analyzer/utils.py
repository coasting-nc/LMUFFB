import numpy as np

def safe_corrcoef(x, y):
    """
    Calculate Pearson correlation coefficient safely, returning 0.0 if variance is zero,
    or if non-finite values (NaN, Inf) are present. Suppresses NumPy runtime warnings.
    """
    if len(x) < 2 or len(y) < 2:
        return 0.0

    # Ensure inputs are numpy arrays
    x = np.asarray(x)
    y = np.asarray(y)

    # Check for non-finite values (NaN, Inf)
    if not np.all(np.isfinite(x)) or not np.all(np.isfinite(y)):
        return 0.0

    # Check for zero variance
    if np.std(x) == 0 or np.std(y) == 0:
        return 0.0

    # Suppress warnings from np.corrcoef (e.g. if we missed a zero variance case due to precision)
    with np.errstate(divide='ignore', invalid='ignore'):
        c = np.corrcoef(x, y)
        if c.shape == (2, 2):
            val = c[0, 1]
            return val if np.isfinite(val) else 0.0
        return 0.0

def find_invalid_signals(df, columns):
    """
    Identify columns in a DataFrame that contain non-finite values (NaN or Inf).
    Returns a list of column names.
    """
    invalid_cols = []
    for col in columns:
        if col in df.columns:
            if not np.all(np.isfinite(df[col])):
                invalid_cols.append(col)
    return invalid_cols
