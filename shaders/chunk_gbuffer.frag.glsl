/*
 * Copyright (c) 2021, Kirill GPRB. All Rights Reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#version 460

in VS_OUTPUT {
    vec3 model;
    vec3 normal;
    vec3 texcoord;
} vso;

layout(location = 0) out vec3 albedo;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec3 position;

layout(binding = 0) uniform sampler2DArray atlas;

void main()
{
    albedo = texture(atlas, vso.texcoord).rgb;
    normal = normalize(vso.normal);
    position = vso.model;
}