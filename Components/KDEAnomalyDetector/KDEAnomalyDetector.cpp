// ======================================================================
// \title  KDEAnomalyDetector.cpp
// \author ryan
// \brief  cpp file for KDEAnomalyDetector component implementation class
// ======================================================================

#include "Components/KDEAnomalyDetector/KDEAnomalyDetector.hpp"

namespace Components {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

KDEAnomalyDetector ::KDEAnomalyDetector(const char* const compName) :
    KDEAnomalyDetectorComponentBase(compName),
    anomalySamples(0) {

    // When we receive telemetry, we need to determine whether it should be used
    // by the model, and if we do need to use it, it will be one of the
    // dimensions in the matrix we assemble.  This mapping below specifices
    // which telemetry channels get mapped to which dimension.

    // Svc::SystemResources: the base ID of 0x10012000 comes directly from our
    // topology.  All of these telemetry readings have type F32.  We ignore the
    // MEMORY_TOTAL and MEMORY_USED telemetry channels.
    dimMap[0x10012005] = 0; // CPU
    dimMap[0x10012006] = 1; // CPU1
    dimMap[0x10012007] = 2; // CPU2
    dimMap[0x10012008] = 3; // CPU3
    dimMap[0x10012009] = 4; // CPU4
    dimMap[0x1001200a] = 5; // CPU5
    dimMap[0x1001200b] = 6; // CPU6
    dimMap[0x1001200c] = 7; // CPU7
    dimMap[0x1001200d] = 8; // CPU8
    dimMap[0x1001200e] = 9; // CPU9
    dimMap[0x1001200f] = 10; // CPU10
    dimMap[0x10012010] = 11; // CPU11
    dimMap[0x10012011] = 12; // CPU12
    dimMap[0x10012012] = 13; // CPU13
    dimMap[0x10012013] = 14; // CPU14
    dimMap[0x10012014] = 15; // CPU15

    tlmCache.resize(this->numDims);
}

KDEAnomalyDetector ::~KDEAnomalyDetector() {}

// ----------------------------------------------------------------------
// Run the anomaly detector to see if there is an anomaly.
// ----------------------------------------------------------------------

void KDEAnomalyDetector ::run_handler(FwIndexType portNum, U32 context) {

    // If no model is trained, we can't do any anomaly detection.
    if (!this->kde.IsTrained()) {
        return;
    }

    // Construct the most recent values of the telemetry.
    arma::vec::fixed<this->numDims> point;
    for (size_t d = 0; d < this->numDims; ++d) {
        if (this->tlmCache[d].size() == 0) {
            point[d] = 0.0;
        } else {
            point[d] = (--this->tlmCache[d].end())->second;
        }
    }

    // NOTE: we don't do any further preprocessing of points, because all the
    // CPU utilization values are between 0 and 1.  In a more complex telemetry
    // setup, you might want to normalize.  See the mlpack-fprime-robot example,
    // which uses this same technique on real hardware sensors:
    //
    // https://github.com/SterlingPeet/mlpack-fprime-robot/blob/main/Components/KDEAnomalyDetector/KDEAnomalyDetector.cpp#L126

    // Compute the density estimate.
    arma::vec::fixed<1> estimate;
    this->kde.Evaluate(point, estimate);
    this->tlmWrite_CURRENT_DENSITY(F64(estimate[0]));

    // Get the parameters for anomaly detection.
    Fw::ParamValid isValid;
    F64 threshold = this->paramGet_ANOMALY_THRESHOLD(isValid);
    if (isValid == Fw::ParamValid::INVALID || isValid == Fw::ParamValid::UNINIT) {
        threshold = 1e-6;
    }

    U64 samplesBeforeTrigger = this->paramGet_ANOMALY_SAMPLES_BEFORE_ALARM(isValid);
    if (isValid == Fw::ParamValid::INVALID || isValid == Fw::ParamValid::UNINIT) {
        samplesBeforeTrigger = 5;
    }

    // Now check if we have an anomaly.
    if (estimate[0] < threshold) {
      // We have an anomaly!
      ++anomalySamples;

      if (anomalySamples >= samplesBeforeTrigger) {
          // We have seen enough of this anomaly to raise an alarm!
          this->log_ACTIVITY_HI_AnomalyDetectedEvent(F64(estimate[0]));
      }
    } else {
      // Reset the anomaly counter; we don't have an anomaly.
      anomalySamples = 0;
    }
}

// ----------------------------------------------------------------------
// Process an input telemetry message.
// ----------------------------------------------------------------------

void KDEAnomalyDetector ::tlmIn_handler(FwIndexType portNum,
                                        FwChanIdType id,
                                        Fw::Time& timeTag,
                                        Fw::TlmBuffer& val) {
    // If this is not a telemetry channel we care about, there is nothing to do.
    if (dimMap.count(id) == 0) {
        return;
    }

    const size_t targetDim = dimMap[id];
    F32 v;
    val.deserializeTo(v);
    tlmCache[targetDim][timeTag] = (double) v;

    // Remove any historical telemetry that is too far in the past.
    Fw::ParamValid isValid;
    U64 maxNumSeconds = this->paramGet_MAX_TRAINING_WINDOW_SECS(isValid);
    if (isValid == Fw::ParamValid::INVALID || isValid == Fw::ParamValid::UNINIT) {
        maxNumSeconds = 300;
    }

    Fw::Time lastTime = (--this->tlmCache[targetDim].end())->first;
    while (this->tlmCache[targetDim].size() > 0 &&
        Fw::Time::sub(lastTime,
                      this->tlmCache[targetDim].begin()->first).getSeconds() >
        maxNumSeconds) {
        // Remove the first element; it is too far in the past.
        this->tlmCache[targetDim].erase(this->tlmCache[targetDim].begin());
    }
}

// ----------------------------------------------------------------------
// Train the anomaly detector on historical telemetry data.
// ----------------------------------------------------------------------

void KDEAnomalyDetector ::RESET_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    // In order to do the training, we must first assemble the dataset we will
    // train on.  But, our telemetry may have come in at different times!
    // Therefore, we have to iterate over our telemetry cache to find the
    // minimum time for all telemetry dimensions, and then we have to iterate
    // through to assemble the combined telemetry at each second.

