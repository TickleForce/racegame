#include "terrain.h"
#include "game.h"
#include "renderer.h"
#include "debug_draw.h"
#include "scene.h"

void Terrain::createBuffers()
{
    glCreateBuffers(1, &vbo);
    glCreateBuffers(1, &ebo);

    enum
    {
        POSITION_BIND_INDEX = 0,
        NORMAL_BIND_INDEX = 1,
        BLEND_BIND_INDEX = 2,
    };

    glCreateVertexArrays(1, &vao);

    glEnableVertexArrayAttrib(vao, POSITION_BIND_INDEX);
    glVertexArrayAttribFormat(vao, POSITION_BIND_INDEX, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, POSITION_BIND_INDEX, 0);

    glEnableVertexArrayAttrib(vao, NORMAL_BIND_INDEX);
    glVertexArrayAttribFormat(vao, NORMAL_BIND_INDEX, 3, GL_FLOAT, GL_FALSE, 12);
    glVertexArrayAttribBinding(vao, NORMAL_BIND_INDEX, 0);

    glEnableVertexArrayAttrib(vao, BLEND_BIND_INDEX);
    glVertexArrayAttribFormat(vao, BLEND_BIND_INDEX, 4, GL_UNSIGNED_BYTE, GL_TRUE, 12 + 12);
    glVertexArrayAttribBinding(vao, BLEND_BIND_INDEX, 0);
}

void Terrain::generate(f32 heightScale, f32 scale)
{
    i32 width = (i32)((x2 - x1) / tileSize);
    i32 height = (i32)((y2 - y1) / tileSize);
    for (i32 x = 0; x < width; ++x)
    {
        for (i32 y = 0; y < height; ++y)
        {
#if 0
            heightBuffer[y * width + x] =
                glm::perlin(Vec2(x, y) * scale) * heightScale +
                glm::perlin(Vec2(x, y) * scale * 4.f) * 0.5f;
#endif
        }
    }
    setDirty();
}

void Terrain::resize(f32 x1, f32 y1, f32 x2, f32 y2, bool preserve)
{
    // record old info
    OwnedPtr<f32[]> oldHeightBuffer = move(heightBuffer);
    i32 ow = (i32)((this->x2 - this->x1) / tileSize);
    i32 oh = (i32)((this->y2 - this->y1) / tileSize);
    OwnedPtr<u32[]> oldBlend = move(blend);
    i32 xOffset = (i32)((this->x1 - x1) / tileSize);
    i32 yOffset = (i32)((this->y1 - y1) / tileSize);

    // allocate and clear new buffers
    this->x1 = snap(x1, tileSize);
    this->y1 = snap(y1, tileSize);
    this->x2 = snap(x2, tileSize);
    this->y2 = snap(y2, tileSize);
    i32 width = (i32)((this->x2 - this->x1) / tileSize);
    i32 height = (i32)((this->y2 - this->y1) / tileSize);
	heightBufferSize = width * height;
    heightBuffer.reset(new f32[heightBufferSize]);
    for (u32 i = 0; i < heightBufferSize; ++i)
    {
		heightBuffer[i] = 0.f;
    }
    vertices.reset(new Vertex[heightBufferSize]);
	indices.reset(new u32[heightBufferSize * 6]);
    blend.reset(new u32[heightBufferSize]);
	for (u32 i = 0; i < heightBufferSize; ++i)
	{
		blend[i] = 0x000000FF;
	}

    // copy over old
    if (preserve)
    {
        i32 startOffsetX = max(0, -xOffset);
        i32 startOffsetY = max(0, -yOffset);
        i32 w = min(width - xOffset, ow);
        i32 h = min(height - yOffset, oh);
        for (i32 x=startOffsetX; x<w; ++x)
        {
            for (i32 y=startOffsetY; y<h; ++y)
            {
                heightBuffer[(y + yOffset) * width + x + xOffset] = oldHeightBuffer[y * ow + x];
                blend[(y + yOffset) * width + x + xOffset] = oldBlend[y * ow + x];
            }
        }
    }

    setDirty();
}

