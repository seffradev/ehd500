# TODO
- [ ] Threat assessment and risk analysis
  - [ ] Add web application and authentication if relevant
- [ ] Compare PC/SC library performance
  - [ ] Patch a library if necessary
- [ ] Flash SimTracer
  - [ ] Explore if ATR and card hot-swapping works as expected

# Meetings
- Weekly reoccurring meetings with Hanna, Fridays on odd weeks.
- Weekly reoccurring meetings with Leissner, Fridays on even weeks.

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
  - TARA (either appendix or subchapter in report) (deep-dive into cuber security aspects?)
    - Methodology (qualitative?)
  - PC/SC analysis (either appendix or subchapter in report)
    - Methodology (quantitative)
  - OpenTelemetry
    - Methodology
      - Literature study of specification and documentation