    // First, compute the maximum minimum time across all telemetry dimensions
    // (this is the beginning of the dataset), and the maximum time across all
    // telemetry dimensions (this is the end of the dataset).
    bool firstMaxMinTime = true;
    bool firstMaxTime = true;
    Fw::Time maxMinTime;
    Fw::Time maxTime;
    for (size_t d = 0; d < this->numDims; ++d) {
        if (this->tlmCache[d].size() == 0) {
            // Skip empty dimensions.
            continue;
        }

        const Fw::Time& t = this->tlmCache[d].begin()->first;
        if (firstMaxMinTime) {
            maxMinTime = t;
            firstMaxMinTime = false;
        } else if (t > maxMinTime) {
            maxMinTime = t;
        }

        const Fw::Time& t2 = (--this->tlmCache[d].end())->first;
        if (firstMaxTime) {
            maxTime = t2;
            firstMaxTime = false;
        } else if (t > maxTime) {
            maxTime = t2;
        }
    }

    // Now determine the size of our dataset.
    U32 numSeconds = Fw::Time::sub(maxTime, maxMinTime).getSeconds();
    if (numSeconds == 0) {
        // There is not enough data to train.
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    // Now construct the dataset by iterating over telemetry channels.
    arma::mat dataset(this->numDims, numSeconds, arma::fill::none);
    Fw::Time currentTime = maxMinTime;
    std::array<std::map<Fw::Time, double>::const_iterator, this->numDims>
        tlmIters;
    for (size_t d = 0; d < this->numDims; ++d) {
        tlmIters[d] = this->tlmCache[d].begin();
    }

    // We construct the telemetry value for each channel as the
    // (non-interpolated) most recent value seen at each given point in time.
    for (size_t c = 0; c < numSeconds; ++c) {
        for (size_t r = 0; r < this->numDims; ++r) {
            if (this->tlmCache[r].size() == 0) {
                // If we have no telemetry values, consider it to be zero.
                dataset(r, c) = 0.0;
                continue;
            }

            // For each telemetry dimension, make sure we are looking at the
            // most recent observation *before* `currentTime`.
            while (tlmIters[r] != this->tlmCache[r].end() &&
                   tlmIters[r]->first < currentTime) {
                ++tlmIters[r];
            }

            // After the loop, we have walked one sample past where we want to
            // be, so back up.
            if (tlmIters[r] != this->tlmCache[r].begin()) {
                --tlmIters[r];
            }

            // Note that Armadillo is column-major---so, dimensions are rows,
            // and observations are values.  See more here:
            //
            // https://www.mlpack.org/doc/user/matrices.html
            dataset(r, c) = tlmIters[r]->second;
        }

        // Increment the time we are looking for by one second.
        currentTime += 1.0f;
    }

    // Get the parameters for training.
    Fw::ParamValid isValid;
    U64 leafSize = this->paramGet_LEAF_SIZE(isValid);
    if (isValid == Fw::ParamValid::INVALID || isValid == Fw::ParamValid::UNINIT) {
        leafSize = 20;
    }

    F64 kernelBandwidth = this->paramGet_KERNEL_BW(isValid);
    if (isValid == Fw::ParamValid::INVALID || isValid == Fw::ParamValid::UNINIT) {
        kernelBandwidth = 0.5;
    }

    // Now train the model.  (This is actually really easy now that we have the
    // dataset built!  It is true when they say 90% of data science is getting
    // data in the right format, and only 10% is the actual modeling...)
    arma::wall_clock c;
    c.tic();
    this->kde.Kernel().Bandwidth(kernelBandwidth);
    this->kde.Train(std::move(dataset));
    this->anomalySamples = 0;
    const F32 treeBuildTime = (F32) c.toc();

    this->tlmWrite_TREE_BUILD_TIME(treeBuildTime);
    this->tlmWrite_MODEL_POINTS(this->kde.ReferenceTree()->Dataset().n_cols);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

}  // namespace Components
