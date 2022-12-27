#include "irrnoise.h"

#include "Simplex.h"

#include <QDebug>

#include <chrono>

using namespace irr;

IrrNoise::IrrNoise(QObject *parent, irr::IrrlichtDevice *device)
    : QObject{parent}, device(device)
{
    if (!device) return; // needs a device
    smgr = device->getSceneManager();

    init_memory();

    time = get_time_millis();
    time_delta = 0;

    set_chunk_noise(); // starts the mesh countdown
    set_chunk_outline();
    set_chunk_mesh();

    pool.setMaxThreadCount(8);
}

IrrNoise::~IrrNoise() {
    if (chunks_data) free(chunks_data);
    if (chunks_outline) free(chunks_outline);
    //if (chunks_blocks) free(chunks_blocks);
    for (size_t c = 0; c < chunk_mesh.size(); c++) {
        if (chunk_mesh[c].mesh) chunk_mesh[c].mesh->drop();
        //chunk_mesh[c].node->drop();
    }
    if (mesh_object) mesh_object->drop();
}

/* If the mesh is ready to update then clear out the old scene data
 * and add the new meshes to the scene.
 */
void IrrNoise::update() {
    if (busy) return;
    if (chunks_progress) return;

    chunks_progress = 1;
    busy = true;

    //qDebug() << "MESH UPDATE";

    for (u32 i = 0; i < chunk_mesh.size(); i++) {
        // TODO check if this is safe
        if (!chunk_mesh[i].node) {
            chunk_mesh[i].node = smgr->addMeshSceneNode(chunk_mesh[i].mesh);
        }
    }
}

void IrrNoise::update_end() {
    if (busy == false) return;

    /*for (u32 i = 0; i < chunk_mesh.size(); i++) {
        chunk_mesh[i].mesh->drop();
        chunk_mesh[i].mesh = nullptr;
    }*/

    uint64_t time_new = get_time_millis();
    time_delta = float(time_new - time) / 1000.0f;
    time = time_new;

    woff += woff_speed * time_delta;
    //qDebug() << "Delta" << time_delta;

    set_chunk_noise(); // starts the mesh countdown
    set_chunk_outline();
    set_chunk_mesh();

    busy = false;
}

void IrrNoise::init_memory() {
    if (chunks_data) {
        free(chunks_data);
        free(chunks_outline);
        for (size_t c = 0; c < chunk_mesh.size(); c++) {
            //chunk_mesh[c].node->drop();
            chunk_mesh[c].mesh->drop();
        }
        mesh_object->drop();
        mesh_object = nullptr;
    }

    chunk_side_max = chunk_blocks * chunk_count;
    int chunks_size = chunk_side_max * chunk_side_max * chunk_side_max;
    int chunks_total = chunk_count * chunk_count * chunk_count;

    chunks_data = (uint8_t *)malloc(chunks_size);
    chunks_outline = (uint8_t *)malloc(chunks_size);

    chunk_mesh.resize(chunks_total);

    if (!mesh_object) {
        mesh_object = smgr->getMesh("../cube.obj");
    }
}

uint64_t IrrNoise::get_time_millis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>
            (std::chrono::system_clock::now().time_since_epoch()).count();
}

void IrrNoise::set_chunk_noise() {
    // stop main UI from using the chunk mesh data
    // counter gets reduced in the mesh generation
    chunks_progress = chunk_count * chunk_count * chunk_count;

    for (int zside = 0; zside < chunk_count; zside++) {
        for (int yside = 0; yside < chunk_count; yside++) {
            for (int xside = 0; xside < chunk_count; xside++) {
                // Add to threadpool
                auto fun = new ProcessChunkNoise(this, xside, yside, zside);
                pool.start(fun);
            }
        }
    }
}

