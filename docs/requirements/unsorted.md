Functional: System *should* enable **tracing** on **forwarded**
APDU:s.

**Tracing** or sniffing: logging of APDU containing source and destination,
alongside data contents. If applicable, extra information such
as type, DF and EF should be provided. OpenTelemetry?

Functional: System *should* enable **manipulation** of
**forwarded** APDU:s.

**Manipulation**: Rewriting of the data contents of an APDU. This
allows transformation of one APDU into another.

Functional: System *should* enable **reprogramming** of SIM.

**Reprogramming**:

Functional: System *should* provide a mechanism for **real-time**
switch of SIM-UE connections.
(Non-functional: constrain to HTTP?)

Functional: System *must* store ATR for each SIM to send to UE
when connected.

Functional: System *must* provide a connectivity mechanism
between SIM and UE.

Non-functional: System *should* enable SIM-UE connectivity over IP.
(Constrain to TCP? WebSockets?)
