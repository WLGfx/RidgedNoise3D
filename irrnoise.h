#ifndef IRRNOISE_H
#define IRRNOISE_H

#include <QObject>
#include <QThreadPool>
#include <QRunnable>
#include <QAtomicInt>

#include "irrlicht.h"

/*
 * Multithreaded ridged noise 3d mesh generation
 *
 * params in
 *      mesh size (no. of cubes on x, y and y)
 *      seed
 *      noise settings, scale, etc
 */

struct ChunkMeshData {
    irr::scene::SMesh *mesh = nullptr;
    irr::scene::IMeshSceneNode *node = nullptr;
    int blocks;
    //uint8_t *chunks_data;
    //uint8_t *chunks_outline;
};

class IrrNoise : public QObject
{
    Q_OBJECT
public:
    explicit IrrNoise(QObject *parent = nullptr, irr::IrrlichtDevice *device = 0);
    ~IrrNoise();

    irr::IrrlichtDevice *device = 0;
    irr::scene::ISceneManager *smgr = 0;

    void update();
    void update_end();

//private:
    void init_memory();

    void set_chunk_noise();
    //void process_chunks_noise(int xside, int yside, int zside);

    void set_chunk_outline();
    //void process_chunks_outline(int xside, int yside, int zside, ChunkMeshData *data);

    void set_chunk_mesh();
    //void process_chunks_mesh(int xside, int yside, int zside, ChunkMeshData *data);

    QAtomicInt chunks_progress = 0;
    bool busy = false;

    void set_block(int x, int y, int z, uint8_t v);
    uint8_t get_block(int x, int y, int z);

    void set_outline(int x, int y, int z, uint8_t v);
    uint8_t get_outline(int x, int y, int z);

    // noise data
    float noise_scale = 0.04f;
    float noise_cutoff = 0.965f;

    float woff = 0.0f, woff_speed = 0.1f;

    // time and delta
    uint64_t get_time_millis();
    uint64_t time;
    float time_delta;

    // chunk data
    int chunk_count = 5; // number of chunks x, y, and z
    int chunk_blocks = 10; // blocks along each side of chunk
    float chunk_size = 35.0f; // wid, hgt, dep of each chunk
    int chunk_side_max; // chunk_count * chunk_blocks

    uint8_t *chunks_data = nullptr;
    uint8_t *chunks_outline = nullptr;
    //irr::scene::SMesh *chunks_blocks = nullptr;
    std::vector<ChunkMeshData> chunk_mesh;

    irr::scene::IMesh *mesh_object = nullptr; // mesh object to copy


    // Threadpool
    QThreadPool pool;
signals:

};



class ProcessChunkNoise : public QRunnable {
public:
    ProcessChunkNoise(IrrNoise *noise, int xside, int yside, int zside);
    void run();
private:
    IrrNoise *noise;
    int xside, yside, zside;
};



class ProcessChunkOutline : public QRunnable {
public:
    ProcessChunkOutline(IrrNoise *noise, int xside, int yside, int zisde,
                        ChunkMeshData *data);
    void run();
private:
    IrrNoise *noise;
    int xside, yside, zside;
    ChunkMeshData *data;
};


class ProcessChunkMesh : public QRunnable {
public:
    ProcessChunkMesh(IrrNoise *noise, int xside, int yside, int zside,
                     ChunkMeshData *data, int i);
    void run();
private:
    IrrNoise *noise;
    int xside, yside, zside;
    ChunkMeshData *data;
    int index;
};

#endif // IRRNOISE_H
