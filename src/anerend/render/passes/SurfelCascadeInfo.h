#pragma once

namespace render {
namespace surfel {

//static int surfPixSize[5] = { 8, 16, 32, 64, 128 };
static int surfPixSize[5] = { 8, 32, 64, 128, 128 };
static int numRaysPerSurfel[5] = { 16, 64, 256, 1024, 4096 };
static int sqrtNumRaysPerSurfel[5] = { 8, 8, 16, 32, 64 };

}
}