# TODO
- [ ] Threat assessment and risk analysis
  - [ ] Add web application and authentication if relevant
- [ ] Compare PC/SC library performance
  - [ ] Patch a library if necessary
- [ ] Flash SimTracer
  - [ ] Explore if ATR and card hot-swapping works as expected
- [ ] Calculate required data throughput for a set number of SIM

# Meetings
- Weekly reoccurring meetings with Hanna, Fridays on odd weeks.
- Weekly reoccurring meetings with Leissner, Fridays on even weeks.

# Random notes
Actual implementation of APDU forwarding doesn't need any
awareness of the SIM messages. Only if intended to virtualize or
mock a SIM, alternatively manipulating and rewriting messages.
However, PC/SC testing needs APDU-awareness so knowledge is
relevant there.

Some APDU awareness is useful for forwarding if a UI should
display SIM identification like IMSI and ICCID.

# Tools and technologies
- OpenTelemetry
- PC/SC
- xmake
- doxygen
- testcontainers (if feasible)
- If a linker is required (compiled language)
  - mold

# Sustainability and ethical considerations
- Effective code (few CPU cycles)
  - Sources from Green Software Foundation
- Reduced CI work and only periodical processes (remove unnecessary computations)
  - Sources from Green Software Foundation
- Open source (available and giving back to the community)

# Potential deliverables
- Consumer (IP-to-UE)
  - Hardware
  - Software
- Provider (SIM-to-IP)
  - Hardware
  - Software
- Mediator
  - Forwarding functionality
    Purely forward APDU:s over IP
  - Manipulation functionality (optional?)
    Alter forwarded APDU:s, no change on SIM
  - Reconfiguration functionality (optional?)
    Rewrite files on SIM
  - Trace functionality (optional)
    Observability into APDU:s
- Documentation
  - Threat assessment and risk analysis
  - PC/SC library performance assessment
  - Performance analysis
- OpenTelemetry (optional)
- Suggestions for strategy pivots at Leissner (optional, personal)

# Benchmark & testing of PC/SC libraries
- 3 SIM at once
- Multiple APDU/RAPDU
- Languages
  - C++
  - Rust
  - Python
  - JavaScript
- Requires reading of smart card specification

# Report disposition
- First page
- Abstract and summary
- Introduction
- Methodology
  - Literature study
    - Osmocom Remote SIM
    - SimTracer documentation
    - ISO 7816, ETSI and 3GPP specifications
  - Test- and benchmark-driven development
- Background/theory
- Results
  - Deliverables
    - Software
    - Hardware
    - Documentation
- Discussion/analysis
- Conclusion
- Appendix
  - Open source contributions
  - TARA (either appendix or subchapter in report) (deep-dive into cuber security aspects?)
    - Methodology (qualitative?)
  - PC/SC analysis (either appendix or subchapter in report)
    - Methodology (quantitative)
  - OpenTelemetry
    - Methodology
      - Literature study of specification and documentation
