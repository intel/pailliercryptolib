// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <benchmark/benchmark.h>

#include <vector>

#include "ipcl/ipcl.hpp"

#define ADD_SAMPLE_KEY_LENGTH_ARGS Args({1024})->Args({2048})
#define ADD_SAMPLE_VECTOR_SIZE_ARGS \
  Args({16})                        \
      ->Args({64})                  \
      ->Args({128})                 \
      ->Args({256})                 \
      ->Args({512})                 \
      ->Args({1024})                \
      ->Args({2048})

constexpr bool Enable_DJN = true;

// P_BN comes from ISO_IEC_18033_6_Compliance
const BigNumber P_BN =
    "0xff03b1a74827c746db83d2eaff00067622f545b62584321256e62b01509f10962f9c5c"
    "8fd0b7f5184a9ce8e81f439df47dda14563dd55a221799d2aa57ed2713271678a5a0b8b4"
    "0a84ad13d5b6e6599e6467c670109cf1f45ccfed8f75ea3b814548ab294626fe4d14ff76"
    "4dd8b091f11a0943a2dd2b983b0df02f4c4d00b413";

// Q_BN comes from ISO_IEC_18033_6_Compliance
const BigNumber Q_BN =
    "0xdacaabc1dc57faa9fd6a4274c4d588765a1d3311c22e57d8101431b07eb3ddcb05d77d"
    "9a742ac2322fe6a063bd1e05acb13b0fe91c70115c2b1eee1155e072527011a5f849de70"
    "72a1ce8e6b71db525fbcda7a89aaed46d27aca5eaeaf35a26270a4a833c5cda681ffd49b"
    "aa0f610bad100cdf47cc86e5034e2a0b2179e04ec7";

// R_BN comes from ISO_IEC_18033_6_Compliance
const BigNumber R_BN =
    "0x57fb19590c31dc7c034b2a889cf4037ce3db799909c1eb0adb6199d8e96791daca9018"
    "891f34309daff32dced4af7d793d16734d055e28023acab7295956bfbfdf62bf0ccb2ed3"
    "1d5d176ca8b404e93007565fb6b72c33a512b4dc4f719231d62e27e34c3733929af32247"
    "f88c20d1ee77096cc80d3d642464054c815b35878ba812349c8bdc3c6b645daf1a0de609"
    "65f44dcf705681032480f1eeba82243196b96903becdc0df0801d4120cbd6db1c4b2841a"
    "27991c44a43750c24ed0825718ad14cfb9c6b40b78ff3d25f71741f2def1c9d420d4b0fa"
    "1e0a02e7851b5ec6a81133a368b80d1500b0f28fc653d2e6ff4366236dbf80ae3b4beae3"
    "5e04579f2c";

const BigNumber HS_BN =
    "0x7788f6e8f57d3488cf9e0c7f4c19521de9aa172bf35924c7827a1189d6c688ac078f77"
    "7efcfc230e34f1fa5ae8d9d2ed5b062257618e0a0a485b0084b3fd39080031ea739bb48c"
    "dcce4ad41704ed930d40f53a1cc5d7f70bcb379f17a912b0ad14fabe8fc10213dcd1eabd"
    "9175ee9bf66c31e9af9703c9d92fa5c8d36279459631ba7e9d4571a10960f8e8d031b267"
    "22f6ae6f618895b9ce4fce926c8f54169168f6bb3e033861e08c2eca2161198481bc7c52"
    "3a38310be22f4dd7d028dc6b774e5cb8e6f33b24168697743b7deff411510e27694bf2e8"
    "0258b325fd97370f5110f54d8d7580b45ae3db26da4e3b0409f0cfbc56d9d9856b66d8bf"
    "46e727dc3148f70362d05faea743621e3841c94c78d53ee7e7fdef61022dd56922368991"
    "f843ca0aebf8436e5ec7e737c7ce72ac58f138bb11a3035fe96cc5a7b1aa9d565cb8a317"
    "f42564482dd3c842c5ee9fb523c165a8507ecee1ac4f185bdbcb7a51095c4c46bfe15aec"
    "3dfd77e1fd2b0003596df83bbb0d5521f16e2301ec2d4aafe25e4479ee965d8bb30a689a"
    "6f38ba710222fff7cf359d0f317b8e268f40f576c04262a595cdfc9a07b72978b9564ace"
    "699208291da7024e86b6eeb1458658852f10794c677b53db8577af272233722ad4579d7a"
    "074e57217e1c57d11862f74486c7f2987e4d09cd6fb2923569b577de50e89e6965a27e18"
    "7a8a341a7282b385ef";

static void BM_KeyGen(benchmark::State& state) {
  int64_t n_length = state.range(0);
  for (auto _ : state) {
    ipcl::keyPair key = ipcl::generateKeypair(n_length, Enable_DJN);
  }
}
BENCHMARK(BM_KeyGen)->Unit(benchmark::kMicrosecond)->ADD_SAMPLE_KEY_LENGTH_ARGS;

static void BM_Encrypt(benchmark::State& state) {
  size_t dsize = state.range(0);

  BigNumber n = P_BN * Q_BN;
  int n_length = n.BitSize();
  ipcl::PublicKey* pub_key = new ipcl::PublicKey(n, n_length, Enable_DJN);
  ipcl::PrivateKey* priv_key = new ipcl::PrivateKey(pub_key, P_BN, Q_BN);

  std::vector<BigNumber> r_bn_v(dsize, R_BN);
  pub_key->setRandom(r_bn_v);
  pub_key->setHS(HS_BN);

  std::vector<BigNumber> exp_bn_v(dsize);
  for (size_t i = 0; i < dsize; i++)
    exp_bn_v[i] = P_BN - BigNumber((unsigned int)(i * 1024));

  ipcl::PlainText pt(exp_bn_v);

  ipcl::CipherText ct;
  for (auto _ : state) ct = pub_key->encrypt(pt);

  delete pub_key;
  delete priv_key;
}
BENCHMARK(BM_Encrypt)
    ->Unit(benchmark::kMicrosecond)
    ->ADD_SAMPLE_VECTOR_SIZE_ARGS;

static void BM_Decrypt(benchmark::State& state) {
  size_t dsize = state.range(0);

  BigNumber n = P_BN * Q_BN;
  int n_length = n.BitSize();
  ipcl::PublicKey* pub_key = new ipcl::PublicKey(n, n_length, Enable_DJN);
  ipcl::PrivateKey* priv_key = new ipcl::PrivateKey(pub_key, P_BN, Q_BN);

  std::vector<BigNumber> r_bn_v(dsize, R_BN);
  pub_key->setRandom(r_bn_v);
  pub_key->setHS(HS_BN);

  std::vector<BigNumber> exp_bn_v(dsize);
  for (size_t i = 0; i < dsize; i++)
    exp_bn_v[i] = P_BN - BigNumber((unsigned int)(i * 1024));

  ipcl::PlainText pt(exp_bn_v), dt;
  ipcl::CipherText ct = pub_key->encrypt(pt);
  for (auto _ : state) dt = priv_key->decrypt(ct);

  delete pub_key;
  delete priv_key;
}

BENCHMARK(BM_Decrypt)
    ->Unit(benchmark::kMicrosecond)
    ->ADD_SAMPLE_VECTOR_SIZE_ARGS;