void Terrain::onCreate(Scene* scene)
{
    actor = g_game.physx.physics->createRigidStatic(PxTransform(PxIdentity));
    physicsUserData.entityType = ActorUserData::ENTITY;
    physicsUserData.entity = this;
    actor->userData = &physicsUserData;
    scene->getPhysicsScene()->addActor(*actor);

    materials[0] = g_game.physx.materials.generic;
    materials[1] = g_game.physx.materials.offroad;

    regenerateMesh();
    regenerateCollisionMesh(scene);
    regenerateMaterial();

    if (!scene->terrain)
    {
        scene->terrain = this;
    }
}

void Terrain::onRender(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    regenerateMaterial();

    /*
    u32 width = (x2 - x1) / tileSize;
    u32 height = (y2 - y1) / tileSize;
    for (u32 x = 0; x < width - 1; ++x)
    {
        for (i32 y = 0; y < height - 1; ++y)
        {
            f32 z1 = heightBuffer[(y * width) + x];
            Vec3 pos1(x1 + x * tileSize, y1 + y * tileSize, z1);

            f32 z2 = heightBuffer[((y + 1) * width) + x];
            Vec3 pos2(x1 + x * tileSize, y1 + (y + 1) * tileSize, z2);

            f32 z3 = heightBuffer[(y * width) + x + 1];
            Vec3 pos3(x1 + (x + 1) * tileSize, y1 + y * tileSize, z3);

            const Vec4 color(0.f, 1.f, 0.f, 1.f);
            scene->debugDraw.line(pos1, pos2, color, color);
            scene->debugDraw.line(pos1, pos3, color, color);
        }
    }
    */
    auto renderDepth = [](void* renderData){
        Terrain* terrain = (Terrain*)renderData;
        glBindVertexArray(terrain->vao);
        glDrawElements(GL_TRIANGLES, terrain->indexCount, GL_UNSIGNED_INT, 0);
    };
    auto renderColor = [](void* renderData){
        Terrain* t = (Terrain*)renderData;
        glBindTextureUnit(6, t->textures[0]->handle);
        glBindTextureUnit(7, t->textures[1]->handle);
        glBindTextureUnit(8, t->textures[2]->handle);
        glBindTextureUnit(9, t->textures[3]->handle);
        glBindTextureUnit(10, t->normalTextures[0]->handle);
        glBindTextureUnit(11, t->normalTextures[1]->handle);
        glBindTextureUnit(12, t->normalTextures[2]->handle);
        glBindTextureUnit(13, t->normalTextures[3]->handle);
		if (g_game.isEditing)
		{
			glUniform3fv(3, 1, (GLfloat*)&t->brushSettings);
			glUniform3fv(4, 1, (GLfloat*)&t->brushPosition);
		}
        glUniform4fv(5, 1, t->texScale);
		if (g_game.config.graphics.highQualityTerrainEnabled)
		{
			glUniform3fv(6, 1, (f32*)&t->fresnel[0]);
			glUniform3fv(7, 1, (f32*)&t->fresnel[1]);
			glUniform3fv(8, 1, (f32*)&t->fresnel[2]);
			glUniform3fv(9, 1, (f32*)&t->fresnel[3]);
		}
		glBindVertexArray(t->vao);
        glDrawElements(GL_TRIANGLES, t->indexCount, GL_UNSIGNED_INT, 0);

        // reset the brush settings every frame
        t->setBrushSettings(1.f, 1.f, 1.f, { 0, 0, 1000000 });
    };
    rw->depthPrepass(depthShader, { this, renderDepth });
    rw->shadowPass(depthShader, { this, renderDepth });
    rw->opaqueColorPass(g_game.isEditing ? colorShaderWithBrush : colorShader, { this, renderColor });
}

