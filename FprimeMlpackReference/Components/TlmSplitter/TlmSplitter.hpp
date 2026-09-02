// ======================================================================
// \title  TlmSplitter.hpp
// \author Ryan Curtin (ryan@ratml.org)
// \brief  hpp file for TlmSplitter component implementation class
// ======================================================================

#ifndef FprimeMlpackReference_TlmSplitter_HPP
#define FprimeMlpackReference_TlmSplitter_HPP

#include "FprimeMlpackReference/Components/TlmSplitter/TlmSplitterComponentAc.hpp"

namespace FprimeMlpackReference {

class TlmSplitter final : public TlmSplitterComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct TlmSplitter object
    TlmSplitter(const char* const compName  //!< The component name
    );

    //! Destroy TlmSplitter object
    ~TlmSplitter();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for TlmRecv
    //!
    //! Telemetry input
    void TlmRecv_handler(FwIndexType portNum,  //!< The port number
                         FwChanIdType id,      //!< Telemetry Channel ID
                         Fw::Time& timeTag,    //!< Time Tag
                         Fw::TlmBuffer& val    //!< Buffer containing serialized telemetry value
                         ) override;
};

}  // namespace FprimeMlpackReference

#endif
