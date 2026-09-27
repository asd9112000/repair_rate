# GROUP-CA-LIVE core specification

Each active SA writes the four canonical slots to the single frozen 80-bit
candidate map. A matching `state_update_i` wins over the map write on that
edge, restarts at slot zero, and leaves the old physical map bits unusable
until all four current slots have been overwritten. No map clear or generation
tag is used.

The one active-SA BIST-done latch permits a done event before the four-slot
scan completes. The final slot and done event may freeze on the same edge.
After SA D freezes, the unchanged selector registers the GROUP result.

```wavejson
{signal:[{name:'state update',wave:'10......'},{name:'slot',wave:'x2345....'},{name:'BIST done',wave:'0....10..'},{name:'freeze',wave:'0....10..'},{name:'selector ready',wave:'0......10'}]}
```
