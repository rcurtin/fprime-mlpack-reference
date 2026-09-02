# fprime-mlpack-reference project

This project demonstrates the use of the
[mlpack C++ machine learning library](https://www.mlpack.org/)
inside an F-Prime component.  The project implements a simple machine learning
approach for an anomaly detector.

As this is a minimal reference project, the anomaly detector simply watches CPU
utilization telemetry and indicates anomalies as events when the CPU utilization
differs significantly from idle.  (Try running something computationally
intensive while the F Prime deployment is running and see!)

The `KDEAnomalyDetector` component can be used as a starting point for a more
complex machine learning pipeline.  The use of mlpack is not limited to anomaly
detection; it can be used for any machine learning task.

## System Requirements

 * F Prime system requirements (listed [here](https://fprime.jpl.nasa.gov/latest/docs/getting-started/installing-fprime/#system-requirements))
 * A compiler that supports C++17 or newer
 * *Optional:* pre-installed mlpack (see [here](https://www.mlpack.org/download.html))
   - If mlpack is not pre-installed, it will be auto-downloaded by CMake

## Prerequisites

 * Follow the [Hello World Tutorial](https://fprime.jpl.nasa.gov/latest/tutorials-hello-world/docs/hello-world/) for basic understanding of F Prime component structure
 * Optionally, check out the [mlpack documentation](https://www.mlpack.org/doc/index.html) to get an idea of what the library can do

## General structure

Using mlpack inside of F Prime requires some minor CMake setup but is otherwise
a very simple affair: just `#include <mlpack.hpp>` and use the library.

For CMake configuration, two things are necessary:

 * [Add mlpack as a project dependency](TODO)
 * [Link components that use mlpack against OpenBLAS](TODO)

For our actual anomaly detector, we need two components:

 * [`TlmSplit`](TODO)
   - This is a telemetry splitter: it sends all received telemetry both to the
     anomaly detector and the regular telemetry recorder `TlmChan`.

 * [`KDEAnomalyDetector`](TODO)
   - This is the anomaly detector, which uses mlpack's `KDE` class internally.
   - This component provides a `RESET` command, which is used to train the
     anomaly detector on recently-recorded telemetry.
   - The anomaly detector runs on a clock... TODO

Then, we can define a very simple deployment...

TODO

## Running the anomaly detector

Once the deployment is built (TODO: provide some direction how), you can run
`fprime-gds` to start the system.

TODO: provide some information on how to train the model, and detect an anomaly.