void Terrain::regenerateMaterial()
{
    if (!materialGuid)
    {
        //materialGuid = g_res.getMaterial("terrain_grass_material")->guid;
        materialGuid = 0x7157416d2f682da3;
        material = g_res.getMaterial(materialGuid);
    }
    material = g_res.getMaterial(materialGuid);
    for (u32 i=0; i<NUM_TERRAIN_LAYERS; ++i)
    {
        auto& l = material->terrainLayers[i];
        textures[i] = g_res.getTexture(l.colorTextureGuid);
        normalTextures[i] = g_res.getTexture(l.normalTextureGuid);
        texScale[i] = l.textureScale;
        fresnel[i] = Vec3(l.fresnelBias, l.fresnelScale, l.fresnelPower);
    }
}

bool Terrain::isOffroadAt(f32 x, f32 y) const
{
    u32 ux = getCellX(x);
    u32 uy = getCellX(y);
    i32 width = (i32)((x2 - x1) / tileSize);
    return (blend[uy * width + ux] & 0x00FF0000) > OFFROAD_THRESHOLD;
}

Vec3 Terrain::computeNormal(u32 width, u32 height, u32 x, u32 y)
{
    x = clamp(x, 1u, width - 2);
    y = clamp(y, 1u, height - 2);
    f32 hl = heightBuffer[y * width + x - 1];
    f32 hr = heightBuffer[y * width + x + 1];
    f32 hd = heightBuffer[(y - 1) * width + x];
    f32 hu = heightBuffer[(y + 1) * width + x];
    Vec3 normal(hl - hr, hd - hu, 2.f);
    return normalize(normal);
}

void Terrain::regenerateMesh()
{
    // TODO: Update just the section of the mesh that has changed (buffer subdata)
    if (!isDirty) { return; }
    isDirty = false;
    i32 width = (i32)((x2 - x1) / tileSize);
    i32 height = (i32)((y2 - y1) / tileSize);
	u32 indexIndex = 0;
    for (i32 x = 0; x < width; ++x)
    {
        for (i32 y = 0; y < height; ++y)
        {
            f32 z = heightBuffer[(y * width) + x];
            Vec3 pos(x1 + x * tileSize, y1 + y * tileSize, z);
            Vec3 normal = computeNormal(width, height, x, y);
            u32 i = y * width + x;
            vertices[i] = {
                pos,
                normal,
                blend[i]
            };
            if (x < width - 1 && y < height - 1)
            {
                if ((x & 1) ? (y & 1) : !(y & 1))
                {
					indices[indexIndex + 0] = y * width + x;
                    indices[indexIndex + 1] = y * width + x + 1;
                    indices[indexIndex + 2] = (y + 1) * width + x;

                    indices[indexIndex + 3] = (y + 1) * width + x;
                    indices[indexIndex + 4] = y * width + x + 1;
                    indices[indexIndex + 5] = (y + 1) * width + x + 1;
                }
                else
                {
                    indices[indexIndex + 0] = y * width + x;
                    indices[indexIndex + 1] = y * width + x + 1;
                    indices[indexIndex + 2] = (y + 1) * width + x + 1;

                    indices[indexIndex + 3] = (y + 1) * width + x + 1;
                    indices[indexIndex + 4] = (y + 1) * width + x;
                    indices[indexIndex + 5] = y * width + x;
                }
				indexIndex += 6;
            }
        }
    }
	indexCount = indexIndex;
    glNamedBufferData(vbo, heightBufferSize * sizeof(Vertex), vertices.get(), GL_DYNAMIC_DRAW);
    glNamedBufferData(ebo, indexCount * sizeof(u32), indices.get(), GL_DYNAMIC_DRAW);
    glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(Vertex));
    glVertexArrayElementBuffer(vao, ebo);
}

