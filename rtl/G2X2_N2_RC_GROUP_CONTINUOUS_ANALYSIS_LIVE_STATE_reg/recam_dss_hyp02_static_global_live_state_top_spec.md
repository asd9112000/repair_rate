# GROUP-CA-LIVE top specification

The top directly connects the current active-SA 272-bit projection to one
frozen shared analyzer. It reuses the frozen candidate-store, slot decode,
selector, and pivot-address reconstruction modules from the canonical GROUP
case. The fourth-slot pivot capture is suppressed on a same-edge state update
so it cannot be paired with a discarded candidate record.

```wavejson
{signal:[{name:'live state',wave:'x3.......'},{name:'scan slot',wave:'x2345....'},{name:'map write',wave:'011110...'},{name:'state update',wave:'0...10...'},{name:'ready',wave:'0......10'}]}
```
