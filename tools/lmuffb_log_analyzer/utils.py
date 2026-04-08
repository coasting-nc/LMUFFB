import numpy as np

def safe_corrcoef(x, y):
    """
    Calculate Pearson correlation coefficient safely, returning 0.0 if variance is zero.
    """
    if len(x) < 2 or len(y) < 2:
        return 0.0

    # Check for zero variance
    if np.std(x) == 0 or np.std(y) == 0:
        return 0.0

    return np.corrcoef(x, y)[0, 1]