void Terrain::regenerateCollisionMesh(Scene* scene)
{
    if (!isCollisionMeshDirty) { return; }
    isCollisionMeshDirty = false;
    PxTriangleMeshDesc desc;
    desc.points.count = heightBufferSize;
    desc.points.stride = sizeof(Vertex);
    desc.points.data = vertices.get();
    desc.triangles.count = indexCount / 3;
    desc.triangles.stride = 3 * sizeof(indices[0]);
    desc.triangles.data = indices.get();

	u32 materialIndexCount = indexCount / 3;
    materialIndices.reset(new PxMaterialTableIndex[materialIndexCount]);
    for (u32 i=0; i<materialIndexCount; ++i)
    {
        u32 threshold = OFFROAD_THRESHOLD;
        u32 vertexIndex1 = indices[i * 3 + 0];
        u32 vertexIndex2 = indices[i * 3 + 1];
        u32 vertexIndex3 = indices[i * 3 + 2];
        materialIndices[i] = ((blend[vertexIndex1] & 0x00FF0000) > threshold ||
                              (blend[vertexIndex2] & 0x00FF0000) > threshold ||
                              (blend[vertexIndex3] & 0x00FF0000) > threshold) ? 1 : 0;
    }

    PxTypedStridedData<PxMaterialTableIndex> materialIndexData;
    materialIndexData.data = materialIndices.get();
    materialIndexData.stride = sizeof(PxMaterialTableIndex);
    desc.materialIndices = materialIndexData;

    PxDefaultMemoryOutputStream writeBuffer;
    if (!g_game.physx.cooking->cookTriangleMesh(desc, writeBuffer))
    {
        FATAL_ERROR("Failed to create collision mesh for terrain");
    }

    PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
    PxTriangleMesh* triMesh = g_game.physx.physics->createTriangleMesh(readBuffer);
    if (actor->getNbShapes() > 0)
    {
        PxShape* shape;
        actor->getShapes(&shape, 1);
        shape->setGeometry(PxTriangleMeshGeometry(triMesh));
    }
    else
    {
        PxShape* shape = PxRigidActorExt::createExclusiveShape(*actor,
                PxTriangleMeshGeometry(triMesh), materials, ARRAY_SIZE(materials));
        shape->setQueryFilterData(PxFilterData(COLLISION_FLAG_TERRAIN, DECAL_TERRAIN, 0, DRIVABLE_SURFACE));
        shape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_TERRAIN, -1, 0, 0));
        shape->setFlag(PxShapeFlag::eVISUALIZATION, false);
    }
    triMesh->release();
}

f32 Terrain::getZ(Vec2 pos) const
{
    u32 width = (u32)((x2 - x1) / tileSize);
    u32 height = (u32)((y2 - y1) / tileSize);
    f32 x = clamp((pos.x - x1) / tileSize - 0.5f, 0.f, width - 1.f);
    f32 y = clamp((pos.y - y1) / tileSize - 0.5f, 0.f, height - 1.f);
    u32 px = (u32)x;
    u32 py = (u32)y;

    f32 c00 = heightBuffer[(py + 0) * width + px + 0];
    f32 c10 = heightBuffer[(py + 0) * width + px + 1];
    f32 c01 = heightBuffer[(py + 1) * width + px + 0];
    f32 c11 = heightBuffer[(py + 1) * width + px + 1];

    f32 tx = x - px;
    f32 ty = y - py;

    return (1 - tx) * (1 - ty) * c00 +
        tx * (1 - ty) * c10 + (1 - tx) * ty * c01 + tx * ty * c11;
}

i32 Terrain::getCellX(f32 x) const
{
    i32 width = (i32)((x2 - x1) / tileSize);
    return clamp((i32)((x - x1) / tileSize), 0, width - 1);
}

i32 Terrain::getCellY(f32 y) const
{
    i32 height = (i32)((y2 - y1) / tileSize);
    return clamp((i32)((y - y1) / tileSize), 0, height - 1);
}

