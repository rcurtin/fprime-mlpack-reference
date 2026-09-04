module Components {
    @ mlpack-based anomaly detector using kernel density estimation
    active component KDEAnomalyDetector {

        @ Command for resetting/retraining the KDE model.
        async command RESET()

        @ Number of seconds taken to build the tree used by KDE.
        telemetry TREE_BUILD_TIME: F32
        @ Number of points the KDE model was last trained on.
        telemetry MODEL_POINTS: U64
        @ Current density reading.  -1 if no model is trained.
        telemetry CURRENT_DENSITY: F64

        @ Event indicating the model has been reset.
        event ResetEvent() severity activity high id 0 format "KDE model reset"

        @ Event issued when an anomaly is detected.
        event AnomalyDetectedEvent(density: F64) severity activity high id 1 format "Anomaly detected!  Density: {}"

        @ The rate group scheduler input used for automatic anomaly detection.
        sync input port run: Svc.Sched

        @ The input used when a telemetry event is sent from TlmSplitter.
        sync input port tlmIn: Fw.Tlm

        @ Leaf size to use when building the tree.
        param LEAF_SIZE: U64 default 20
        @ Maximum number of seconds to look back in telemetry history when building the tree.
        param MAX_TRAINING_WINDOW_SECS: U64 default 300
        @ Bandwidth of Gaussian kernel to use.
        @ Note: this is tuned specifically for this instance!  You may want a different default for your application.
        param KERNEL_BW: F32 default 25
        @ Threshold for anomaly detection.  Lower thresholds mean fewer detections.
        param ANOMALY_THRESHOLD: F64 default 1e-6
        @ Number of anomalous samples that must be seen before an anomaly is reported.
        param ANOMALY_SAMPLES_BEFORE_ALARM: U64 default 5

        ###############################################################################
        # Standard AC Ports: Required for Channels, Events, Commands, and Parameters  #
        ###############################################################################
        @ Port for requesting the current time
        time get port timeCaller

        @ Enables command handling
        import Fw.Command

        @ Enables event handling
        import Fw.Event

        @ Enables telemetry channels handling
        import Fw.Channel

        @ Port to return the value of a parameter
        param get port prmGetOut

        @Port to set the value of a parameter
        param set port prmSetOut

    }
}