/*void IrrNoise::process_chunks_noise(int xside, int yside, int zside) {
    for (int z = zside * chunk_blocks; z < (zside + 1) * chunk_blocks; z++) {
        for (int y = yside * chunk_blocks; y < (yside + 1) * chunk_blocks; y++) {
            for (int x = xside * chunk_blocks; x < (xside + 1) * chunk_blocks; x++) {
                float xf = noise_scale * x;
                float yf = noise_scale * y;
                float zf = noise_scale * z;
                float val = Simplex::ridgedNoise(glm::vec4(xf, yf, zf, woff));
                uint8_t block_val = val > noise_cutoff ? 1 : 0;
                set_block(x, y, z, block_val);
            }
        }
    }
}*/

void IrrNoise::set_chunk_outline() {
    int chunks_index = 0;
    for (int zside = 0; zside < chunk_count; zside++) {
        for (int yside = 0; yside < chunk_count; yside++) {
            for (int xside = 0; xside < chunk_count; xside++) {
                auto fun = new ProcessChunkOutline(this, xside, yside, zside,
                                                   &chunk_mesh[chunks_index]);
                pool.start(fun);
                chunks_index++;
            }
        }
    }
}

/*void IrrNoise::process_chunks_outline(int xside, int yside, int zside, ChunkMeshData *data) {
    int blocks = 0; // count of used blocks
    for (int z = yside * chunk_blocks; z < (zside + 1) * chunk_blocks; z++) {
        for (int y = yside * chunk_blocks; y < (yside + 1) * chunk_blocks; y++) {
            for (int x = xside * chunk_blocks; x < (xside + 1) * chunk_blocks; x++) {
                if (get_block(x, y, z) && (
                        !get_block(x - 1, y, z) ||
                        !get_block(x + 1, y, z) ||
                        !get_block(x, y - 1, z) ||
                        !get_block(x, y + 1, z) ||
                        !get_block(x, y, z - 1) ||
                        !get_block(x, y, z + 1))
                    ) {
                    set_outline(x, y, z, 1);
                    blocks++;
                } else {
                    set_outline(x, y, z, 0);
                }
            }
        }
    }
    data->blocks = blocks;
    qDebug() << "Blocks:" << blocks;
}*/

void IrrNoise::set_chunk_mesh() {
    int chunks_index = 0;
    for (int zside = 0; zside < chunk_count; zside++) {
        for (int yside = 0; yside < chunk_count; yside++) {
            for (int xside = 0; xside < chunk_count; xside++) {
                auto fun = new ProcessChunkMesh(this, xside, yside, zside,
                                                &chunk_mesh[chunks_index], chunks_index);
                pool.start(fun);
                chunks_index++;
            }
        }
    }
}

