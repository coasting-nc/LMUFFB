import click
from pathlib import Path
from rich.console import Console
from rich.table import Table
from rich.panel import Panel

from .loader import load_log
from .analyzers.slope_analyzer import analyze_slope_stability, detect_oscillation_events
from .plots import plot_slope_timeseries, plot_slip_vs_latg, plot_dalpha_histogram
from .reports import generate_text_report

console = Console()

@click.group()
@click.version_option(version='1.0.0')
def cli():
    """lmuFFB Log Analyzer - Analyze FFB telemetry logs for diagnostics."""
    pass

@cli.command()
@click.argument('logfile', type=click.Path(exists=True))
def info(logfile):
    """Display session info from a log file."""
    try:
        metadata, df = load_log(logfile)
        
        console.print(Panel.fit(
            f"[bold blue]Session Information[/bold blue]\n\n"
            f"Driver: {metadata.driver_name}\n"
            f"Vehicle: {metadata.vehicle_name}\n"
            f"Track: {metadata.track_name}\n"
            f"Duration: {df['Time'].max():.1f} seconds\n"
            f"Frames: {len(df)}\n"
            f"App Version: {metadata.app_version}",
            title="Log File Info"
        ))
    except Exception as e:
        console.print(f"[bold red]Error loading log:[/bold red] {e}")

@cli.command()
@click.argument('logfile', type=click.Path(exists=True))
@click.option('--verbose', '-v', is_flag=True, help='Show detailed output')
def analyze(logfile, verbose):
    """Analyze a log file and show summary."""
    console.print(f"[bold]Analyzing:[/bold] {logfile}")
    
    try:
        metadata, df = load_log(logfile)
        
        # Run slope analysis
        slope_results = analyze_slope_stability(df)
        oscillations = detect_oscillation_events(df)
        
        # Display results
        table = Table(title="Slope Detection Analysis")
        table.add_column("Metric", style="cyan")
        table.add_column("Value", style="green")
        table.add_column("Status", style="yellow")
        
        table.add_row(
            "Slope Std Dev",
            f"{slope_results['slope_std']:.2f}",
            "HIGH" if slope_results['slope_std'] > 5.0 else "OK"
        )
        table.add_row(
            "Slope Range",
            f"{slope_results['slope_min']:.1f} to {slope_results['slope_max']:.1f}",
            "WIDE" if (slope_results['slope_max'] - slope_results['slope_min']) > 20 else "OK"
        )
        
        if slope_results.get('active_percentage') is not None:
            table.add_row(
                "Active %",
                f"{slope_results['active_percentage']:.1f}%",
                "LOW" if slope_results['active_percentage'] < 30 else "OK"
            )
            
        if slope_results.get('floor_percentage') is not None:
            table.add_row(
                "Floor Hits",
                f"{slope_results['floor_percentage']:.1f}%",
                "HIGH" if slope_results['floor_percentage'] > 5 else "OK"
            )
            
        table.add_row(
            "Oscillation Events",
            str(len(oscillations)),
            "MANY" if len(oscillations) > 3 else "OK"
        )
        
        console.print(table)
        
        # Show issues
        if slope_results['issues']:
            console.print("\n[bold red]Issues Detected:[/bold red]")
            for issue in slope_results['issues']:
                console.print(f"  • {issue}")
        else:
            console.print("\n[bold green]No issues detected in slope analysis.[/bold green]")
            
    except Exception as e:
        console.print(f"[bold red]Error analyzing log:[/bold red] {e}")

@cli.command()
@click.argument('logfile', type=click.Path(exists=True))
@click.option('--output', '-o', default='.', help='Output directory for plots')
@click.option('--all', 'plot_all', is_flag=True, help='Generate all plot types')
def plots(logfile, output, plot_all):
    """Generate diagnostic plots from a log file."""
    console.print(f"[bold]Generating plots for:[/bold] {logfile}")
    
    try:
        metadata, df = load_log(logfile)
        output_dir = Path(output)
        output_dir.mkdir(parents=True, exist_ok=True)
        
        base_name = Path(logfile).stem
        
        # Time series plot
        ts_path = output_dir / f"{base_name}_timeseries.png"
        plot_slope_timeseries(df, str(ts_path), show=False)
        console.print(f"  [OK] Created: {ts_path}")
        
        if plot_all:
            # Tire curve
            tc_path = output_dir / f"{base_name}_tire_curve.png"
            plot_slip_vs_latg(df, str(tc_path), show=False)
            console.print(f"  [OK] Created: {tc_path}")
            
            # dAlpha histogram
            hist_path = output_dir / f"{base_name}_dalpha_hist.png"
            plot_dalpha_histogram(df, str(hist_path), show=False)
            console.print(f"  [OK] Created: {hist_path}")
        
        console.print("\n[bold green]Done![/bold green]")
    except Exception as e:
        console.print(f"[bold red]Error generating plots:[/bold red] {e}")

@cli.command()
@click.argument('logfile', type=click.Path(exists=True))
@click.option('--output', '-o', help='Output file path')
def report(logfile, output):
    """Generate a full diagnostic report."""
    try:
        metadata, df = load_log(logfile)
        
        report_text = generate_text_report(metadata, df)
        
        if output:
            with open(output, 'w') as f:
                f.write(report_text)
            console.print(f"[bold green]Report saved to:[/bold green] {output}")
        else:
            console.print(report_text)
    except Exception as e:
        console.print(f"[bold red]Error generating report:[/bold red] {e}")

@cli.command()
@click.argument('logdir', type=click.Path(exists=True, file_okay=False, dir_okay=True))
@click.option('--output', '-o', default='analyzer_results', help='Output directory for batch results')
@click.pass_context
def batch(ctx, logdir, output):
    """Run all analysis commands for all log files in a directory."""
    log_path = Path(logdir)
    output_path = Path(output)
    output_path.mkdir(parents=True, exist_ok=True)
    
    csv_files = sorted(list(log_path.glob("*.csv")))
    if not csv_files:
        console.print(f"[yellow]No .csv files found in {logdir}[/yellow]")
        return
        
    console.print(f"[bold green]Found {len(csv_files)} log files. Starting batch processing...[/bold green]")
    
    for logfile in csv_files:
        console.print(f"\n[bold blue]Processing: {logfile.name}[/bold blue]")
        
        # Run individual commands
        # 1. Info
        ctx.invoke(info, logfile=str(logfile))
        
        # 2. Analyze
        ctx.invoke(analyze, logfile=str(logfile), verbose=False)
        
        # 3. Plots (save to output dir)
        ctx.invoke(plots, logfile=str(logfile), output=str(output_path), plot_all=True)
        
        # 4. Report (save to output dir)
        report_file = output_path / f"{logfile.stem}_report.txt"
        ctx.invoke(report, logfile=str(logfile), output=str(report_file))

    console.print(f"\n[bold green]Batch processing complete! Results saved to: {output}[/bold green]")

if __name__ == '__main__':
    cli()
