import pytest
import numpy as np
import pandas as pd
import warnings
from lmuffb_log_analyzer.utils import safe_corrcoef, find_invalid_signals

def test_safe_corrcoef_nan_inf():
    # Test that NaNs and Infs return 0.0 and don't raise RuntimeWarning
    x_nan = np.array([1.0, 2.0, np.nan])
    y = np.array([1.0, 2.0, 3.0])

    with warnings.catch_warnings(record=True) as w:
        warnings.simplefilter("always")
        assert safe_corrcoef(x_nan, y) == 0.0
        assert safe_corrcoef(y, x_nan) == 0.0

        x_inf = np.array([1.0, 2.0, np.inf])
        assert safe_corrcoef(x_inf, y) == 0.0

        # Ensure no RuntimeWarning was triggered
        for warning in w:
            assert not issubclass(warning.category, RuntimeWarning)

def test_find_invalid_signals():
    df = pd.DataFrame({
        'clean': [1.0, 2.0, 3.0],
        'has_nan': [1.0, np.nan, 3.0],
        'has_inf': [1.0, np.inf, 3.0],
        'mixed': [np.nan, np.inf, 1.0]
    })

    invalid = find_invalid_signals(df, ['clean', 'has_nan', 'has_inf', 'mixed'])
    assert 'clean' not in invalid
    assert 'has_nan' in invalid
    assert 'has_inf' in invalid
    assert 'mixed' in invalid
    assert len(invalid) == 3

def test_safe_corrcoef_zero_variance_suppression():
    # Standard np.corrcoef triggers warning on zero variance
    x = np.array([1.0, 1.0, 1.0])
    y = np.array([1.0, 2.0, 3.0])

    with warnings.catch_warnings(record=True) as w:
        warnings.simplefilter("always")
        res = safe_corrcoef(x, y)
        assert res == 0.0
        # Ensure no RuntimeWarning was triggered
        for warning in w:
            assert not issubclass(warning.category, RuntimeWarning)
