#include "noiseGeneration.h"
#include "FastNoise/FastNoise.h"
#include "FastNoise/Generators/DomainWarpSimplex.h"
#include "FastNoise/Generators/Fractal.h"
#include "FastNoise/Generators/Simplex.h"

static int caveChecks = 0;
static int caveHits = 0;
void initNoise()
{
    //simplex source
    static FastNoise::SmartNode<> terrainSource = FastNoise::New<FastNoise::Simplex>();

    //Fractal FBm for rolling hills
    auto terrainFractal = FastNoise::New<FastNoise::FractalFBm>();
    terrainFractal->SetSource(terrainSource);
    terrainFractal->SetOctaveCount(4);
    terrainFractal->SetGain(0.5f);
    terrainFractal->SetLacunarity(2.0f);

    // Domain warp to make it organic looking
    auto terrainWarp = FastNoise::New<FastNoise::DomainWarpSimplex>();
    terrainWarp->SetSource(terrainFractal);
    terrainWarp->SetWarpAmplitude(30.0f);

    NoiseGenerator::terrainNoise = terrainWarp;

    // Caves 3D
    auto caveSource = FastNoise::New<FastNoise::Simplex>();

    auto caveFractal = FastNoise::New<FastNoise::FractalFBm>();
    caveFractal->SetSource(caveSource);
    caveFractal->SetOctaveCount(4);
    caveFractal->SetGain(0.5f);
    caveFractal->SetLacunarity(2.0f);

    //winding tunels
    auto caveWarp = FastNoise::New<FastNoise::DomainWarpSimplex>();
    caveWarp->SetSource(caveFractal);
    caveWarp->SetWarpAmplitude(20.f);

    NoiseGenerator::caveNoise = caveWarp;
}

bool isCave(int worldX, int worldY, int worldZ)
{
    float scale = 0.08f;

    float nx = worldX * scale;
    float ny = worldY * scale;
    float nz = worldZ * scale;

    float value = NoiseGenerator::caveNoise->GenSingle3D(nx, ny, nz, 4242);

    return value > 0.01f;
}
