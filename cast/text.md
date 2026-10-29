## diagnostics

```
@brief scout message texts, emitted verbatim when a run fails or completes.
```

+---------------------+---------------------------------+-----------------------------------------------------------------------+
| name                | value                           | description                                                           |
+=====================+=================================+=======================================================================+
| failNotFound        | `not found`                     | Referenced table, column, symbol, or file does not exist.             |
| failToolchain       | `toolchain command failed`      | A toolchain row's process could not start or exited nonzero.          |
| failToolchainColumn | `toolchain column not declared` | A ## toolchain table's header row declares no command or flag column. |
| failSampleRange     | `sample value out of range`     | A ## sample table's intervalMs or stackDepth is outside its bounds.   |
| failOutputWrite     | `cannot write output`           | An output file could not be written.                                  |
| failCommand         | `command failed`                | The run command exited nonzero.                                       |
| failFlagUnknown     | `unknown flag`                  | An argument after the manifest that is not --help or --version.       |
| done                | `Fine.`                         | Every step of the run succeeded.                                      |
+---------------------+---------------------------------+-----------------------------------------------------------------------+
