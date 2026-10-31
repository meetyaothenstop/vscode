# Independent40V cap review

Reviewer review_forward_ovp40 found no Critical, Important or Minor issue.
Confirmed mode1-only cached min(target*1.5,40) before publication; reverse,
Boost and bench scope preserved; exact ADC edge and cap-to-low/reverse tests;
identities fit21 columns. Independently reran main backend, phase, menu and UI
executables and inspected the fresh47-group log plus both clean ARM logs.
Inspected actual mode1-only clamp selection in candidate.disasm.

Documentation is updated to state the software trip-setting cap and to mark
the prior54V setting as historical. Root verified actual HEX/CCM/no-probes/DMA,
AXF export, banner/parameter identity and recorded new fingerprints.

Declined to judge: board timing, hardware OCP, electrical overshoot and physical
peak limitation. No such board claims are made. Software cap does not resolve
the low-target overvoltage root cause or guarantee peak voltage<=40V.
