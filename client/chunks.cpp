/*
 * chunks.cpp
 * Copyright (c) 2021, Kirill GPRB.
 * All Rights Reserved.
 */
#include <client/components/chunk_mesh.hpp>
#include <client/chunks.hpp>
#include <client/globals.hpp>
#include <shared/components/chunk.hpp>

void ClientChunkManager::implOnClear()
{
    const auto view = globals::registry.view<ChunkComponent>();
    for(const auto [entity, cp] : view.each())
        globals::registry.destroy(entity);
}

bool ClientChunkManager::implOnRemove(const chunkpos_t &cp, const ClientChunk &data)
{
    globals::registry.destroy(data.entity);
    return true;
}

ClientChunk ClientChunkManager::implOnCreate(const chunkpos_t &cp)
{
    if(ClientChunk *nc = find(cp + chunkpos_t(0, 0, 1)))
        globals::registry.emplace_or_replace<ChunkFlaggedForMeshingComponent>(nc->entity);
    if(ClientChunk *nc = find(cp - chunkpos_t(0, 0, 1)))
        globals::registry.emplace_or_replace<ChunkFlaggedForMeshingComponent>(nc->entity);
    if(ClientChunk *nc = find(cp + chunkpos_t(0, 1, 0)))
        globals::registry.emplace_or_replace<ChunkFlaggedForMeshingComponent>(nc->entity);
    if(ClientChunk *nc = find(cp - chunkpos_t(0, 1, 0)))
        globals::registry.emplace_or_replace<ChunkFlaggedForMeshingComponent>(nc->entity);
    if(ClientChunk *nc = find(cp + chunkpos_t(1, 0, 0)))
        globals::registry.emplace_or_replace<ChunkFlaggedForMeshingComponent>(nc->entity);
    if(ClientChunk *nc = find(cp - chunkpos_t(1, 0, 0)))
        globals::registry.emplace_or_replace<ChunkFlaggedForMeshingComponent>(nc->entity);
    ClientChunk data;
    data.entity = globals::registry.create();
    globals::registry.emplace<ChunkComponent>(data.entity, ChunkComponent(cp));
    globals::registry.emplace<ChunkFlaggedForMeshingComponent>(data.entity);
    data.data.fill(NULL_VOXEL);
    return std::move(data);
}

voxel_t ClientChunkManager::implGetVoxel(const ClientChunk &data, const localpos_t &lp) const
{
    return data.data[toVoxelIdx(lp)];
}

void ClientChunkManager::implSetVoxel(ClientChunk *data, const chunkpos_t &cp, const localpos_t &lp, voxel_t voxel, voxel_set_flags_t flags)
{ 
    if(flags & VOXEL_SET_UPDATE_NEIGHBOURS) {
        if(ClientChunk *nc = find(cp + chunkpos_t(0, 0, 1)))
            globals::registry.emplace_or_replace<ChunkFlaggedForMeshingComponent>(nc->entity);
        if(ClientChunk *nc = find(cp - chunkpos_t(0, 0, 1)))
            globals::registry.emplace_or_replace<ChunkFlaggedForMeshingComponent>(nc->entity);
        if(ClientChunk *nc = find(cp + chunkpos_t(0, 1, 0)))
            globals::registry.emplace_or_replace<ChunkFlaggedForMeshingComponent>(nc->entity);
        if(ClientChunk *nc = find(cp - chunkpos_t(0, 1, 0)))
            globals::registry.emplace_or_replace<ChunkFlaggedForMeshingComponent>(nc->entity);
        if(ClientChunk *nc = find(cp + chunkpos_t(1, 0, 0)))
            globals::registry.emplace_or_replace<ChunkFlaggedForMeshingComponent>(nc->entity);
        if(ClientChunk *nc = find(cp - chunkpos_t(1, 0, 0)))
            globals::registry.emplace_or_replace<ChunkFlaggedForMeshingComponent>(nc->entity);
    }

    data->data[toVoxelIdx(lp)] = voxel;
    globals::registry.emplace_or_replace<ChunkFlaggedForMeshingComponent>(data->entity);
}
