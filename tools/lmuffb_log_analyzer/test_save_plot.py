import pandas as pd
import numpy as np
from pathlib import Path
from lmuffb_log_analyzer.plots import plot_slip_vs_latg

def test_plot_slip_vs_latg_saves():
    df = pd.DataFrame({
        'CalcSlipAngleFront': np.random.uniform(0, 0.2, 100),
        'LatAccel': np.random.uniform(0, 10, 100),
        'Speed': np.random.uniform(10, 50, 100),
        'SlipRatioFL': np.zeros(100)
    })
    
    out_path = Path('test_out.png')
    res = plot_slip_vs_latg(df, str(out_path), show=False)
    
    if out_path.exists():
        print(f"File created: {out_path}")
        out_path.unlink()
    else:
        print("File NOT created!")
        print(f"Result returned: '{res}'")

if __name__ == '__main__':
    test_plot_slip_vs_latg_saves()