void Terrain::raise(Vec2 pos, f32 radius, f32 falloff, f32 amount)
{
    i32 minX = getCellX(pos.x - radius);
    i32 minY = getCellY(pos.y - radius);
    i32 maxX = getCellX(pos.x + radius);
    i32 maxY = getCellY(pos.y + radius);
    i32 width = (i32)((x2 - x1) / tileSize);
    for (i32 x=minX; x<=maxX; ++x)
    {
        for (i32 y=minY; y<=maxY; ++y)
        {
            Vec2 p(x1 + x * tileSize, y1 + y * tileSize);
            f32 t = powf(clamp(1.f - (length(pos - p) / radius), 0.f, 1.f), falloff);
            heightBuffer[y * width + x] += t * amount;
        }
    }
    setDirty();
}

void Terrain::perturb(Vec2 pos, f32 radius, f32 falloff, f32 amount)
{
    i32 minX = getCellX(pos.x - radius);
    i32 minY = getCellY(pos.y - radius);
    i32 maxX = getCellX(pos.x + radius);
    i32 maxY = getCellY(pos.y + radius);
    i32 width = (i32)((x2 - x1) / tileSize);
    for (i32 x=minX; x<=maxX; ++x)
    {
        for (i32 y=minY; y<=maxY; ++y)
        {
#if 0
            Vec2 p(x1 + x * tileSize, y1 + y * tileSize);
            f32 scale = 0.1f;
            f32 noise = glm::perlin(Vec2(x, y) * scale);
            f32 t = powf(clamp(1.f - (length(pos - p) / radius), 0.f, 1.f), falloff);
            heightBuffer[y * width + x] += t * noise * amount;
#endif
        }
    }
    setDirty();
}

void Terrain::flatten(Vec2 pos, f32 radius, f32 falloff, f32 amount, f32 z)
{
    i32 minX = getCellX(pos.x - radius);
    i32 minY = getCellY(pos.y - radius);
    i32 maxX = getCellX(pos.x + radius);
    i32 maxY = getCellY(pos.y + radius);
    i32 width = (i32)((x2 - x1) / tileSize);
    for (i32 x=minX; x<=maxX; ++x)
    {
        for (i32 y=minY; y<=maxY; ++y)
        {
            Vec2 p(x1 + x * tileSize, y1 + y * tileSize);
            f32 t = powf(clamp(1.f - (length(pos - p) / radius), 0.f, 1.f), falloff);
            f32 currentZ = heightBuffer[y * width + x];
            heightBuffer[y * width + x] += (z - currentZ) * t * amount;
        }
    }
    setDirty();
}

void Terrain::smooth(Vec2 pos, f32 radius, f32 falloff, f32 amount)
{
    i32 minX = getCellX(pos.x - radius);
    i32 minY = getCellY(pos.y - radius);
    i32 maxX = getCellX(pos.x + radius);
    i32 maxY = getCellY(pos.y + radius);
    i32 width = (i32)((x2 - x1) / tileSize);
    i32 height = (i32)((y2 - y1) / tileSize);
    for (i32 x=minX; x<=maxX; ++x)
    {
        for (i32 y=minY; y<=maxY; ++y)
        {
            Vec2 p(x1 + x * tileSize, y1 + y * tileSize);
            f32 t = powf(clamp(1.f - (length(pos - p) / radius), 0.f, 1.f), falloff);
            f32 hl = heightBuffer[y * width + clamp(x - 1, 0, width - 1)];
            f32 hr = heightBuffer[y * width + clamp(x + 1, 0, width - 1)];
            f32 hd = heightBuffer[clamp(y - 1, 0, height - 1) * width + x];
            f32 hu = heightBuffer[clamp(y + 1, 0, height - 1) * width + x];
            f32 currentZ = heightBuffer[y * width + x];
            f32 average = (hl + hr + hd + hu) * 0.25f;
            heightBuffer[y * width + x] += (average - currentZ) * t * amount;
        }
    }
    setDirty();
}

