# kuserspace - Advanced System Information Library

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](https://github.com/malghumuy/kuserspace)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/std/the-standard)
[![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)](https://www.linux.org/)

> **Note**: This library is under active development. New features and optimizations are being added regularly.

## Overview

kuserspace is a high-performance C++ library for retrieving and monitoring system information on Linux systems. It provides thread-safe, efficient access to CPU, memory, and system statistics with minimal overhead.

### Key Features

- **Thread-Safe Operations**: All operations are designed for concurrent access
- **Memory Efficient**: Zero-copy operations where possible
- **Real-time Monitoring**: Continuous system monitoring with callback support
- **Asynchronous Operations**: Non-blocking API for high-performance applications
- **Extensible Architecture**: Easy to extend with new system information sources

## Installation

### Prerequisites

- C++17 or later
- CMake 3.10 or later
- Linux system
- GCC 7.0+ or Clang 5.0+

### Building from Source

```bash
git clone https://github.com/malghumuy/kuserspace.git
cd kuserspace
mkdir build && cd build
cmake ..
make
sudo make install
```

## Quick Start

```cpp
#include <kuserspace/Memory.h>
#include <kuserspace/Processor.h>

int main() {
    auto& memory = kuserspace::Memory::getInstance();
    auto& processor = kuserspace::Processor::getInstance();

    // Get memory statistics
    auto memStats = memory.getStats();
    std::cout << "Memory Usage: " << memStats.used << " / " << memStats.total << std::endl;

    // Get CPU information
    auto cpuInfo = processor.getStats();
    std::cout << "CPU Usage: " << cpuInfo.totalUtilization << "%" << std::endl;

    return 0;
}
```

## Advanced Features

### Memory Management

```cpp
// Continuous memory monitoring
memory.startContinuousMonitoring([](const Memory::Stats& stats) {
    // Handle memory updates
});

// Asynchronous statistics retrieval
auto future = memory.getStatsAsync();
auto stats = future.get();
```

### CPU Information

```cpp
// Get detailed CPU information
for (const auto& core : processor.getAllCores()) {
    std::cout << "Core " << core.id << ": " 
              << core.utilization << "% utilization" << std::endl;
}

// Monitor CPU temperature
auto temps = processor.getTemperatures();
```

### System Monitoring

```cpp
// Combined system monitoring
void monitorCallback(const SystemStats& stats) {
    // Handle system-wide statistics
}
```

## Performance Considerations

- **Memory Usage**: ~2MB resident memory
- **CPU Overhead**: <1% during continuous monitoring
- **Thread Safety**: All operations are thread-safe
- **Cache Efficiency**: Optimized for modern CPU architectures

## Production Usage

### Best Practices

1. **Error Handling**
   ```cpp
   try {
       auto stats = memory.getStats();
   } catch (const std::exception& e) {
       // Handle errors appropriately
   }
   ```

2. **Resource Management**
   ```cpp
   // Always stop monitoring when done
   memory.startContinuousMonitoring(callback);
   // ... do work ...
   memory.stopMonitoring();
   ```

3. **Performance Tuning**
   ```cpp
   // Adjust monitoring interval for your needs
   memory.setMonitoringInterval(std::chrono::milliseconds(100));
   ```

### Production Checklist

- [ ] Implement proper error handling
- [ ] Set appropriate monitoring intervals
- [ ] Handle resource cleanup
- [ ] Monitor library performance
- [ ] Implement logging strategy
- [ ] Set up monitoring alerts

## Contributing

All contributions are welcomed! I don't have any [Contrinuting Guide] or any further details yet. 

### Development Status

- [x] Core functionality
- [x] Thread safety
- [x] Performance optimizations
- [x] Documentation
- [ ] Additional system metrics
- [ ] More platform support
- [ ] Extended monitoring capabilities

## Roadmap

- [ ] GPU monitoring support
- [ ] Network statistics
- [ ] Disk I/O monitoring
- [ ] Process-specific statistics
- [ ] Container support
- [ ] More platform support

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

**Note**: This library is actively maintained. Any updates will be in releases or changelog.