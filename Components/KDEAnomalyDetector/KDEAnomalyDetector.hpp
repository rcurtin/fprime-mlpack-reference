// ======================================================================
// \title  KDEAnomalyDetector.hpp
// \author Ryan Curtin (ryan@ratml.org)
// \brief  hpp file for KDEAnomalyDetector component implementation class
// ======================================================================

#ifndef Components_KDEAnomalyDetector_HPP
#define Components_KDEAnomalyDetector_HPP

#include "Components/KDEAnomalyDetector/KDEAnomalyDetectorComponentAc.hpp"
#include <mlpack.hpp>

namespace Components {

class KDEAnomalyDetector final : public KDEAnomalyDetectorComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct KDEAnomalyDetector object
    KDEAnomalyDetector(const char* const compName  //!< The component name
    );

    //! Destroy KDEAnomalyDetector object
    ~KDEAnomalyDetector();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for run
    //!
    //! The rate group scheduler input used for automatic anomaly detection.
    void run_handler(FwIndexType portNum,  //!< The port number
                     U32 context           //!< The call order
                     ) override;

    //! Handler implementation for tlmIn
    //!
    //! The input used when a telemetry event is sent from TlmSplitter.
    void tlmIn_handler(FwIndexType portNum,  //!< The port number
                       FwChanIdType id,      //!< Telemetry Channel ID
                       Fw::Time& timeTag,    //!< Time Tag
                       Fw::TlmBuffer& val    //!< Buffer containing serialized telemetry value
                       ) override;

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler implementation for command RESET
    //!
    //! Command for resetting/retraining the KDE model.
    void RESET_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                          U32 cmdSeq            //!< The command sequence number
                          ) override;

    // Internally-held parameters for the anomaly detector.

    //! The number of dimensions of telemetry we are using.
    constexpr static const size_t numDims = 17;
    //! The current version of the tree.
    mlpack::KDE<> kde;

    //! Mappings from telemetry channels to dimensions used by the KDE model.
    std::map<FwChanIdType, size_t> dimMap;
    //! Cache for telemetry (for training).  Telemetry may come in at different
    //! times, so, we need to store all the telemetry we get, which we will
    //! rebuild into a dataset with fixed time points when we train the model.
    std::vector<std::map<Fw::Time, double>> tlmCache;

    //! Number of consecutive anomalies we have seen.
    U64 anomalySamples;
};

}  // namespace Components

#endif