// adapted from http://ranmantaru.com/blog/2011/10/08/water-erosion-on-heightmap-terrain/
void Terrain::erode(Vec2 pos, f32 radius, f32 falloff, f32 amount)
{
    amount *= 0.03f;

    i32 minX = getCellX(pos.x - radius);
    i32 minY = getCellY(pos.y - radius);
    i32 maxX = getCellX(pos.x + radius);
    i32 maxY = getCellY(pos.y + radius);
    i32 width = (i32)((x2 - x1) / tileSize);
    i32 height = (i32)((y2 - y1) / tileSize);

    f32 Kq = 5; // soil carrying capacity
    f32 Kw = 0.006f; // evaporation speed
    f32 Kr = 0.2f; // erosion speed
    f32 Kd = 0.4f; // deposition speed
    f32 Ki = 0.1f; // direction inertia (higher makes smoother channel turns)
    f32 minSlope = 0.06f; // soil carrying capacity
    f32 g = 15; // gravity
    f32 Kg = g * 2;
    f32 scale = 40.f;

    OwnedPtr<Vec2[]> erosion(new Vec2[width * height]);

    const u32 MAX_PATH_LEN = 10;

#define HMAP(X, Y) (heightBuffer[(Y) * width + (X)] / scale)

#define DEPOSIT_AT(X, Z, W) \
{ \
f32 delta = ds * (W) * amount; \
erosion[clamp((Z) * width + (X), 0, (i32)heightBufferSize)].y += delta; \
heightBuffer[clamp((Z) * width + (X), 0, (i32)heightBufferSize)] += delta * scale; \
}

#define DEPOSIT(H) \
DEPOSIT_AT(xi  , zi  , (1-xf)*(1-zf)) \
DEPOSIT_AT(xi+1, zi  ,    xf *(1-zf)) \
DEPOSIT_AT(xi  , zi+1, (1-xf)*   zf ) \
DEPOSIT_AT(xi+1, zi+1,    xf *   zf ) \
(H)+=ds;

    const u32 iterations = (u32)(radius * radius);
    for (u32 i =0; i < iterations; ++i)
    {
        //i32 xi = irandom(randomSeries, 0, width - 1);
        //i32 zi = irandom(randomSeries, 0, height - 1);
        i32 xi = irandom(randomSeries, minX, maxX);
        i32 zi = irandom(randomSeries, minY, maxY);
        f32 xp = (f32)xi, zp = (f32)zi;
        f32 xf = 0, zf = 0;
        f32 h = HMAP(xi, zi);
        f32 s = 0, v = 0, w = 1;
        f32 h00 = h;
        f32 h10 = HMAP(xi+1, zi  );
        f32 h01 = HMAP(xi  , zi+1);
        f32 h11 = HMAP(xi+1, zi+1);
        f32 dx = 0, dz = 0;
        for (u32 numMoves = 0; numMoves < MAX_PATH_LEN; ++numMoves)
        {
            f32 gx = h00 + h01 - h10 - h11;
            f32 gz = h00 + h10 - h01 - h11;
            dx = (dx - gx) * Ki + gx;
            dz = (dz - gz) * Ki + gz;

            f32 dl = sqrtf(dx * dx + dz * dz);
            if (dl <= FLT_EPSILON)
            {
                f32 a = random(randomSeries, 0.f, PI * 2.f);
                dx = cosf(a);
                dz = sinf(a);
            }
            else
            {
                dx /= dl;
                dz /= dl;
            }

            f32 nxp = xp + dx;
            f32 nzp = zp + dz;

            i32 nxi = (i32)floorf(nxp);
            i32 nzi = (i32)floorf(nzp);
            f32 nxf = nxp - nxi;
            f32 nzf = nzp - nzi;

            f32 nh00 = HMAP(nxi  , nzi  );
            f32 nh10 = HMAP(nxi+1, nzi  );
            f32 nh01 = HMAP(nxi  , nzi+1);
            f32 nh11 = HMAP(nxi+1, nzi+1);

            f32 nh = (nh00 * (1 - nxf) + nh10 * nxf) * (1 - nzf) + (nh01 * (1 - nxf) + nh11 * nxf) * nzf;
            if (nh >= h)
            {
                f32 ds = (nh - h) + 0.001f;
                if (ds >= s)
                {
                    ds = s;
                    DEPOSIT(h)
                    s = 0;
                    break;
                }
                DEPOSIT(h)
                s -= ds;
                v = 0;
            }

            f32 dh = h - nh;
            f32 slope = dh;
            f32 q = max(slope, minSlope) * v * w * Kq;
            f32 ds = s - q;
            if (ds >= 0)
            {
                ds *= Kd;
                DEPOSIT(dh)
                s -= ds;
            }
            else
            {
                ds *= -Kr;
                ds = min(ds, dh * 0.99f);

#define ERODE(X, Z, W) \
{ \
f32 delta = ds * (W) * amount; \
heightBuffer          [clamp((Z) * width + (X), 0, (i32)heightBufferSize)] -= delta * scale; \
Vec2 &e = erosion[clamp((Z) * width + (X), 0, (i32)heightBufferSize)]; \
f32 r=e.x, d=e.y; \
if (delta<=d) d-=delta; \
else { r+=delta-d; d=0; } \
e.x=r; e.y=d; \
}

                for (i32 z = zi - 1; z <= zi + 2; ++z)
                {
                    f32 zo = z - zp;
                    f32 zo2 = zo * zo;
                    for (i32 x = xi - 1; x <= xi + 2; ++x)
                    {
                        f32 xo = x - xp;
                        f32 w = 1 - (xo * xo + zo2) * 0.25f;
                        if (w <= 0)
                        {
                            continue;
                        }
                        w *= 0.1591549430918953f;
                        ERODE(x, z, w)
                    }
                }
                dh -= ds;
                s += ds;
            }

            v = sqrtf(v * v + Kg * dh);
            w *= 1 - Kw;

            xp = nxp;
            zp = nzp;
            xi = nxi;
            zi = nzi;
            xf = nxf;
            zf = nzf;

            h = nh;
            h00 = nh00;
            h10 = nh10;
            h01 = nh01;
            h11 = nh11;
        }
    }
    setDirty();
}

