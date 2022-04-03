/*
 * Copyright (c) 2022 Kirill GPRB
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include <client/chunks.hpp>
#include <client/comp/static_chunk_mesh_needs_update_component.hpp>
#include <client/globals.hpp>
#include <common/comp/chunk_component.hpp>

static void forceFullStaticMeshUpdate(entt::entity entity)
{
    globals::registry.get_or_emplace<StaticChunkMeshNeedsUpdateComponent<VoxelType::STATIC_CUBE>>(entity);
    globals::registry.get_or_emplace<StaticChunkMeshNeedsUpdateComponent<VoxelType::STATIC_FLORA>>(entity);
    globals::registry.get_or_emplace<StaticChunkMeshNeedsUpdateComponent<VoxelType::STATIC_LIQUID>>(entity);
    globals::registry.get_or_emplace<StaticChunkMeshNeedsUpdateComponent<VoxelType::STATIC_GAS>>(entity);
}

void ClientChunkManager::impl_onClear()
{
    const auto view = globals::registry.view<ChunkComponent>();
    for(const auto [entity, chunk] : view.each())
        globals::registry.destroy(entity);
}

bool ClientChunkManager::impl_onRemove(const chunk_pos_t &cpos, const ClientChunk &chunk)
{
    globals::registry.destroy(chunk.entity);
    return true;
}

ClientChunk ClientChunkManager::impl_onCreate(const chunk_pos_t &cpos)
{
    if(const ClientChunk *neighbour = find(cpos + chunk_pos_t(0, 0, 1)))
        forceFullStaticMeshUpdate(neighbour->entity);
    if(const ClientChunk *neighbour = find(cpos - chunk_pos_t(0, 0, 1)))
        forceFullStaticMeshUpdate(neighbour->entity);
    if(const ClientChunk *neighbour = find(cpos + chunk_pos_t(0, 1, 0)))
        forceFullStaticMeshUpdate(neighbour->entity);
    if(const ClientChunk *neighbour = find(cpos - chunk_pos_t(0, 1, 0)))
        forceFullStaticMeshUpdate(neighbour->entity);
    if(const ClientChunk *neighbour = find(cpos + chunk_pos_t(1, 0, 0)))
        forceFullStaticMeshUpdate(neighbour->entity);
    if(const ClientChunk *neighbour = find(cpos - chunk_pos_t(1, 0, 0)))
        forceFullStaticMeshUpdate(neighbour->entity);
    ClientChunk chunk = {};
    chunk.entity = globals::registry.create();
    globals::registry.emplace<ChunkComponent>(chunk.entity, ChunkComponent(cpos));
    forceFullStaticMeshUpdate(chunk.entity);
    chunk.data.fill(NULL_VOXEL);
    return std::move(chunk);
}

voxel_t ClientChunkManager::impl_onGetVoxel(const ClientChunk &chunk, const local_pos_t &lpos) const
{
    return chunk.data.at(world::getVoxelIndex(lpos));
}

void ClientChunkManager::impl_onSetVoxel(ClientChunk *chunk, const chunk_pos_t &cpos, const local_pos_t &lpos, voxel_t voxel, voxel_set_flags_t flags)
{
    if(flags & VOXEL_SET_UPDATE_NEIGHBOURS) {
        if(const ClientChunk *neighbour = find(cpos + chunk_pos_t(0, 0, 1)))
            forceFullStaticMeshUpdate(neighbour->entity);
        if(const ClientChunk *neighbour = find(cpos - chunk_pos_t(0, 0, 1)))
            forceFullStaticMeshUpdate(neighbour->entity);
        if(const ClientChunk *neighbour = find(cpos + chunk_pos_t(0, 1, 0)))
            forceFullStaticMeshUpdate(neighbour->entity);
        if(const ClientChunk *neighbour = find(cpos - chunk_pos_t(0, 1, 0)))
            forceFullStaticMeshUpdate(neighbour->entity);
        if(const ClientChunk *neighbour = find(cpos + chunk_pos_t(1, 0, 0)))
            forceFullStaticMeshUpdate(neighbour->entity);
        if(const ClientChunk *neighbour = find(cpos - chunk_pos_t(1, 0, 0)))
            forceFullStaticMeshUpdate(neighbour->entity);
    }
    chunk->data.at(world::getVoxelIndex(lpos)) = voxel;
    forceFullStaticMeshUpdate(chunk->entity);
}