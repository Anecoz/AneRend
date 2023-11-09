//const uint NUM_RAYS_PER_SURFEL = 16;

struct SurfelSHParams
{
  float L_lm_r[9];
  float L_lm_g[9];
  float L_lm_b[9];
};

uint SURFEL_DIR_IRR_PIX_SIZE[5] = uint[](8, 8, 16, 32, 64);
float SURFEL_T_MAX[5] = float[](30.0, 1.0, 2.0, 50.0, 50.0);
float SURFEL_T_MIN[5] = float[](0.1, 0.3, 0.8, 1.5, 4.0);

//const uint SURFEL_OCT_PIX_SIZE[5] = uint[](4, 8, 16, 32, 64);
//const uint SURFEL_PIXEL_SIZE[5] = uint[](8, 16, 32, 64, 128); // one surfel for each nxn pixel tile
const uint SURFEL_PIXEL_SIZE[5] = uint[](8, 32, 64, 128, 128); // one surfel for each nxn pixel tile

SurfelSHParams calcShForDir(vec3 dir, vec3 L)
{
  SurfelSHParams params;

  vec3 lm[9];
  lm[0] = 0.282095 * L;
  lm[1] = 0.488603 * L * dir.x;
  lm[2] = 0.488603 * L * dir.z;
  lm[3] = 0.488603 * L * dir.y;
  lm[4] = 1.092548 * L * dir.x * dir.z;
  lm[5] = 1.092548 * L * dir.y * dir.z;
  lm[6] = 1.092548 * L * dir.x * dir.y;
  lm[7] = 0.315392 * L * (3.0 * dir.z * dir.z - 1.0);
  lm[8] = 0.546274 * L * (dir.x * dir.x - dir.y * dir.y);

  params.L_lm_r[0] = lm[0].r;
  params.L_lm_g[0] = lm[0].g;
  params.L_lm_b[0] = lm[0].b;

  params.L_lm_r[1] = lm[1].r;
  params.L_lm_g[1] = lm[1].g;
  params.L_lm_b[1] = lm[1].b;

  params.L_lm_r[2] = lm[2].r;
  params.L_lm_g[2] = lm[2].g;
  params.L_lm_b[2] = lm[2].b;

  params.L_lm_r[3] = lm[3].r;
  params.L_lm_g[3] = lm[3].g;
  params.L_lm_b[3] = lm[3].b;

  params.L_lm_r[4] = lm[4].r;
  params.L_lm_g[4] = lm[4].g;
  params.L_lm_b[4] = lm[4].b;

  params.L_lm_r[5] = lm[5].r;
  params.L_lm_g[5] = lm[5].g;
  params.L_lm_b[5] = lm[5].b;

  params.L_lm_r[6] = lm[6].r;
  params.L_lm_g[6] = lm[6].g;
  params.L_lm_b[6] = lm[6].b;

  params.L_lm_r[7] = lm[7].r;
  params.L_lm_g[7] = lm[7].g;
  params.L_lm_b[7] = lm[7].b;

  params.L_lm_r[8] = lm[8].r;
  params.L_lm_g[8] = lm[8].g;
  params.L_lm_b[8] = lm[8].b;

  return params;
}