import os
import shutil
from pathlib import Path
from click.testing import CliRunner
from lmuffb_log_analyzer.cli import cli

def test_batch_command(tmp_path):
    # Setup test directory with a few log files
    log_dir = tmp_path / "logs"
    log_dir.mkdir()
    
    # Use real sample log path
    sample_log = Path(__file__).parent / "sample_log.csv"
    shutil.copy(sample_log, log_dir / "log1.csv")
    shutil.copy(sample_log, log_dir / "log2.csv")
    
    output_dir = tmp_path / "results"
    
    runner = CliRunner()
    
    # Run batch command
    result = runner.invoke(cli, ['batch', str(log_dir), '--output', str(output_dir)])
    
    assert result.exit_code == 0
    assert "Found 2 log files" in result.output
    assert "Processing: log1.csv" in result.output
    assert "Processing: log2.csv" in result.output
    
    # Check if files were created in the output directory
    assert (output_dir / "log1_report.txt").exists()
    assert (output_dir / "log1_timeseries.png").exists()
    assert (output_dir / "log2_report.txt").exists()
    assert (output_dir / "log2_timeseries.png").exists()
