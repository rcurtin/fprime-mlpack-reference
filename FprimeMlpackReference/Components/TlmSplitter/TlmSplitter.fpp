module FprimeMlpackReference {
    @ Telemetry splitter component
    passive component TlmSplitter {

        @ Telemetry input
        sync input port TlmRecv: Fw.Tlm

        @ Telemetry output A
        output port TlmOutA: Fw.Tlm

        @ Telemetry output B
        output port TlmOutB: Fw.Tlm

    }
}
