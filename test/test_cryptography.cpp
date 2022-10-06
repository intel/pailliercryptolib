// Copyright (C) 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <climits>
#include <iostream>
#include <random>
#include <vector>

#include "gtest/gtest.h"
#include "ipcl/ipcl.hpp"

constexpr int SELF_DEF_NUM_VALUES = 9;

TEST(CryptoTest, CryptoTest) {
  const uint32_t num_values = SELF_DEF_NUM_VALUES;

  ipcl::keyPair key = ipcl::generateKeypair(2048, true);

  std::vector<uint32_t> exp_value(num_values);
  ipcl::PlainText pt;
  ipcl::CipherText ct;
  ipcl::PlainText dt;

  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, UINT_MAX);

  for (int i = 0; i < num_values; i++) {
    exp_value[i] = dist(rng);
  }

  pt = ipcl::PlainText(exp_value);
  ct = key.pub_key->encrypt(pt);
  dt = key.priv_key->decrypt(ct);

  for (int i = 0; i < num_values; i++) {
    std::vector<uint32_t> v = dt.getElementVec(i);
    EXPECT_EQ(v[0], exp_value[i]);
  }

  delete key.pub_key;
  delete key.priv_key;
}

TEST(CryptoTest, ISO_IEC_18033_6_ComplianceTest) {
  // Ensure that at least 2 different numbers are encrypted
  // Because ir_bn_v[1] will set to a specific value
  const uint32_t num_values = SELF_DEF_NUM_VALUES + 1;

  BigNumber p =
      "0xff03b1a74827c746db83d2eaff00067622f545b62584321256e62b01509f10962f9c5c"
      "8fd0b7f5184a9ce8e81f439df47dda14563dd55a221799d2aa57ed2713271678a5a0b8b4"
      "0a84ad13d5b6e6599e6467c670109cf1f45ccfed8f75ea3b814548ab294626fe4d14ff76"
      "4dd8b091f11a0943a2dd2b983b0df02f4c4d00b413";
  BigNumber q =
      "0xdacaabc1dc57faa9fd6a4274c4d588765a1d3311c22e57d8101431b07eb3ddcb05d77d"
      "9a742ac2322fe6a063bd1e05acb13b0fe91c70115c2b1eee1155e072527011a5f849de70"
      "72a1ce8e6b71db525fbcda7a89aaed46d27aca5eaeaf35a26270a4a833c5cda681ffd49b"
      "aa0f610bad100cdf47cc86e5034e2a0b2179e04ec7";

  BigNumber n = p * q;
  int n_length = n.BitSize();

  ipcl::PublicKey* public_key = new ipcl::PublicKey(n, n_length);
  ipcl::PrivateKey* private_key = new ipcl::PrivateKey(public_key, p, q);

  ipcl::keyPair key = {public_key, private_key};

  std::vector<BigNumber> pt_bn_v(num_values);
  std::vector<BigNumber> ir_bn_v(num_values);

  BigNumber c1 =
      "0x1fb7f08a42deb47876e4cbdc3f0b172c033563a696ad7a7c76fa5971b793fa488dcdd6"
      "bd65c7c5440d67d847cb89ccca468b2c96763fff5a5ece8330251112d65e59b7da94cfe9"
      "309f441ccc8f59c67dec75113d37b1ee929c8d4ce6b5e561a30a91104b0526de892e4eff"
      "9f4fbecba3db8ed94267be31df360feaffb1151ef5b5a8e51777f09d38072bcb1b1ad15d"
      "80d5448fd0edb41cc499f8eebae2af26569427a26d0afeaa833173d6ae4e5f84eb88c0c6"
      "8c29baecf7ec5af2c1c5577336ca9482690f1c94597654afda84c6fb74df95cdd08fa9a6"
      "6296126b4061b0530d124f3797426a08f72e90ef4994eeb348f5e92bd12d41cd3343a9e2"
      "71a2f73d2cc7ffbd65bf64fb63e759f312e615aae01ae9f4573a21f1a70f56a61cfbb94d"
      "8f96fcf06c2b3216ed9574f6888df86cd5e471b641507ac6815ca781f6d31e69d6848e54"
      "2a7c57dc21109b5574b63365a19273783fafc93639c414b9475ea5ea82e73958ff5fdba9"
      "67d52721ff71209e5a3db3c580e1bfd142ba4b8ab77eb16cb488d46a04a672662cd108b7"
      "e9c58ba13dfb850653208f81956539475ffce85e0b0da59e5bd8d90051be9b2cc99e37c0"
      "60ce09814e1524458bfb5427d7a16b672682be448fa16464fcb3e7f1dca6812a2c5a9814"
      "b98ccb676367b7b3b269c670cd0210edf70ad9cb337f766af75fe06d18b3f7f7c2eae656"
      "5ff2815c2c09b1a1f5";

  BigNumber c2 =
      "0x61803645f2798c06f2c08fc254eee612c55542051c8777d6ce69ede9c84a179afb2081"
      "167494dee727488ae5e9b56d98f4fcf132514616859fc854fbd3acf6aecd97324ac3f2af"
      "fa9f44864a9afc505754aa3b564b4617e887d6aa1f88095bccf6b47f458566f9d85e80fc"
      "d478a58d4c2e895d0ed428aa8919d8ce752472bdc704fe9f01b1f663e3a9defca4b38471"
      "34883d5433b6bebb7d5a0358bcc8e3385cdf8787a1c78165eb03fc295c2ee93809d7a7a4"
      "689e79faf173e4ca3d0a6a9175887d0c70b35c529aa02699c4d4e8c98a9f3b8f2be41f35"
      "905adebf8a6940a93875d1e24e578a93bdb7cbf66cd3cdb736466588649ac237d55121ce"
      "0c0d18bc5da660d8faf9f0849ed1775ffcc5edb6900ebfb6c1e33459d29655edf706324c"
      "f642c8f36433d6b850a43ee0e788e120737b8a2858d1b5302bad3413102fd7dccfe458b2"
      "57fdbf920fe942e23ec446b1b302d41710fe56b26e11987ac06cfa635664c7a0ec18f8c8"
      "c871919fc893a3117ff5e73d4c115e66e3bc5bd2b9127b2bb816c549245c65cf22a533a3"
      "d2b6cb7c46757d3a87173f93e8b431891697f8d60c59631734f46cf3d70d9065f0167d5a"
      "d7353c0812af024ced593273551d29c89232f2f3d548b9248291c1b8e833ed178eb2cf1a"
      "d6f1d6864f1fd3e2e3937e00d391ad330b443aec85528571740ed5538188c32caab27c7b"
      "f437df2bb97cb90e02";

  BigNumber c1c2 =
      "0x309f6e614d875e3bb0a77eedeb8895e7c6f297f161f576aef4f8b72bb5b81ef78b831a"
      "af134b09fe8697159cfd678c49920cb790e36580c5201a96848d7242fceb025808dd26b5"
      "0ff573ffca3f65e51b3b9fe85c7e44f5c8df0a9e524f64a5acc5c62cba7475978eb55e08"
      "93eff1c40547ef9db087f8a54a13bf33a4648c4719233cfb107ba469c61f1c07578d9c19"
      "fa8012b743d31fbca8eb4250ad902cf0c3d24c619fcd0874ad6a12ab8eafffabca6ed1aa"
      "a4ba0df1544c3826364ac955c5853dc0490b9992e867e2dc95ec4b8742f177b7b24f29f6"
      "8de4d552f32ca0da7d5cb2d85f020eefb8b58261c93643a4b63a9223efea803367b932b4"
      "30ae47730d9b493e4194cbc7e8aa6d8aae45aa016d7f197dab5bb9508d5af6c3f47c0ec4"
      "8ff604e53edbafa9a1bdae6add7169b83278a025f0be7980688806deaa9afaf80ca4212d"
      "53079c4841546bc1622c5bf211a9db1f8933211b6a5b5f312d6919181bf7797188645052"
      "a9fff167c7acbc43454cd3caab36a501feba27f28720f2ab23d5dea3c73d4421b059eef9"
      "f1c227a3ed59c487c9483a08e98bfd34920349fa861b41ce61a4caa8b7f0fc1fcba7dedb"
      "8f9c64ab3a42968f6c88f45541c734d7c0206968a103d02985854a5156d9edb99a332de9"
      "a6d47f9af6e68e18960fa5916cc48994334354d6303312b8e96602766bec337a8a92c596"
      "b21b6038828a6c9744";

  BigNumber m1m2 = "0x616263646566676869606a6b6c6d6e6f";

  BigNumber r0 =
      "0x57fb19590c31dc7c034b2a889cf4037ce3db799909c1eb0adb6199d8e96791daca9018"
      "891f34309daff32dced4af7d793d16734d055e28023acab7295956bfbfdf62bf0ccb2ed3"
      "1d5d176ca8b404e93007565fb6b72c33a512b4dc4f719231d62e27e34c3733929af32247"
      "f88c20d1ee77096cc80d3d642464054c815b35878ba812349c8bdc3c6b645daf1a0de609"
      "65f44dcf705681032480f1eeba82243196b96903becdc0df0801d4120cbd6db1c4b2841a"
      "27991c44a43750c24ed0825718ad14cfb9c6b40b78ff3d25f71741f2def1c9d420d4b0fa"
      "1e0a02e7851b5ec6a81133a368b80d1500b0f28fc653d2e6ff4366236dbf80ae3b4beae3"
      "5e04579f2c";
  BigNumber r1 =
      "0x6ee8ed76227672a7bcaa1e7f152c2ea39f2fa225f0713f58210c59b2270b110e38b650"
      "69aaedbeffc713c021336cc12f65227cc0357ca531c07c706e7224c2c11c3145bc0a05b1"
      "64f426ec03350820f9f416377e8720ddb577843cae929178bfe5772e2cc1e9b94e8fce81"
      "4eaf136c6ed218ca7b10ea4d5218e7ba82bd74bb9f19d3ccc7d2e140e91cfb25f76f54aa"
      "70f2ed88ef343dd5fb98617c0036b7717f7458ec847d7b52e8764a4e92c397133a95e35e"
      "9a82d5dc264ff423398cfadfbaec4727854e68f2e9e210d6a65c39b5a9b2a0ebdc538983"
      "4883680e42b5d8582344e3e07a01fbd6c46328dcfa03074d0bc02927f58466c2fa74ab60"
      "8177e3ec1b";

  for (int i = 0; i < num_values; i++) {
    ir_bn_v[i] = r0;
    pt_bn_v[i] = "0x414243444546474849404a4b4c4d4e4f";
  }
  ir_bn_v[1] = r1;
  pt_bn_v[1] = "0x20202020202020202020202020202020";

  ipcl::PlainText pt;
  ipcl::CipherText ct;

  key.pub_key->setRandom(ir_bn_v);

  pt = ipcl::PlainText(pt_bn_v);
  ct = key.pub_key->encrypt(pt);

  ipcl::PlainText dt;
  dt = key.priv_key->decrypt(ct);
  for (int i = 0; i < num_values; i++) {
    EXPECT_EQ(dt.getElement(i), pt_bn_v[i]);
  }

  std::string str1, str2;
  c1.num2hex(str1);
  EXPECT_EQ(str1, ct.getElementHex(0));
  c2.num2hex(str2);
  EXPECT_EQ(str2, ct.getElementHex(1));

  ipcl::CipherText a(key.pub_key, ct.getElement(0));
  ipcl::CipherText b(key.pub_key, ct.getElement(1));
  ipcl::CipherText sum = a + b;

  std::string str3;
  c1c2.num2hex(str3);
  EXPECT_EQ(str3, sum.getElementHex(0));

  std::string str4;
  ipcl::PlainText dt_sum;

  dt_sum = key.priv_key->decrypt(sum);
  m1m2.num2hex(str4);
  EXPECT_EQ(str4, dt_sum.getElementHex(0));

  delete key.pub_key;
  delete key.priv_key;
}
