# EARLY-CA-LIVE top specification

The top consumes the current active-SA 272-bit analyzer projection directly
and instantiates exactly one frozen `recam_shared_config_analyzer`. The
policy top does not retain a four-SA projection bank. On a valid commit the
unchanged selected-address mux formats the same live pivot projection.

```wavejson
{signal:[{name:'live state',wave:'x3......'},{name:'active SA',wave:'2.......'},{name:'scan active',wave:'011110..'},{name:'commit',wave:'0.....10'}]}
```
