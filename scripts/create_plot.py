import os
import sys

import matplotlib.pyplot as plt


def read_data(filename):
    """
    Read data from CSV file.

    Args:
        filename (str): Path to the CSV file

    Returns:
        tuple: (threads, speedup) lists containing the data
    """
    threads = []
    speedup = []

    try:
        with open(filename, "r") as file:
            # Skip header line
            file.readline().strip()

            for line_num, line in enumerate(file, start=2):
                line = line.strip()
                if not line:  # Skip empty lines
                    continue

                # Split by semicolon and remove empty strings
                parts = [part.strip() for part in line.split(";") if part.strip()]

                if len(parts) >= 3:
                    try:
                        t = int(parts[0])
                        s = float(parts[2])
                        threads.append(t)
                        speedup.append(s)
                    except ValueError:
                        print(f"Warning: Could not parse line {line_num}: {line}")
                        continue
                else:
                    print(f"Warning: Invalid format at line {line_num}: {line}")

    except FileNotFoundError:
        print(f"Error: File '{filename}' not found.")
        sys.exit(1)
    except Exception as e:
        print(f"Error reading file: {e}")
        sys.exit(1)

    if not threads:
        print("Error: No valid data found in the file.")
        sys.exit(1)

    return threads, speedup


def create_plot(threads, speedup, name: str):
    """
    Create speedup plot with ideal line.

    Args:
        threads (list): List of thread counts
        speedup (list): List of speedup values
    """
    # Create figure and axis
    fig, ax = plt.subplots(figsize=(10, 6))

    # Plot actual speedup data
    ax.plot(
        threads,
        speedup,
        "o-",
        linewidth=2,
        markersize=8,
        label="Actual Speedup",
        color="blue",
    )

    # Plot ideal speedup line (y = x)
    max_threads = max(threads)
    ideal_threads = [0, max_threads]
    ideal_speedup = [0, max_threads]
    ax.plot(
        ideal_threads,
        ideal_speedup,
        "--",
        linewidth=2,
        label="Ideal Speedup",
        color="red",
        alpha=0.7,
    )

    # Set labels and title
    ax.set_xlabel("Number of Threads", fontsize=12, fontweight="bold")
    ax.set_ylabel("Speedup", fontsize=12, fontweight="bold")
    ax.set_title("Speedup Analysis", fontsize=14, fontweight="bold")

    # Add grid
    ax.grid(True, alpha=0.3, linestyle="--")

    # Add legend
    ax.legend(loc="upper left", fontsize=10)

    # Set axis limits
    ax.set_xlim(0, max_threads * 1.05)
    ax.set_ylim(0, max(speedup) * 1.05)

    # Add value labels on data points
    for i, (t, s) in enumerate(zip(threads, speedup)):
        ax.annotate(
            f"{s:.2f}",
            (t, s),
            textcoords="offset points",
            xytext=(5, 5),
            ha="center",
            fontsize=8,
        )

    # Set tick parameters
    ax.tick_params(axis="both", which="major", labelsize=10)

    plt.tight_layout()  # Adjust layout
    plt.savefig(name)


def main():
    """
    Main function to read CSV file and create plot.

    Usage: python create_plot.py <filename>
    """
    # Check command line arguments
    if len(sys.argv) != 2:
        print("Usage: python create_plot.py <filename>")
        print("Example: python create_plot.py data.csv")
        sys.exit(1)

    filename = sys.argv[1]

    # Check if file exists
    if not os.path.exists(filename):
        print(f"Error: File '{filename}' does not exist.")
        sys.exit(1)

    # Read data from file
    print(f"Reading data from {filename}...")
    threads, speedup = read_data(filename)

    print(f"Found {len(threads)} data points")
    print(f"Threads: {threads}")
    print(f"Speedup: {[f'{s:.3f}' for s in speedup]}")

    # Create plot
    print("Creating plot...")
    create_plot(threads, speedup, f"{filename}.png")

    print("Done!")


if __name__ == "__main__":
    main()
