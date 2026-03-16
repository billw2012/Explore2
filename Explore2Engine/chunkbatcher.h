#if !defined(__EXPLORE2_CHUNK_BATCHER_H__)
#define __EXPLORE2_CHUNK_BATCHER_H__

namespace explore2{;

/*
To batch chunks:
Methods of batching:

batch visible chunks into nxn grids of textures and verts.
vertex data contains encoded mask for each chunk.
encoded to 1 bit values, 32 chunks only requires one extra float
pass shader an array of float weights for each chunk, and use the mask to
choose one. 
e.g.

static const int GRID_SIZE = 4
float weights[GRID_SIZE*GRID_SIZE];

int extract_bit(float1 val, int bit)
{
	return (val >> bit) & 1;
}

void vs(float1 weightMask)
{
	float finalWeight = 0.0;
	for(int idx = 0; idx < GRID_SIZE*GRID_SIZE; idx++)
	{
		finalWeight += weights[idx] * extract_bit(weightMask, idx);
	}
}

*/


struct ChunkBatcher
{

};

}

#endif 