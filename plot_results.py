#!/usr/bin/env python3
"""
Monte Carlo π Estimator Visualization Script

This script creates plots from CSV files generated by the Monte Carlo π estimator.
It can visualize convergence, error analysis, and performance metrics.
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import argparse
import sys
from pathlib import Path

def plot_convergence(csv_file, output_file=None):
    """Plot the convergence of π estimates over sample count."""
    try:
        df = pd.read_csv(csv_file)
    except FileNotFoundError:
        print(f"Error: File '{csv_file}' not found.")
        return
    except pd.errors.EmptyDataError:
        print(f"Error: File '{csv_file}' is empty.")
        return
    
    plt.figure(figsize=(12, 8))
    
    # Main convergence plot
    plt.subplot(2, 2, 1)
    plt.semilogx(df['Sample_Count'], df['Pi_Estimate'], 'b-', linewidth=1.5, label='Estimated π')
    plt.axhline(y=np.pi, color='r', linestyle='--', linewidth=2, label='Actual π')
    plt.xlabel('Number of Samples')
    plt.ylabel('π Estimate')
    plt.title('Monte Carlo π Convergence')
    plt.legend()
    plt.grid(True, alpha=0.3)
    
    # Error plot
    plt.subplot(2, 2, 2)
    plt.semilogx(df['Sample_Count'], df['Error'], 'g-', linewidth=1.5)
    plt.xlabel('Number of Samples')
    plt.ylabel('Absolute Error')
    plt.title('Absolute Error vs Sample Count')
    plt.grid(True, alpha=0.3)
    
    # Relative error plot
    plt.subplot(2, 2, 3)
    relative_error = (df['Error'] / np.pi) * 100
    plt.semilogx(df['Sample_Count'], relative_error, 'm-', linewidth=1.5)
    plt.xlabel('Number of Samples')
    plt.ylabel('Relative Error (%)')
    plt.title('Relative Error vs Sample Count')
    plt.grid(True, alpha=0.3)
    
    # Histogram of estimates (if we have enough data points)
    plt.subplot(2, 2, 4)
    if len(df) > 10:
        plt.hist(df['Pi_Estimate'], bins=min(20, len(df)//5), alpha=0.7, color='skyblue', edgecolor='black')
        plt.axvline(x=np.pi, color='r', linestyle='--', linewidth=2, label='Actual π')
        plt.xlabel('π Estimate')
        plt.ylabel('Frequency')
        plt.title('Distribution of π Estimates')
        plt.legend()
    else:
        plt.text(0.5, 0.5, 'Not enough data points\nfor histogram', 
                ha='center', va='center', transform=plt.gca().transAxes)
        plt.title('Distribution of π Estimates')
    
    plt.tight_layout()
    
    if output_file:
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"Plot saved to {output_file}")
    else:
        plt.show()

def plot_error_analysis(csv_file, output_file=None):
    """Plot detailed error analysis."""
    try:
        df = pd.read_csv(csv_file)
    except FileNotFoundError:
        print(f"Error: File '{csv_file}' not found.")
        return
    
    plt.figure(figsize=(15, 10))
    
    # Error convergence with confidence intervals
    plt.subplot(2, 3, 1)
    plt.semilogx(df['Sample_Count'], df['Error'], 'b-', linewidth=2, label='Absolute Error')
    
    # Theoretical error bound (1/sqrt(n))
    theoretical_error = 1.0 / np.sqrt(df['Sample_Count'])
    plt.semilogx(df['Sample_Count'], theoretical_error, 'r--', linewidth=2, label='Theoretical Bound (1/√n)')
    
    plt.xlabel('Number of Samples')
    plt.ylabel('Absolute Error')
    plt.title('Error Convergence Analysis')
    plt.legend()
    plt.grid(True, alpha=0.3)
    
    # Relative error
    plt.subplot(2, 3, 2)
    relative_error = (df['Error'] / np.pi) * 100
    plt.semilogx(df['Sample_Count'], relative_error, 'g-', linewidth=2)
    plt.xlabel('Number of Samples')
    plt.ylabel('Relative Error (%)')
    plt.title('Relative Error')
    plt.grid(True, alpha=0.3)
    
    # Error vs estimate scatter
    plt.subplot(2, 3, 3)
    plt.scatter(df['Pi_Estimate'], df['Error'], alpha=0.6, s=20)
    plt.xlabel('π Estimate')
    plt.ylabel('Absolute Error')
    plt.title('Error vs Estimate')
    plt.grid(True, alpha=0.3)
    
    # Convergence rate analysis
    plt.subplot(2, 3, 4)
    if len(df) > 1:
        # Calculate convergence rate
        log_samples = np.log(df['Sample_Count'])
        log_errors = np.log(df['Error'])
        
        # Fit linear regression
        coeffs = np.polyfit(log_samples, log_errors, 1)
        convergence_rate = -coeffs[0]  # Negative because error decreases
        
        plt.scatter(log_samples, log_errors, alpha=0.6, s=30)
        plt.plot(log_samples, np.polyval(coeffs, log_samples), 'r-', linewidth=2,
                label=f'Slope: {convergence_rate:.3f}')
        plt.xlabel('log(Number of Samples)')
        plt.ylabel('log(Absolute Error)')
        plt.title(f'Convergence Rate Analysis\nRate: {convergence_rate:.3f}')
        plt.legend()
        plt.grid(True, alpha=0.3)
    
    # Statistical summary
    plt.subplot(2, 3, 5)
    stats_text = f"""
    Statistical Summary:
    
    Final Estimate: {df['Pi_Estimate'].iloc[-1]:.10f}
    Actual π: {np.pi:.10f}
    Final Error: {df['Error'].iloc[-1]:.2e}
    Final Rel. Error: {relative_error.iloc[-1]:.6f}%
    
    Min Error: {df['Error'].min():.2e}
    Max Error: {df['Error'].max():.2e}
    Mean Error: {df['Error'].mean():.2e}
    Std Error: {df['Error'].std():.2e}
    """
    plt.text(0.1, 0.5, stats_text, transform=plt.gca().transAxes, 
             fontsize=10, verticalalignment='center', fontfamily='monospace')
    plt.axis('off')
    plt.title('Statistical Summary')
    
    # Sample efficiency
    plt.subplot(2, 3, 6)
    efficiency = 1.0 / (df['Sample_Count'] * df['Error']**2)
    plt.semilogx(df['Sample_Count'], efficiency, 'purple', linewidth=2)
    plt.xlabel('Number of Samples')
    plt.ylabel('Efficiency (1/(n·ε²))')
    plt.title('Sample Efficiency')
    plt.grid(True, alpha=0.3)
    
    plt.tight_layout()
    
    if output_file:
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"Error analysis plot saved to {output_file}")
    else:
        plt.show()

def create_summary_report(csv_file):
    """Create a text summary report of the Monte Carlo results."""
    try:
        df = pd.read_csv(csv_file)
    except FileNotFoundError:
        print(f"Error: File '{csv_file}' not found.")
        return
    
    print("\n" + "="*60)
    print("MONTE CARLO π ESTIMATION SUMMARY REPORT")
    print("="*60)
    
    final_estimate = df['Pi_Estimate'].iloc[-1]
    final_error = df['Error'].iloc[-1]
    relative_error = (final_error / np.pi) * 100
    
    print(f"File: {csv_file}")
    print(f"Total data points: {len(df)}")
    print(f"Sample range: {df['Sample_Count'].min():,} to {df['Sample_Count'].max():,}")
    print()
    
    print("FINAL RESULTS:")
    print(f"  Estimated π: {final_estimate:.10f}")
    print(f"  Actual π:    {np.pi:.10f}")
    print(f"  Absolute Error: {final_error:.2e}")
    print(f"  Relative Error: {relative_error:.6f}%")
    print()
    
    print("ERROR STATISTICS:")
    print(f"  Minimum Error: {df['Error'].min():.2e}")
    print(f"  Maximum Error: {df['Error'].max():.2e}")
    print(f"  Mean Error:    {df['Error'].mean():.2e}")
    print(f"  Std Deviation: {df['Error'].std():.2e}")
    print()
    
    print("ESTIMATE STATISTICS:")
    print(f"  Minimum Estimate: {df['Pi_Estimate'].min():.10f}")
    print(f"  Maximum Estimate: {df['Pi_Estimate'].max():.10f}")
    print(f"  Mean Estimate:    {df['Pi_Estimate'].mean():.10f}")
    print(f"  Std Deviation:    {df['Pi_Estimate'].std():.10f}")
    print()
    
    # Convergence analysis
    if len(df) > 1:
        log_samples = np.log(df['Sample_Count'])
        log_errors = np.log(df['Error'])
        coeffs = np.polyfit(log_samples, log_errors, 1)
        convergence_rate = -coeffs[0]
        
        print("CONVERGENCE ANALYSIS:")
        print(f"  Convergence Rate: {convergence_rate:.3f}")
        print(f"  Theoretical Rate: 0.500 (Monte Carlo)")
        print(f"  Rate Ratio: {convergence_rate/0.5:.3f}")
        print()
    
    print("="*60)

def main():
    parser = argparse.ArgumentParser(description='Visualize Monte Carlo π estimation results')
    parser.add_argument('csv_file', help='Input CSV file from Monte Carlo π estimator')
    parser.add_argument('-o', '--output', help='Output file for plots (e.g., plot.png)')
    parser.add_argument('-t', '--type', choices=['convergence', 'error', 'summary'], 
                       default='convergence', help='Type of visualization')
    parser.add_argument('--no-display', action='store_true', help='Don\'t display plots (save only)')
    
    args = parser.parse_args()
    
    if not Path(args.csv_file).exists():
        print(f"Error: File '{args.csv_file}' not found.")
        sys.exit(1)
    
    if args.type == 'convergence':
        plot_convergence(args.csv_file, args.output)
    elif args.type == 'error':
        plot_error_analysis(args.csv_file, args.output)
    elif args.type == 'summary':
        create_summary_report(args.csv_file)
    
    if args.output and not args.no_display:
        print(f"\nPlot saved to: {args.output}")
        print("You can also view it with: display", args.output)

if __name__ == "__main__":
    main() 