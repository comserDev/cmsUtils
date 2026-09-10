# DJB2 hash

cmsUtils V2 exposes a small, general-purpose DJB2 utility at
`<cms/util/hash/djb2.h>`. It is a byte-exact, non-cryptographic hash with a
fixed 32-bit wraparound contract:

```text
hash = 5381
hash = hash * 33 + byte
```

The public API includes the `cms::util::hash::Djb2` state object and the
`cms::util::hash::djb2()` one-shot overloads for `ByteView` and `StringView`.
`StringView` bytes are converted to unsigned bytes; no case normalization is
performed by the hash itself. Embedded NUL and other binary byte values are
valid input.

The styled ANSI formatter keeps ASCII uppercase conversion as its presentation
policy before feeding tag bytes to `Djb2`, preserving its existing V1 color
mapping.

DJB2 is not a cryptographic hash and must not be used for authentication,
integrity protection, persistent globally unique identifiers, or collision
resistance. Use SHA-256, HMAC, or CRC-32 according to the protocol contract.

See the [V2 API reference](API_REFERENCE.md) for the public declarations.
