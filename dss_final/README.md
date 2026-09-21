# DSS Final Artifact Archive

`dss_final/` is a packaged-results archive, not a development RTL tree.

1. Do not edit functional RTL directly inside this directory.
2. New final cases require functional-verification pass, authoritative synthesis,
   and human approval.
3. A changed final case must be packaged from a new canonical commit and a new
   synthesis record.
4. Every final case must be independently reproducible from its source manifest
   and methodology record.

Functional RTL development remains under canonical `rtl/`.
