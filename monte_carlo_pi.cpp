#include <iostream>
#include <random>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <vector>
#include <cmath>

// Define M_PI if not available (Windows)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class MonteCarloPiEstimator {
private:
    std::mt19937 rng;
    std::uniform_real_distribution<double> dist;
    
public:
    MonteCarloPiEstimator(unsigned int seed = std::random_device{}()) 
        : rng(seed), dist(-1.0, 1.0) {}
    
    // Calculate π using Monte Carlo method
    double estimatePi(unsigned long long numSamples, bool showProgress = false) {
        unsigned long long pointsInside = 0;
        unsigned long long progressStep = numSamples / 10; // Show progress every 10%
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        for (unsigned long long i = 0; i < numSamples; ++i) {
            double x = dist(rng);
            double y = dist(rng);
            
            // Check if point is inside unit circle (x² + y² ≤ 1)
            if (x * x + y * y <= 1.0) {
                pointsInside++;
            }
            
            // Show progress if requested
            if (showProgress && progressStep > 0 && (i + 1) % progressStep == 0) {
                double currentEstimate = 4.0 * static_cast<double>(pointsInside) / (i + 1);
                std::cout << "Progress: " << std::fixed << std::setprecision(1) 
                         << (100.0 * (i + 1) / numSamples) << "% - "
                         << "Current π estimate: " << std::setprecision(6) << currentEstimate << std::endl;
            }
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        double piEstimate = 4.0 * static_cast<double>(pointsInside) / numSamples;
        
        std::cout << "\n=== Monte Carlo π Estimation Results ===" << std::endl;
        std::cout << "Number of samples: " << numSamples << std::endl;
        std::cout << "Points inside circle: " << pointsInside << std::endl;
        std::cout << "Estimated π: " << std::fixed << std::setprecision(10) << piEstimate << std::endl;
        std::cout << "Actual π: " << std::fixed << std::setprecision(10) << M_PI << std::endl;
        std::cout << "Absolute error: " << std::fixed << std::setprecision(10) << std::abs(piEstimate - M_PI) << std::endl;
        std::cout << "Relative error: " << std::fixed << std::setprecision(6) 
                 << (std::abs(piEstimate - M_PI) / M_PI) * 100 << "%" << std::endl;
        std::cout << "Computation time: " << duration.count() << " ms" << std::endl;
        
        return piEstimate;
    }
    
    // Generate intermediate estimates for plotting
    std::vector<std::pair<unsigned long long, double>> generateIntermediateEstimates(
        unsigned long long numSamples, unsigned long long stepSize) {
        
        std::vector<std::pair<unsigned long long, double>> estimates;
        unsigned long long pointsInside = 0;
        
        for (unsigned long long i = 0; i < numSamples; ++i) {
            double x = dist(rng);
            double y = dist(rng);
            
            if (x * x + y * y <= 1.0) {
                pointsInside++;
            }
            
            if ((i + 1) % stepSize == 0) {
                double currentEstimate = 4.0 * static_cast<double>(pointsInside) / (i + 1);
                estimates.emplace_back(i + 1, currentEstimate);
            }
        }
        
        return estimates;
    }
    
    // Save results to CSV file
    void saveResultsToCSV(const std::string& filename, unsigned long long numSamples, 
                         bool includeIntermediate = false) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filename << std::endl;
            return;
        }
        
        file << "Sample_Count,Pi_Estimate,Error" << std::endl;
        
        if (includeIntermediate) {
            unsigned long long stepSize = std::max(1ULL, numSamples / 1000); // 1000 data points max
            auto estimates = generateIntermediateEstimates(numSamples, stepSize);
            
            for (const auto& estimate : estimates) {
                double error = std::abs(estimate.second - M_PI);
                file << estimate.first << "," << std::fixed << std::setprecision(10) 
                     << estimate.second << "," << error << std::endl;
            }
        } else {
            // Just save final result
            double finalEstimate = estimatePi(numSamples, false);
            double error = std::abs(finalEstimate - M_PI);
            file << numSamples << "," << std::fixed << std::setprecision(10) 
                 << finalEstimate << "," << error << std::endl;
        }
        
        file.close();
        std::cout << "Results saved to " << filename << std::endl;
    }
};

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -n <number>    Number of samples (default: 1000000)" << std::endl;
    std::cout << "  -p             Show progress during computation" << std::endl;
    std::cout << "  -s <filename>  Save results to CSV file" << std::endl;
    std::cout << "  -i             Include intermediate estimates in CSV" << std::endl;
    std::cout << "  -h             Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << programName << " -n 1000000 -p" << std::endl;
    std::cout << "  " << programName << " -n 10000000 -s results.csv -i" << std::endl;
}

int main(int argc, char* argv[]) {
    unsigned long long numSamples = 1000000; // Default value
    bool showProgress = false;
    bool saveToFile = false;
    bool includeIntermediate = false;
    std::string filename = "monte_carlo_pi_results.csv";
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-n" && i + 1 < argc) {
            numSamples = std::stoull(argv[++i]);
        } else if (arg == "-p") {
            showProgress = true;
        } else if (arg == "-s" && i + 1 < argc) {
            filename = argv[++i];
            saveToFile = true;
        } else if (arg == "-i") {
            includeIntermediate = true;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    std::cout << "Monte Carlo π Estimator" << std::endl;
    std::cout << "=========================" << std::endl;
    
    // Create estimator and run simulation
    MonteCarloPiEstimator estimator;
    
    if (saveToFile) {
        estimator.saveResultsToCSV(filename, numSamples, includeIntermediate);
    } else {
        estimator.estimatePi(numSamples, showProgress);
    }
    
    return 0;
} 