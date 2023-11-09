const int NUM_PROBES_PER_PLANE = 64;
const int NUM_PROBE_PLANES = 8;
const vec3 PROBE_STEP = vec3(1.0, 2.0, 1.0); // every meter in xz, every 2 meters in y
const uint NUM_RAYS_PER_PROBE = 256;

const int PROBE_IRR_DIR_PIX_SIZE = 16;
const int PROBE_CONV_PIX_SIZE = 8;