void Terrain::matchTrack(Vec2 pos, f32 radius, f32 falloff, f32 amount, Scene* scene)
{
    i32 minX = getCellX(pos.x - radius);
    i32 minY = getCellY(pos.y - radius);
    i32 maxX = getCellX(pos.x + radius);
    i32 maxY = getCellY(pos.y + radius);
    i32 width = (i32)((x2 - x1) / tileSize);
    for (i32 x=minX; x<=maxX; ++x)
    {
        for (i32 y=minY; y<=maxY; ++y)
        {
            Vec2 p(x1 + x * tileSize, y1 + y * tileSize);
            f32 t = powf(clamp(1.f - (length(pos - p) / radius), 0.f, 1.f), falloff);
            f32 currentZ = heightBuffer[y * width + x];
            f32 z = currentZ;
            Vec3 from = Vec3(p.x, p.y, 1000.f);
            Vec3 rayDir = Vec3(0, 0, -1);
            PxRaycastBuffer rayHit;
            if (scene->raycastStatic(from, rayDir, 10000.f, &rayHit, COLLISION_FLAG_TRACK))
            {
                z = rayHit.block.position.z - 0.15f;
            }
            else
            {
                PxSweepBuffer sweepHit;
                if (scene->sweepStatic(9.f, from, rayDir, 10000.f, &sweepHit, COLLISION_FLAG_TRACK))
                {
                    z = sweepHit.block.position.z - 0.45f;
                }
            }
            heightBuffer[y * width + x] += (z - currentZ) * t * amount;
        }
    }
    setDirty();
}