/*void IrrNoise::process_chunks_mesh(int xside, int yside, int zside, ChunkMeshData *data) {
    u32 ch_buff_pos = 0; // position in SMesh data

    scene::SMeshBuffer *ch_buff = nullptr; // current buffer to add blocks to
    u32 ch_buff_vpos = 0; // pos in current buffer to add block at
    u32 ch_buff_ipos = 0;

    scene::IMeshBuffer *mo_buff = mesh_object->getMeshBuffer(0);
    u32 mo_vert_count = mo_buff->getVertexCount();
    u32 mo_ind_count = mo_buff->getIndexCount();
    video::S3DVertex *mo_verts = (video::S3DVertex*)mo_buff->getVertices();
    u16 *mo_inds = mo_buff->getIndices();

    u32 blocks_max = (65535 - mo_vert_count) / mo_vert_count; // max blocks per mesh buffer
    u32 blocks_count = data->blocks; // total blocks to add
    u32 blocks = 0; // current blocks to add to mesh buffer

    float block_scale = chunk_size / chunk_blocks;
    float middle = chunk_size * chunk_count / 2.0f;

    for (int z = yside * chunk_blocks; z < (zside + 1) * chunk_blocks; z++) {
        for (int y = yside * chunk_blocks; y < (yside + 1) * chunk_blocks; y++) {
            for (int x = xside * chunk_blocks; x < (xside + 1) * chunk_blocks; x++) {
                if (!get_outline(x, y, z)) continue; // skip empty blocks

                if (!ch_buff && blocks_count) { // get new buffer if blocks still to add
                    if (ch_buff_pos >= data->mesh->getMeshBufferCount()) {
                        ch_buff = new scene::SMeshBuffer();
                        data->mesh->addMeshBuffer(ch_buff);
                        ch_buff->drop();
                    } else {
                        ch_buff = (scene::SMeshBuffer*)data->mesh->getMeshBuffer(ch_buff_pos);
                    }

                    // get max blocks for single mesh (max verts is 65535)
                    blocks = blocks_count > blocks_max ? blocks_max : blocks_count;
                    blocks_count -= blocks;

                    // set the size of the vertex and index buffers
                    ch_buff->Vertices.set_used(blocks * mo_vert_count);
                    ch_buff->Indices.set_used(blocks * mo_ind_count);

                    qDebug() << "New MeshBuffer, blocks:" << blocks;
                }

                core::vector3df pos(x * block_scale - middle,
                                    y * block_scale - middle,
                                    z * block_scale - middle);

                for (u32 v = 0; v < mo_vert_count; v++) {
                    u32 i = ch_buff_vpos + v;
                    ch_buff->Vertices[i] = mo_verts[v];
                    ch_buff->Vertices[i].Pos *= block_scale;
                    ch_buff->Vertices[i].Pos += pos;
                }

                for (u32 i = 0; i < mo_ind_count; i++) {
                    ch_buff->Indices[i + ch_buff_ipos] = mo_inds[i] + ch_buff_vpos;
                }

                ch_buff_vpos += mo_vert_count;
                ch_buff_ipos += mo_ind_count;

                blocks--;

                if (!blocks) { // finished with mesh buffer
                    ch_buff->setHardwareMappingHint(irr::scene::EHM_DYNAMIC, irr::scene::EBT_VERTEX_AND_INDEX);
                    ch_buff->recalculateBoundingBox();
                    ch_buff = nullptr;

                    ch_buff_vpos = 0;
                    ch_buff_ipos = 0;
                    ch_buff_pos++;
                }
            }
        }
    }
    // TODO clean up unused mesh buffers
    if (ch_buff_pos > data->mesh->getMeshBufferCount()) {
        for (u32 i = ch_buff_pos; i < data->mesh->getMeshBufferCount(); i++) {
            data->mesh->getMeshBuffer(i)->drop();
        }
        qDebug() << "MeshBuffer count now:" << ch_buff_pos << "Old:" << data->mesh->getMeshBufferCount();
        data->mesh->MeshBuffers.set_used(ch_buff_pos); // reduce size
    } else {
        qDebug() << "MeshBuffer count:" << ch_buff_pos;
    }

    data->mesh->recalculateBoundingBox();
    data->mesh->setMaterialFlag(irr::video::EMF_FOG_ENABLE, true);
    data->mesh->setDirty(irr::scene::EBT_VERTEX_AND_INDEX);
    //data->node->getMesh()->setDirty(irr::scene::EBT_VERTEX_AND_INDEX);
    //data->node->setMaterialFlag(irr::video::EMF_FOG_ENABLE, true);
    //data->node->setMaterialFlag(irr::video::EMF_COLOR_MATERIAL, true);
    //data->node->setMaterialType(irr::video::EMT_SOLID);
}*/

void IrrNoise::set_block(int x, int y, int z, uint8_t v) {
    int s1 = chunk_side_max;
    chunks_data[s1 * s1 * z + s1 * y + x] = v;
}

uint8_t IrrNoise::get_block(int x, int y, int z) {
    int s1 = chunk_side_max;
    if (x < 0 || x >= s1) return 0;
    if (y < 0 || y >= s1) return 0;
    if (z < 0 || z >= s1) return 0;
    return chunks_data[s1 * s1 * z + s1 * y + x];
}

