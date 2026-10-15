// One CKKS ciphertext multiplication on Microsoft SEAL: tensor, relinearise,
// rescale.
//
//   inp.a: tensor<slots x f64>   (secret)
//   inp.b: tensor<slots x f64>   (secret)
//   out.c: tensor<slots x f64>   (secret) — the product slot by slot
//
// SEAL has no automatic scaling technique: `multiply` tensors, `relinearize`
// returns the result to a pair, and `rescale_to_next` divides out the second
// scale and drops a prime. The three calls are the three stages the
// specification names, and all three are inside the measured window because
// there is nowhere else for them to go.
#include "solve.h"

#include <iostream>

void* solve_init(const fherma::Point& p, const Public& fhe) {
    // Nothing to prepare: the relinearisation key is the envelope's and is
    // made before this is called. The ring is said out loud because the
    // security level is a floor the library computes and the envelope
    // enforces: had the chain not fitted the ring at that floor, there would
    // have been no context to reach this line.
    const auto& parms = fhe.context.first_context_data()->parms();
    std::cout << "ring " << parms.poly_modulus_degree() << ", " << fhe.slots
              << " slots packed, " << p.slots << " in use, depth " << p.depth
              << ", " << p.security << "-bit security, " << p.precision
              << " bits wanted" << std::endl;
    return nullptr;
}

std::vector<Plaintext> solve_encoding(const Public& fhe,
                                      const fherma::Inputs& inp) {
    // Slot i holds element i, and the encoder pads what is left of the ring
    // with zeros. Nothing is folded into the layout, because a layout that
    // helped one multiplication along would be answering a different question
    // than the one the kernel asks.
    std::vector<Plaintext> packings(2);
    fhe.ckks->encode(inp.a.data, fhe.scale, packings[0]);
    fhe.ckks->encode(inp.b.data, fhe.scale, packings[1]);
    return packings;
}

std::vector<Ciphertext> solve_run(
    void* state,
    const Public& fhe,
    const std::vector<Ciphertext>& cts) {
    // Tensor, then relinearise: the product of two degree-1 ciphertexts is
    // degree 2, and a key switch brings it back under the key the envelope
    // holds. This is where the limbs are paid for — one key switch over as
    // many residue polynomials as the level carries.
    Ciphertext product;
    fhe.evaluator.multiply(cts[0], cts[1], product);
    fhe.evaluator.relinearize_inplace(product, fhe.relin_keys);
    // Rescale: the product holds the scale twice over. Dividing it out costs
    // a level and leaves the answer at the operands' own scale.
    fhe.evaluator.rescale_to_next_inplace(product);
    return {product};
}

fherma::Outputs solve_decoding(const fherma::Point& p,
                               const Public& fhe,
                               const std::vector<Plaintext>& pts) {
    // decode fills the ring's whole slot count; the point asked for the first
    // p.slots of them.
    std::vector<double> values;
    fhe.ckks->decode(pts.at(0), values);
    values.resize(p.slots);

    fherma::Outputs out{};
    out.c.shape = {static_cast<int64_t>(p.slots)};
    out.c.data = std::move(values);
    return out;
}

void solve_free(void* state) {}