void Terrain::paint(Vec2 pos, f32 radius, f32 falloff, f32 amount, u32 materialIndex)
{
    i32 minX = getCellX(pos.x - radius);
    i32 minY = getCellY(pos.y - radius);
    i32 maxX = getCellX(pos.x + radius);
    i32 maxY = getCellY(pos.y + radius);
    i32 width = (i32)((x2 - x1) / tileSize);
    for (i32 x=minX; x<=maxX; ++x)
    {
        for (i32 y=minY; y<=maxY; ++y)
        {
            Vec2 p(x1 + x * tileSize, y1 + y * tileSize);
            f32 t = powf(clamp(1.f - (length(pos - p) / radius), 0.f, 1.f), falloff);
            u8* b = reinterpret_cast<u8*>(&blend[y * width + x]);
            Vec4 bl = Vec4(b[0], b[1], b[2], b[3]) / 255.f;
            bl[materialIndex] += t * amount;
            bl /= bl.x + bl.y + bl.z + bl.w;
            b[0] = u8(bl[0] * 255.f);
            b[1] = u8(bl[1] * 255.f);
            b[2] = u8(bl[2] * 255.f);
            b[3] = u8(bl[3] * 255.f);
        }
    }
    setDirty();
}

void Terrain::serializeState(Serializer& s)
{
    s.field(tileSize);
    s.field(x1);
    s.field(y1);
    s.field(x2);
    s.field(y2);
    s.field(materialGuid);

    if (s.deserialize)
    {
	    resize(x1, y1, x2, y2);
        auto& heightBufferBytes = s.dict["heightBuffer"].bytearray().val();
	    memcpy(heightBuffer.get(), heightBufferBytes.data(), heightBufferBytes.size());
        if (s.dict["blendBuffer"].hasValue())
        {
            auto& blendBytes = s.dict["blendBuffer"].bytearray().val();
		    memcpy(blend.get(), blendBytes.data(), blendBytes.size());
        }
    }
    else
    {
        s.dict["heightBuffer"] = DataFile::makeBytearray(DataFile::Value::ByteArray(
                            (u8*)heightBuffer.get(),
                            (u8*)(heightBuffer.get() + heightBufferSize)));
        s.dict["blendBuffer"] = DataFile::makeBytearray(DataFile::Value::ByteArray(
                            (u8*)blend.get(),
                            (u8*)(blend.get() + heightBufferSize)));
    }
}

void Terrain::applyDecal(Decal& decal)
{
    i32 width = (i32)((x2 - x1) / tileSize);
    i32 height = (i32)((y2 - y1) / tileSize);
    Array<u32> collisionIndices;
    collisionIndices.reserve(256);
    BoundingBox bb = decal.getBoundingBox();
    i32 startX = getCellX(bb.min.x);
    i32 startY = getCellY(bb.min.y);
    i32 endX = min(getCellX(bb.max.x) + 1, width - 1);
    i32 endY = min(getCellY(bb.max.y) + 1, height - 1);
    for (i32 x = startX; x < endX; ++x)
    {
        for (i32 y = startY; y < endY; ++y)
        {
            if (x < width - 1 && y < height - 1)
            {
                if ((x & 1) ? (y & 1) : !(y & 1))
                {
                    collisionIndices.push(y * width + x);
                    collisionIndices.push(y * width + x + 1);
                    collisionIndices.push((y + 1) * width + x);

                    collisionIndices.push((y + 1) * width + x);
                    collisionIndices.push(y * width + x + 1);
                    collisionIndices.push((y + 1) * width + x + 1);
                }
                else
                {
                    collisionIndices.push(y * width + x);
                    collisionIndices.push(y * width + x + 1);
                    collisionIndices.push((y + 1) * width + x + 1);

                    collisionIndices.push((y + 1) * width + x + 1);
                    collisionIndices.push((y + 1) * width + x);
                    collisionIndices.push(y * width + x);
                }
            }
        }
    }
    decal.addMesh((f32*)vertices.get(), sizeof(Vertex),
            collisionIndices.data(), (u32)collisionIndices.size(), Mat4(1.f));
}