void IrrNoise::set_outline(int x, int y, int z, uint8_t v) {
    int s1 = chunk_side_max;
    chunks_outline[s1 * s1 * z + s1 * y + x] = v;
}

uint8_t IrrNoise::get_outline(int x, int y, int z) {
    int s1 = chunk_side_max;
    return chunks_outline[s1 * s1 * z + s1 * y + x];
}



ProcessChunkNoise::ProcessChunkNoise(IrrNoise *n, int x, int y, int z) {
    noise = n;
    xside = x;
    yside = y;
    zside = z;
}

void ProcessChunkNoise::run() {
    int chunk_blocks = noise->chunk_blocks;
    float noise_scale = noise->noise_scale;
    float noise_cutoff = noise->noise_cutoff;
    float woff = noise->woff;

    for (int z = zside * chunk_blocks; z < (zside + 1) * chunk_blocks; z++) {
        for (int y = yside * chunk_blocks; y < (yside + 1) * chunk_blocks; y++) {
            for (int x = xside * chunk_blocks; x < (xside + 1) * chunk_blocks; x++) {
                float xf = noise_scale * x;
                float yf = noise_scale * y;
                float zf = noise_scale * z;
                float val = Simplex::ridgedNoise(glm::vec4(xf, yf, zf, woff));
                //float val = Simplex::fBm(glm::vec4(xf, yf, zf, woff));
                uint8_t block_val = val > noise_cutoff ? 1 : 0;
                noise->set_block(x, y, z, block_val);
            }
        }
    }
}


ProcessChunkOutline::ProcessChunkOutline(IrrNoise *n, int x, int y, int z,
                                         ChunkMeshData *d) {
    noise = n;
    xside = x;
    yside = y;
    zside = z;
    data = d;
}

void ProcessChunkOutline::run() {
    int chunk_blocks = noise->chunk_blocks;

    int blocks = 0; // count of used blocks
    for (int z = zside * chunk_blocks; z < (zside + 1) * chunk_blocks; z++) {
        for (int y = yside * chunk_blocks; y < (yside + 1) * chunk_blocks; y++) {
            for (int x = xside * chunk_blocks; x < (xside + 1) * chunk_blocks; x++) {
                if (noise->get_block(x, y, z) && (
                        !noise->get_block(x - 1, y, z) ||
                        !noise->get_block(x + 1, y, z) ||
                        !noise->get_block(x, y - 1, z) ||
                        !noise->get_block(x, y + 1, z) ||
                        !noise->get_block(x, y, z - 1) ||
                        !noise->get_block(x, y, z + 1))
                    ) {
                    noise->set_outline(x, y, z, 1);
                    blocks++;
                } else {
                    noise->set_outline(x, y, z, 0);
                }
            }
        }
    }
    data->blocks = blocks;
    //qDebug() << "Blocks:" << blocks;
}


ProcessChunkMesh::ProcessChunkMesh(IrrNoise *n, int x, int y, int z,
                                   ChunkMeshData *d, int i) {
    noise = n;
    xside = x;
    yside = y;
    zside = z;
    data = d;
    index = i;
}

