//const uint NUM_RAYS_PER_SURFEL = 16;

uint SURFEL_DIR_IRR_PIX_SIZE[5] = uint[](4, 8, 16, 32, 64);
float SURFEL_T_MAX[5] = float[](0.5, 1.0, 2.0, 50.0, 50.0);
float SURFEL_T_MIN[5] = float[](0.1, 0.5, 1.0, 2.0, 4.0);

const uint SURFEL_OCT_PIX_SIZE[5] = uint[](4, 8, 16, 32, 64);
const uint SURFEL_PIXEL_SIZE[5] = uint[](8, 16, 32, 64, 128); // one surfel for each nxn pixel tile