# Ciphertext multiplication on Microsoft SEAL

An answer to `ciphertext-multiplication/ckks@1.0.0` on FHERMA: one CKKS
multiplication of two secret vectors — tensor, relinearise, rescale — and
nothing else inside the measured call. One directory per point, because the
envelope builds the context from `config.jsonc` and never reads the point;
`solve.cpp` and every generated file beside it are byte-identical in both.

SEAL spells the three stages out, having no automatic scaling technique to
place any of them elsewhere:

```cpp
Ciphertext product;
fhe.evaluator.multiply(cts[0], cts[1], product);          // tensor
fhe.evaluator.relinearize_inplace(product, fhe.relin_keys);
fhe.evaluator.rescale_to_next_inplace(product);           // the level it costs
```

## The two points

| | `solution-s32768-d10` | `solution-s4096-d20` |
| --- | --- | --- |
| point | slots 32768, depth 10, precision 15 | slots 4096, depth 20, precision 20 |
| `poly_modulus_degree` | 65536 | 65536 |
| `slot_count` | 32768 | 32768, of which 4096 in use |
| `mult_depth` | 10 | 20 |
| `scale_mod_size` | 50 | 48 |
| `first_mod_size` / `special_mod_size` | 60 / 60 | 60 / 60 |
| primes in the chain | 12 | 22 |
| residue polynomials per ciphertext | 11 | 21 |
| log Q | 560 | 1020 |
| log Q · P | 620 | 1080 |
| key-switch digits | 11 | 21 |

Both points land on a ring of 65536, as they do for every library on this
board: the wide one because CKKS packs at most ring/2 slots, the deep one
because twenty levels at this scale is over a thousand bits of Q, which
32768 does not carry at 128-bit classic security.

## Where SEAL differs from the libraries beside it

**One special prime, and a digit per prime of Q.** SEAL's key switch is an
RNS decomposition: the relinearisation key has one component per prime of Q —
eleven at the wide point, twenty-one at the deep one — and the raise is into
a single auxiliary prime. OpenFHE and Lattigo answer the same points with a
hybrid key switch of three digits and four or six auxiliary primes. So SEAL
carries a narrower modulus for the same depth, 620 bits against 800 and 1080
against 1380, and does more work inside the key switch for it. There is no
digit count to set: the decomposition is the library's, and this is the
trade it makes.

**No sparse packing.** A SEAL ciphertext packs exactly half the ring, so the
deep point runs with 4096 of 32768 slots in use. OpenFHE's `batch_size` makes
the same point pack 4096, which is why the row's `slot_count` reads 32768
where the other libraries' reads 4096. The multiplication is over the whole
ring either way.

**The security floor, enforced twice over.** `sec_level_type::tc128` is
SEAL's own enforcement, and its copy of the HE standard's table stops at a
ring of 32768: above it `MaxBitCount` returns zero, which the context reads
as "no modulus is secure" and refuses every parameter set however wide the
ring, while the library itself works up to 131072. Since both points need
65536, the envelope carries the standard's remaining rows — 1782 bits at this
ring, the same number the Lattigo envelope holds — and checks log Q · P
against them before it generates a key. Up to 32768 the refusal is SEAL's
own; above it the refusal is the envelope's, against the same table.

**No bootstrapping.** The library has none, so the kernels on this platform
that need a refresh are not answered here.

## What is measured

`solve_run` only: the three calls above. Encoding, encryption, decryption and
decoding are timed separately and reported beside the measurement, the keys
exist only inside the envelope, and `solve_init` — not measured — has nothing
to prepare beyond printing the ring and the slot count the library agreed to.

The four functions take a `Public` rather than a context, because SEAL
spreads over `Evaluator`, `CKKSEncoder`, `Encryptor` and `Decryptor` what
another library keeps behind one handle. The envelope gathers the public half
and hands it over; the secret key and the decryptor stay inside.

## Building it outside the platform

The image the platform builds in, on the source as the platform sees it:

```sh
docker run --rm -v "$PWD/solution-s32768-d10":/solution \
  -v "$POINT":/point fherma/seal:4.7.0 \
  sh -c "cd /solution && cmake -B build -DCMAKE_BUILD_TYPE=Release \
         && cmake --build build && ./build/solution /point"
```

`$POINT` is a directory the kernel's bundle prepared: `manifest.json`,
`cases/000000/a.bin`, `cases/000000/b.bin` — little-endian doubles — and the
run writes `out/results.json` with what each case cost. `CMakeLists.txt` asks
for no version, so the same source builds in every SEAL image the platform
holds.

## Rows this leaves open

- **4.6.2 beside 4.7.0.** The October 2026 release claims improvements in
  key switching, rescaling, encryption, decryption and the NTT, and states
  that results are identical given the same randomness. Two images of one
  source answer whether the claim shows up in this kernel.
- **`x86-64-v4`.** SEAL ships no assembly, so the compiler's target is the
  whole of what a vectorised build can be — the question the OpenFHE pair
  asks, asked of a library that cannot answer it with a backend of its own.
- **Intel HEXL**, which is such a backend: AVX-512 for the NTT and modular
  arithmetic, built in when the image asks for it.
- **BGV and BFV**, which this envelope also speaks, for a kernel over
  integers rather than reals.