void ProcessChunkMesh::run() {
    if (!data->mesh) data->mesh = new scene::SMesh();

    int chunk_size = noise->chunk_size;
    int chunk_blocks = noise->chunk_blocks;
    int chunk_count = noise->chunk_count;

    u32 ch_buff_pos = 0; // position in SMesh buffer data
    u32 ch_buff_corig = data->mesh->getMeshBufferCount();

    scene::SMeshBuffer *ch_buff = nullptr; // current buffer ptr to add blocks to
    u32 ch_buff_vpos = 0; // pos in current buffer to add block at
    u32 ch_buff_ipos = 0;

    scene::IMeshBuffer *mo_buff = noise->mesh_object->getMeshBuffer(0);
    u32 mo_vert_count = mo_buff->getVertexCount();
    u32 mo_ind_count = mo_buff->getIndexCount();

    video::S3DVertex *mo_verts = (video::S3DVertex*)mo_buff->getVertices();
    u16 *mo_inds = mo_buff->getIndices();

    u32 blocks_max = (65535 - mo_vert_count) / mo_vert_count; // max blocks per mesh buffer
    u32 blocks_count = data->blocks; // total blocks to add
    u32 blocks = 0; // current blocks to add to mesh buffer

    float block_scale = chunk_size / chunk_blocks;
    float middle = chunk_blocks * chunk_count *block_scale / 2.0f;

    for (int z = zside * chunk_blocks; z < (zside + 1) * chunk_blocks; z++) {
        for (int y = yside * chunk_blocks; y < (yside + 1) * chunk_blocks; y++) {
            for (int x = xside * chunk_blocks; x < (xside + 1) * chunk_blocks; x++) {
                if (!noise->get_outline(x, y, z)) continue; // skip empty blocks

                if (!ch_buff) { // get new buffer if blocks still to add

                    // get max blocks for single mesh (max verts is 65535)
                    blocks = blocks_count > blocks_max ? blocks_max : blocks_count;
                    blocks_count -= blocks;

                    if (!blocks) continue;

                    if (ch_buff_pos >= ch_buff_corig) {
                        ch_buff = new scene::SMeshBuffer();
                        data->mesh->addMeshBuffer(ch_buff);
                        ch_buff->drop();
                    } else {
                        ch_buff = (scene::SMeshBuffer*)data->mesh->getMeshBuffer(ch_buff_pos);
                    }

                    // set the size of the vertex and index buffers
                    ch_buff->Vertices.set_used(blocks * mo_vert_count);
                    ch_buff->Indices.set_used(blocks * mo_ind_count);

                    ch_buff_vpos = 0;
                    ch_buff_ipos = 0;
                }

                core::vector3df pos(x * block_scale - middle,
                                    y * block_scale - middle,
                                    z * block_scale - middle);

                for (u32 v = 0; v < mo_vert_count; v++) {
                    u32 i = ch_buff_vpos + v;
                    ch_buff->Vertices[i] = mo_verts[v];
                    ch_buff->Vertices[i].Pos *= block_scale;
                    ch_buff->Vertices[i].Pos += pos;
                }

                for (u32 i = 0; i < mo_ind_count; i++) {
                    ch_buff->Indices[i + ch_buff_ipos] = mo_inds[i] + ch_buff_vpos;
                }

                ch_buff_vpos += mo_vert_count;
                ch_buff_ipos += mo_ind_count;

                blocks--;

                if (!blocks) { // finished with mesh buffer
                    ch_buff->setHardwareMappingHint(irr::scene::EHM_DYNAMIC,
                                                    irr::scene::EBT_VERTEX_AND_INDEX);
                    ch_buff->recalculateBoundingBox();
                    ch_buff = nullptr;

                    ch_buff_pos++;
                }
            }
        }
    }

    // clean up unused mesh buffers
    if (ch_buff_pos < ch_buff_corig) {
        for (u32 i = ch_buff_pos; i < ch_buff_corig; i++) {
            data->mesh->getMeshBuffer(i)->drop();
        }
        //data->mesh->MeshBuffers.set_used(ch_buff_pos); // reduce size
        data->mesh->MeshBuffers.erase(ch_buff_pos, ch_buff_corig - ch_buff_pos);
    }

    data->mesh->recalculateBoundingBox();
    data->mesh->setMaterialFlag(irr::video::EMF_FOG_ENABLE, true);
    data->mesh->setDirty(irr::scene::EBT_VERTEX_AND_INDEX);
    //data->node->getMesh()->setDirty(irr::scene::EBT_VERTEX_AND_INDEX);
    //data->node->setMaterialFlag(irr::video::EMF_FOG_ENABLE, true);
    //data->node->setMaterialFlag(irr::video::EMF_COLOR_MATERIAL, true);
    //data->node->setMaterialType(irr::video::EMT_SOLID);

    noise->chunks_progress--;
}
