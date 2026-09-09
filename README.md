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

 * [Add mlpack as a project dependency](https://github.com/rcurtin/fprime-mlpack-reference/blob/master/CMakeLists.txt#L20)
 * [Link components that use mlpack against OpenBLAS](https://github.com/rcurtin/fprime-mlpack-reference/blob/master/Components/KDEAnomalyDetector/CMakeLists.txt#L22)

For our example anomaly detector, we need two components that are implemented in
this repository:

 * [`TlmSplit`](https://github.com/rcurtin/fprime-mlpack-reference/tree/master/Components/TlmSplitter)
   - This is a telemetry splitter: it sends all received telemetry both to the
     anomaly detector and the regular telemetry recorder `TlmChan`.

 * [`KDEAnomalyDetector`](https://github.com/rcurtin/fprime-mlpack-reference/tree/master/Components/KDEAnomalyDetector)
   - This is the anomaly detector, which uses mlpack's `KDE` class internally.
   - This component provides a `RESET` command, which is used to train the
     anomaly detector on recently-recorded telemetry.
   - The anomaly detector runs on a clock via the `run` input port.

Then, this repository has a simple example deployment:

 * [`ExampleDeployment`](https://github.com/rcurtin/fprime-mlpack-reference/tree/master/ExampleDeployment)
   - The [`instances.fpp`](https://github.com/rcurtin/fprime-mlpack-reference/blob/master/ExampleDeployment/Top/instances.fpp)
     file defines a `TlmSplit` and `KDEAnomalyDetector` component.

   - The [`topology.fpp`](https://github.com/rcurtin/fprime-mlpack-reference/blob/master/ExampleDeployment/Top/topology.fpp)
     file routes all telemetry to `TlmSplit`, which then routes it to the
     anomaly detector.  The anomaly detector is placed on the 1 Hz run group,
     so, it runs once per second.

## Building the anomaly detector

Once you have `fprime-bootstrap` installed, clone the project like this:

```sh
fprime-bootstrap clone https://github.com/rcurtin/fprime-mlpack-reference
```

and then you can use the virtual environment created by that process to actually
build the project:

```sh
cd fprime-mlpack-reference/
source fprime-venv/bin/activate
fprime-util generate
fprime-util build
```

## Running the anomaly detector

Once the deployment is built, you can run `fprime-gds` to start the system.

Once the GDS is open and you see the green circle in the upper right hand
corner, the system will be recording historical telemetry.  You can let it sit
for a while to gather historical data---usually a few minutes is enough.
Remember that we are modeling what "normal" is on the system, so, you might want
to kill any heavy computation that is going on before you start `fprime-gds`.

Once you've waited for a while, you can send the
`ExampleDeployment.anomalyDetector.RESET` command.  This will build a KDE model
on all of the collected historical data.

After training, you can switch to the "Channels" tab to see that the anomaly
detector is now producing telemetry, under the prefix
`ExampleDeployment.anomalyDetector`.  Of primary interest is the
`CURRENT_DENSITY` telemetry channel, which tells you what the estimated density
of the current state of telemetry is.  When this drops below 1e-6, then
anomalies will be recorded in the "Events" tab.

So, once things are trained, try doing something anomalous on the computer
running the deployment---perhaps, building a big project, or running some kind
of stress testing like `mprime` or
[`stress`](https://linux.die.net/man/1/stress).  You should, after a few
seconds, see anomalies being reported every second until you kill whatever
process is using all the CPUs.

## Tuning the anomaly detector

Of course, this example deployment here is tuned specifically for detecting
anomalies *only* using CPU utilization.  A real mission would want to use other
telemetry channels.

The code in [`KDEAnomalyDetector.cpp`](https://github.com/rcurtin/fprime-mlpack-reference/blob/master/Components/KDEAnomalyDetector/KDEAnomalyDetector.cpp)
has comments about where to add more telemetry channels, and the component also
comes with two tuning parameters that can be set as commands:

 * `ANOMALY_THRESHOLD_PRM_SET`: sets the threshold for what density is
   considered an anomaly; defaults to `1e-6`.  If the density is lower than
   this, then an anomaly is reported.

 * `ANOMALY_SAMPLES_BEFORE_ALARM_PRM_SET`: sets the number of consecutive times
   that the current density must be below the threshold for an anomaly to be
   reported.  Defaults to `5`.

 * `MAX_TRAINING_WINDOW_PARAM_SECS_PRM_SET`: sets the maximum length of
   historical telemetry that is used to train the model.

The algorithmic approach itself can also be tuned:

 * `LEAF_SIZE_PRM_SET`: mlpack's `KDE` class uses a
   [kd-tree](https://mlpack.org/doc/user/core/trees/kdtree.html)
   to accelerate the computation of density estimates, and the kd-tree is built
   such that the maximum number of points in a leaf is this value.  Smaller
   values can result in more accurate density estimates, but at the cost of a
   little computational overhead.  Default `20`.

 * `KERNEL_BW_PRM_SET`: kernel density estimation depends strongly on the
   bandwidth of the kernel used.  The larger this value is, the larger the area
   around a particular point that is non-anomalous is.

## Final notes and further reference

This effort is a simplified version of the
[mlpack-fprime-robot](https://github.com/SterlingPeet/mlpack-fprime-robot)
repository, which was presented at SmallSat 2026.  Refer to that repository and
the paper for more details:

 * "Lightweight Open Source On-Spacecraft Machine Learning with mlpack and F
   Prime".
   Ryan R. Curtin, Scott M. Gilliland, Sterling Peet.
   SmallSat 2026;
   [paper](https://digitalcommons.usu.edu/smallsat/2026/all2026/66/),
   [slides](https://www.ratml.org/misc/smallsat2026.pdf).
