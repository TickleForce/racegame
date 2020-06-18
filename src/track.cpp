#include "track.h"
#include "scene.h"
#include "renderer.h"
#include "input.h"
#include "game.h"
#include "track_graph.h"
#include "2d.h"
#include "entities/start.h"

Vec4 red = { 1.f, 0.f, 0.f, 1.f };
Vec4 brightRed = { 1.f, 0.25f, 0.25f, 1.f };
Vec4 orange = { 1.f, 0.5f, 0.f, 1.f };
Vec4 brightOrange = { 1.f, 0.65f, 0.1f, 1.f };
Vec4 blue = { 0.f, 0.0f, 1.f, 1.f };
Vec4 brightBlue = { 0.25f, 0.25f, 1.0f, 1.f };

void Track::onCreate(Scene* scene)
{
    actor = g_game.physx.physics->createRigidStatic(PxTransform(PxIdentity));
    physicsUserData.entityType = ActorUserData::ENTITY;
    physicsUserData.entity = this;
    actor->userData = &physicsUserData;
    scene->getPhysicsScene()->addActor(*actor);
    this->scene = scene;
    for (auto& c : connections)
    {
        if (c->isDirty || c->vertices.empty())
        {
            createSegmentMesh(*c, scene);
        }
    }
    if (!scene->track)
    {
        scene->track = this;
    }
}

void Track::onRender(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    for (auto& c : connections)
    {
        /*
        Vec3 prevP;
        for (f32 t=0.f; t<=1.f; t+=0.01f)
        {
            Vec3 p = pointOnBezierCurve(
                    points[c.pointIndexA].position,
                    points[c.pointIndexA].position + c.handleOffsetA,
                    points[c.pointIndexB].position + c.handleOffsetB,
                    points[c.pointIndexB].position, t);
            if (t > 0.f)
            {
                scene->debugDraw.line(p, prevP, orange, orange);
            }
            prevP = p;
        }
        */
        if (c->isDirty || c->vertices.empty())
        {
            createSegmentMesh(*c, scene);
        }
    }

    auto renderDepth = [](void* renderData){
        Track* track = (Track*)renderData;
        for (auto& c : track->connections)
        {
            glBindVertexArray(c->vao);
            glDrawElements(GL_TRIANGLES, (GLsizei)c->indices.size(), GL_UNSIGNED_INT, 0);
        }
    };
    auto renderColor = [](void* renderData){
        Track* track = (Track*)renderData;
        glBindTextureUnit(0, g_res.getTexture("tarmac")->handle);
        glBindTextureUnit(5, g_res.getTexture("tarmac_normal")->handle);
        glBindTextureUnit(6, g_res.getTexture("tarmac_spec")->handle);
        for (auto& c : track->connections)
        {
            glBindVertexArray(c->vao);
            glDrawElements(GL_TRIANGLES, (GLsizei)c->indices.size(), GL_UNSIGNED_INT, 0);
        }
    };
    rw->depthPrepass(depthShader, { this, renderDepth });
    rw->shadowPass(depthShader, { this, renderDepth });
    rw->opaqueColorPass(colorShader, { this, renderColor });
}

void Track::clearSelection()
{
    selectedPoints.clear();
}

void Track::trackModeUpdate(Renderer* renderer, Scene* scene, f32 deltaTime, bool& isMouseHandled, GridSettings* gridSettings)
{
    RenderWorld* rw = renderer->getRenderWorld();
    Mesh* sphere = g_res.getModel("misc")->getMeshByName("world.Sphere");
    Vec2 mousePos = g_input.getMousePosition();
    Camera const& cam = rw->getCamera(0);
    Vec3 rayDir = screenToWorldRay(mousePos,
            Vec2(g_game.windowWidth, g_game.windowHeight), cam.view, cam.projection);
    f32 radius = 18;

    if (!isMouseHandled && g_input.isMouseButtonPressed(MOUSE_LEFT)
            && !g_input.isKeyDown(KEY_LCTRL) && !g_input.isKeyDown(KEY_LSHIFT))
    {
        clearSelection();
    }

    // track points
    for (i32 i=0; i<(i32)points.size();)
    {
        Vec3 point = points[i].position;
        Vec2 pointScreen = project(point, rw->getCamera(0).viewProjection)
            * Vec2(g_game.windowWidth, g_game.windowHeight);

        auto it = selectedPoints.find([&i](auto& s) { return s.pointIndex == i; });
        bool isSelected = !!it;

        if (isSelected)
        {
            i32 d = (i32)g_input.isKeyPressed(KEY_Q) - (i32)g_input.isKeyPressed(KEY_E);
            if (d != 0)
            {
                points[i].position.z += 2.f * d;
                for (auto& c : connections)
                {
                    if (c->pointIndexA == i || c->pointIndexB == i)
                    {
                        c->isDirty = true;
                    }
                }
            }

            if (points.size() > 1 && connections.size() > 1 && g_input.isKeyPressed(KEY_DELETE))
            {
                for (auto it = connections.begin(); it != connections.end();)
                {
                    if (it->get()->pointIndexA == i || it->get()->pointIndexB == i)
                    {
                        connections.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
                selectedPoints.erase(it);
                points.erase(points.begin() + i);
                for (auto& conn : connections)
                {
                    if (conn->pointIndexA > i)
                    {
                        --conn->pointIndexA;
                    }
                    if (conn->pointIndexB > i)
                    {
                        --conn->pointIndexB;
                    }
                }
                continue;
            }
            gridSettings->z = point.z + 0.15f;
        }

        if (length(pointScreen - mousePos) < radius && !isDragging)
        {
            if (!isMouseHandled && g_input.isMouseButtonPressed(MOUSE_LEFT))
            {
                isMouseHandled = true;
                if (g_input.isKeyDown(KEY_LSHIFT))
                {
                    if (isSelected)
                    {
                        selectedPoints.erase(it);
                    }
                }
                else
                {
                    selectMousePos = mousePos;
                    if (!isSelected)
                    {
                        selectedPoints.push_back({ i, {} });
                    }
                }
            }
        }
        if (isSelected)
        {
            drawOverlay(rw, sphere,
                        glm::translate(Mat4(1.f), points[i].position) *
                        glm::scale(Mat4(1.08f), Vec3(1.f)), { 1, 1, 1 }, -1);
        }
        Vec3 color = isSelected ? brightRed : red;
        drawOverlay(rw, sphere,
                    glm::translate(Mat4(1.f), points[i].position) *
                    glm::scale(Mat4(1.f), Vec3(1.f)), color);
        ++i;
    }

    // track connections
    for (i32 i=0; i<(i32)connections.size(); ++i)
    {
        auto& c = connections[i];
        if (isDragging)
        {
            if (selectedPoints.find([&c](auto& s) {
                return s.pointIndex == c->pointIndexA || s.pointIndex == c->pointIndexB; }))
            {
                c->isDirty = true;
            }
        }

        f32 widthDiff = ((i32)g_input.isKeyPressed(KEY_Q) - (i32)g_input.isKeyPressed(KEY_E)) * 1.f;

        Vec3 colorA = orange;
        Vec3 handleA = points[c->pointIndexA].position + c->handleOffsetA;
        Vec2 handleAScreen = project(handleA, rw->getCamera(0).viewProjection)
            * Vec2(g_game.windowWidth, g_game.windowHeight);
        if (!isDragging && length(handleAScreen - mousePos) < radius)
        {
            colorA = brightOrange;
            if (!isMouseHandled && g_input.isMouseButtonPressed(MOUSE_LEFT))
            {
                isMouseHandled = true;
                dragConnectionIndex = i;
                dragConnectionHandle = 0;
                f32 t = rayPlaneIntersection(cam.position, rayDir, Vec3(0, 0, 1), handleA);
                Vec3 p = cam.position + rayDir * t;
                dragOffset = handleA - p;
                isDragging = true;
                // TODO: don't move the other handle if a certain key is pressed (ALT?)
                for (i32 connectionIndex = 0; connectionIndex < (i32)connections.size(); ++connectionIndex)
                {
                    auto const& c2 = connections[connectionIndex];
                    if (&c2 != &c)
                    {
                        if (c2->pointIndexB == c->pointIndexA)
                        {
                            if (1.f - dot(-normalize(c2->handleOffsetB),
                                        normalize(c->handleOffsetA)) < 0.01f)
                            {
                                dragOppositeConnectionIndex = connectionIndex;
                                dragOppositeConnectionHandle = 1;
                                break;
                            }
                        }
                        else if (c2->pointIndexA == c->pointIndexA)
                        {
                            if (1.f - dot(-normalize(c2->handleOffsetA),
                                        normalize(c->handleOffsetA)) < 0.01f)
                            {
                                dragOppositeConnectionIndex = connectionIndex;
                                dragOppositeConnectionHandle = 0;
                                break;
                            }
                        }
                    }
                }
            }
        }
        if (dragConnectionIndex == i && dragConnectionHandle == 0)
        {
            c->widthA += widthDiff;
            c->isDirty = true;
            colorA = brightOrange;
            f32 t = rayPlaneIntersection(cam.position, rayDir, Vec3(0, 0, 1), handleA);
            Vec3 p = cam.position + rayDir * t + dragOffset;
            if (gridSettings->snap)
            {
                c->handleOffsetA = snapXY(p, gridSettings->cellSize) - points[c->pointIndexA].position;
            }
            else
            {
                c->handleOffsetA = p - points[c->pointIndexA].position;
            }
            handleA = points[c->pointIndexA].position + c->handleOffsetA;
            if (dragOppositeConnectionIndex != -1)
            {
                auto& c2 = connections[dragOppositeConnectionIndex];
                c2->isDirty = true;
                if (dragOppositeConnectionHandle == 0)
                {
                    c2->handleOffsetA = -c->handleOffsetA;
                }
                else if (dragOppositeConnectionHandle == 1)
                {
                    c2->handleOffsetB = -c->handleOffsetA;
                }
            }
            drawOverlay(rw, sphere,
                        glm::translate(Mat4(1.f), handleA) *
                        glm::scale(Mat4(0.9f), Vec3(1.f)), { 1, 1, 1 }, -1);
        }

        Vec3 colorB = orange;
        Vec3 handleB = points[c->pointIndexB].position + c->handleOffsetB;
        Vec2 handleBScreen = project(handleB, rw->getCamera(0).viewProjection)
            * Vec2(g_game.windowWidth, g_game.windowHeight);
        if (!isDragging && length(handleBScreen - mousePos) < radius)
        {
            colorB = brightOrange;
            if (!isMouseHandled && g_input.isMouseButtonPressed(MOUSE_LEFT))
            {
                isMouseHandled = true;
                dragConnectionIndex = i;
                dragConnectionHandle = 1;
                f32 t = rayPlaneIntersection(cam.position, rayDir, Vec3(0, 0, 1), handleB);
                Vec3 p = cam.position + rayDir * t;
                dragOffset = handleB - p;
                isDragging = true;
                // TODO: don't move the other handle if a certain key is pressed (ALT?)
                for (i32 connectionIndex = 0; connectionIndex < (i32)connections.size(); ++connectionIndex)
                {
                    auto const& c2 = connections[connectionIndex];
                    if (c2 == c)
                    {
                        continue;
                    }
                    if (c2->pointIndexB == c->pointIndexB)
                    {
                        if (1.f - dot(-normalize(c2->handleOffsetB),
                                    normalize(c->handleOffsetB)) < 0.01f)
                        {
                            dragOppositeConnectionIndex = connectionIndex;
                            dragOppositeConnectionHandle = 1;
                            break;
                        }
                    }
                    else if (c2->pointIndexA == c->pointIndexB)
                    {
                        if (1.f - dot(-normalize(c2->handleOffsetA),
                                    normalize(c->handleOffsetB)) < 0.01f)
                        {
                            dragOppositeConnectionIndex = connectionIndex;
                            dragOppositeConnectionHandle = 0;
                            break;
                        }
                    }
                }
            }
        }
        if (dragConnectionIndex == i && dragConnectionHandle == 1)
        {
            c->widthB += widthDiff;
            c->isDirty = true;
            colorB = brightOrange;
            f32 t = rayPlaneIntersection(cam.position, rayDir, Vec3(0, 0, 1), handleB);
            Vec3 p = cam.position + rayDir * t + dragOffset;
            if (gridSettings->snap)
            {
                c->handleOffsetB = snapXY(p, gridSettings->cellSize) - points[c->pointIndexB].position;
            }
            else
            {
                c->handleOffsetB = p - points[c->pointIndexB].position;
            }
            handleB = points[c->pointIndexB].position + c->handleOffsetB;
            if (dragOppositeConnectionIndex != -1)
            {
                auto& c2 = connections[dragOppositeConnectionIndex];
                c2->isDirty = true;
                if (dragOppositeConnectionHandle == 0)
                {
                    c2->handleOffsetA = -c->handleOffsetB;
                }
                else if (dragOppositeConnectionHandle == 1)
                {
                    c2->handleOffsetB = -c->handleOffsetB;
                }
            }
            drawOverlay(rw, sphere,
                        glm::translate(Mat4(1.f), handleB) *
                        glm::scale(Mat4(0.9f), Vec3(1.f)), { 1, 1, 1 }, -1);
        }
        drawOverlay(rw, sphere,
                    glm::translate(Mat4(1.f), handleA) *
                    glm::scale(Mat4(0.8f), Vec3(1.f)), colorA);
        drawOverlay(rw, sphere,
                    glm::translate(Mat4(1.f), handleB) *
                    glm::scale(Mat4(0.8f), Vec3(1.f)), colorB);

        scene->debugDraw.line(points[c->pointIndexA].position + Vec3(0, 0, 0.01f),
                points[c->pointIndexA].position + c->handleOffsetA + Vec3(0, 0, 0.01f),
                Vec4(colorA, 1.f), Vec4(colorA, 1.f));
        scene->debugDraw.line(points[c->pointIndexB].position + Vec3(0, 0, 0.01f),
                points[c->pointIndexB].position + c->handleOffsetB + Vec3(0, 0, 0.01f),
                Vec4(colorB, 1.f), Vec4(colorB, 1.f));
    }

    // handle dragging of points
    if (dragConnectionHandle == -1
        && selectedPoints.size() > 0
        && (g_input.isMouseButtonDown(MOUSE_LEFT) && !isMouseHandled)
        && length(mousePos - selectMousePos) > g_game.windowHeight * 0.005f)
    {
        f32 t = 0.f;
        f32 startZ = 0.f;
        if (selectedPoints.size() > 0)
        {
            t = rayPlaneIntersection(cam.position, rayDir, Vec3(0, 0, 1),
                    points[selectedPoints.back().pointIndex].position);
            startZ = points[selectedPoints.back().pointIndex].position.z;
        }

        Vec3 hitPoint = cam.position + rayDir * t;
        if (!isDragging)
        {
            dragStartPoint = hitPoint;
        }

        Vec3 dragTranslation = hitPoint - dragStartPoint;
        for (auto& s : selectedPoints)
        {
            if (!isDragging)
            {
                s.dragStartPoint = points[s.pointIndex].position;
                s.dragStartPoint.z = startZ;
            }
            points[s.pointIndex].position = s.dragStartPoint + dragTranslation;
            if (gridSettings->snap)
            {
                Vec2 p = snapXY(points[s.pointIndex].position, gridSettings->cellSize);
                points[s.pointIndex].position = Vec3(p, points[s.pointIndex].position.z);
            }
        }

        isDragging = true;
    }

    if (g_input.isMouseButtonReleased(MOUSE_LEFT))
    {
        dragConnectionIndex = -1;
        dragConnectionHandle = -1;
        dragOppositeConnectionIndex = -1;
        dragOppositeConnectionHandle = -1;
        isDragging = false;
    }
}

void Track::matchZ(bool lowest)
{
    if (selectedPoints.size() > 0)
    {
        f32 z = points[selectedPoints[0].pointIndex].position.z;
        for (auto& p : selectedPoints)
        {
            if (lowest)
            {
                if (points[p.pointIndex].position.z < z)
                {
                    z = points[p.pointIndex].position.z;
                }
            }
            else
            {
                if (points[p.pointIndex].position.z > z)
                {
                    z = points[p.pointIndex].position.z;
                }
            }
        }
        for (auto& p : selectedPoints)
        {
            points[p.pointIndex].position.z = z;
        }
    }
}

void Track::extendTrack(i32 prefabCurveIndex)
{
    i32 pointIndex = getSelectedPointIndex();
    BezierSegment* bezierConnection = getPointConnection(pointIndex);
    Vec3 fromHandleOffset = (bezierConnection->pointIndexA == pointIndex)
        ? bezierConnection->handleOffsetA : bezierConnection->handleOffsetB;
    f32 fromWidth = (bezierConnection->pointIndexA == pointIndex)
        ? bezierConnection->widthA : bezierConnection->widthB;
    Vec3 xDir = getPointDir(pointIndex);
    Vec3 yDir = cross(xDir, Vec3(0, 0, 1));
    Vec3 zDir = cross(yDir, xDir);
    Mat4 m(1.f);
    m[0] = Vec4(xDir, m[0].w);
    m[1] = Vec4(yDir, m[1].w);
    m[2] = Vec4(zDir, m[2].w);
    i32 pIndex = pointIndex;
    selectedPoints.clear();
    for (u32 c = 0; c<prefabTrackItems[prefabCurveIndex].curves.size(); ++c)
    {
        Vec3 p = Vec3(m * Vec4(prefabTrackItems[prefabCurveIndex].curves[c].offset, 1.f))
            + points[pIndex].position;
        points.push_back({ p });
        Vec3 h(m * Vec4(prefabTrackItems[prefabCurveIndex].curves[c].handleOffset, 1.f));

        OwnedPtr<BezierSegment> segment(new BezierSegment);
        segment->track = this;
        segment->handleOffsetA = -fromHandleOffset;
        segment->pointIndexA = pIndex;
        segment->handleOffsetB = h;
        segment->pointIndexB = (i32)points.size() - 1;
        segment->widthA = fromWidth;
        segment->widthB = fromWidth;
        connections.push_back(std::move(segment));

        pIndex = (i32)points.size() - 1;
        fromHandleOffset = h;
        selectedPoints.push_back({ (i32)points.size() - 1, {} });
    }
}

void Track::subdividePoints()
{
}

// TODO: something is still wrong with this
void Track::connectPoints()
{
    if (selectedPoints.size() != 2)
    {
        return;
    }

    i32 index1 = selectedPoints[0].pointIndex;
    i32 index2 = selectedPoints[1].pointIndex;
    Point const& p1 = points[index1];
    Point const& p2 = points[index2];

    Vec3 handle1 = (p1.position - p2.position) * 0.3f;
    Vec3 handle2 = -handle1;

    u32 connectionCount1 = 0;
    u32 connectionCount2 = 0;
    for (auto& c : connections)
    {
        if (c->pointIndexA == index1 || c->pointIndexB == index1)
        {
            ++connectionCount1;
        }
        if (c->pointIndexA == index2 || c->pointIndexB == index2)
        {
            ++connectionCount2;
        }
    }

    if (connectionCount1 == 1)
    {
        auto c1 = getPointConnection(index1);
        handle1 = c1->pointIndexA == index1 ? c1->handleOffsetA : c1->handleOffsetB;
    }

    if (connectionCount2 == 1)
    {
        auto c2 = getPointConnection(index2);
        handle2 = c2->pointIndexA == index2 ? c2->handleOffsetA : c2->handleOffsetB;
    }

    OwnedPtr<BezierSegment> segment(new BezierSegment);
    segment->track = this;
    segment->handleOffsetA = -handle1;
    segment->pointIndexA = index1;
    if (BezierSegment* s = getPointConnection(index1))
    {
        segment->widthA = s->pointIndexA == index1 ? s->widthA : s->widthB;
    }
    segment->handleOffsetB = -handle2;
    segment->pointIndexB = index2;
    if (BezierSegment* s = getPointConnection(index2))
    {
        segment->widthB = s->pointIndexA == index2 ? s->widthA : s->widthB;
    }
    connections.push_back(std::move(segment));
}

Track::BezierSegment* Track::getPointConnection(i32 pointIndex)
{
    for (auto& c : connections)
    {
        if (c->pointIndexA == pointIndex || c->pointIndexB == pointIndex)
        {
            return c.get();
        }
    }
    return nullptr;
}

Vec3 Track::getPointDir(i32 pointIndex) const
{
    Vec3 dir;
    u32 count = 0;
    for (auto& c : connections)
    {
        if (c->pointIndexA == pointIndex)
        {
            ++count;
            if (count > 1)
            {
                break;
            }
            dir = -normalize(
                    pointOnBezierCurve(
                        points[c->pointIndexA].position,
                        points[c->pointIndexA].position + c->handleOffsetA,
                        points[c->pointIndexB].position + c->handleOffsetB,
                        points[c->pointIndexB].position, 0.01f) -
                    points[c->pointIndexA].position);
        }
        else if (c->pointIndexB == pointIndex)
        {
            ++count;
            if (count > 1)
            {
                break;
            }
            dir = -normalize(
                    pointOnBezierCurve(
                        points[c->pointIndexA].position,
                        points[c->pointIndexA].position + c->handleOffsetA,
                        points[c->pointIndexB].position + c->handleOffsetB,
                        points[c->pointIndexB].position, 0.99f) -
                    points[c->pointIndexB].position);
        }
    }
    if (count == 1)
    {
        return dir;
    }
    return { 0, 0, 0 };
}

void Track::createSegmentMesh(BezierSegment& c, Scene* scene)
{
    previewMesh.destroy();
    c.isDirty = false;

    if (c.vertices.empty())
    {
        glCreateBuffers(1, &c.vbo);
        glCreateBuffers(1, &c.ebo);

        enum
        {
            POSITION_BIND_INDEX = 0,
            NORMAL_BIND_INDEX = 1
        };

        glCreateVertexArrays(1, &c.vao);

        glEnableVertexArrayAttrib(c.vao, POSITION_BIND_INDEX);
        glVertexArrayAttribFormat(c.vao, POSITION_BIND_INDEX, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(c.vao, POSITION_BIND_INDEX, 0);

        glEnableVertexArrayAttrib(c.vao, NORMAL_BIND_INDEX);
        glVertexArrayAttribFormat(c.vao, NORMAL_BIND_INDEX, 3, GL_FLOAT, GL_FALSE, 12);
        glVertexArrayAttribBinding(c.vao, NORMAL_BIND_INDEX, 0);
    }

    f32 totalLength = c.getLength();

    c.boundingBox = { Vec3(FLT_MAX), Vec3(-FLT_MAX) };
    c.vertices.clear();
    c.indices.clear();
    // TODO: find the maximum viable step size
    const f32 stepSize = 2.f;
    u32 totalSteps = (u32)(totalLength / stepSize);
    Vec3 prevP = points[c.pointIndexA].position;
    for (u32 i=0; i<=totalSteps; ++i)
    {
        f32 t = (f32)i / (f32)totalSteps;
        Vec3 p = pointOnBezierCurve(
                points[c.pointIndexA].position,
                points[c.pointIndexA].position + c.handleOffsetA,
                points[c.pointIndexB].position + c.handleOffsetB,
                points[c.pointIndexB].position, t);
        Vec3 xDir = normalize(i == 0 ? c.handleOffsetA :
                (i == totalSteps ? -c.handleOffsetB : normalize(p - prevP)));
        Vec3 yDir = normalize(cross(xDir, Vec3(0, 0, 1)));
        Vec3 zDir = normalize(cross(yDir, xDir));
        f32 width = lerp(c.widthA, c.widthB, t);
        Vec3 p1 = p + yDir * width;
        Vec3 p2 = p - yDir * width;
        c.vertices.push_back(Vertex{ p1, zDir });
        c.vertices.push_back(Vertex{ p2, zDir });
        if (i > 0)
        {
            c.indices.push_back(i * 2 - 1);
            c.indices.push_back(i * 2 - 2);
            c.indices.push_back(i * 2);
            c.indices.push_back(i * 2);
            c.indices.push_back(i * 2 + 1);
            c.indices.push_back(i * 2 - 1);
        }
        c.boundingBox.min = min(c.boundingBox.min, min(p1, p2));
        c.boundingBox.max = max(c.boundingBox.max, max(p1, p2));
        prevP = p;
    }
    computeBoundingBox();

    glBindVertexArray(c.vao);
    glNamedBufferData(c.vbo, c.vertices.size() * sizeof(Vertex), c.vertices.data(), GL_DYNAMIC_DRAW);
    glNamedBufferData(c.ebo, c.indices.size() * sizeof(u32), c.indices.data(), GL_DYNAMIC_DRAW);
    glVertexArrayVertexBuffer(c.vao, 0, c.vbo, 0, sizeof(Vertex));
    glVertexArrayElementBuffer(c.vao, c.ebo);

    // collision mesh
    PxTriangleMeshDesc desc;
    desc.points.count = (u32)c.vertices.size();
    desc.points.stride = sizeof(Vertex);
    desc.points.data = c.vertices.data();
    desc.triangles.count = (u32)c.indices.size() / 3;
    desc.triangles.stride = 3 * sizeof(c.indices[0]);
    desc.triangles.data = c.indices.data();

    PxDefaultMemoryOutputStream writeBuffer;
    if (!g_game.physx.cooking->cookTriangleMesh(desc, writeBuffer))
    {
        FATAL_ERROR("Failed to create collision mesh for track segment");
    }

    PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
    PxTriangleMesh* triMesh = g_game.physx.physics->createTriangleMesh(readBuffer);

    if (!c.collisionShape)
    {
        c.collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
                PxTriangleMeshGeometry(triMesh), *g_game.physx.materials.track);
        c.collisionShape->setQueryFilterData(PxFilterData(
                    COLLISION_FLAG_TRACK, DECAL_TRACK, 0, DRIVABLE_SURFACE));
        c.collisionShape->setSimulationFilterData(PxFilterData(
                    COLLISION_FLAG_TRACK, -1, 0, 0));
    }
    else
    {
        c.collisionShape->setGeometry(PxTriangleMeshGeometry(triMesh));
    }
    triMesh->release();
}

void Track::buildTrackGraph(TrackGraph* trackGraph, Mat4 const& startTransform)
{
    trackGraph->clear();
    for (Point& p : points)
    {
        p.trackGraphNodeIndex = trackGraph->addNode(p.position);
    }
    u32 nodeIndex = (u32)points.size();
    for (auto& c : connections)
    {
        c->trackGraphNodeIndexA = points[c->pointIndexA].trackGraphNodeIndex;
        c->trackGraphNodeIndexB = points[c->pointIndexB].trackGraphNodeIndex;
        f32 totalLength = c->getLength();
        f32 stepSize = 20.f;
        u32 totalSteps = max(2u, (u32)(totalLength / stepSize));
        u32 startNodeIndex = nodeIndex;
        // TODO: distribute points more evenly
        for (u32 i=1; i<totalSteps; ++i)
        {
            Vec3 p = c->pointOnCurve(i / (f32)totalSteps);
            trackGraph->addNode(p);
            if (i > 1)
            {
                trackGraph->addConnection(nodeIndex - 1, nodeIndex);
            }
            ++nodeIndex;
        }
        trackGraph->addConnection(c->pointIndexA, startNodeIndex);
        trackGraph->addConnection(nodeIndex - 1, c->pointIndexB);
    }
    trackGraph->rebuild(startTransform);
}

void Track::computeBoundingBox()
{
    boundingBox = { Vec3(FLT_MAX), Vec3(-FLT_MAX) };
    for (auto& c : connections)
    {
        boundingBox = boundingBox.growToFit(c->boundingBox);
    }
    f32 minSize = 100.f;
    Vec2 addSize = max(Vec2(0.f),
        Vec2(minSize) - (Vec2(boundingBox.max) - Vec2(boundingBox.min)));
    boundingBox.min -= Vec3(addSize * 0.5f, 0.f);
    boundingBox.max += Vec3(addSize * 0.5f, 0.f);
}

void Track::buildPreviewMesh(Scene* scene)
{
    previewMesh.destroy();

    u32 vertexCount = 0;
    u32 indexCount = 0;
    for (auto& c : connections)
    {
        vertexCount += c->vertices.size();
        indexCount += c->indices.size();
    }
    const u32 vertexElementCount = 7;
    previewMesh.vertices.resize(vertexCount * vertexElementCount + 2);
    previewMesh.indices.resize(indexCount);

#if 0
    u32 vertexIndex = 0;
    u32 indexOffset = 0;
    u32 indexIndex = 0;
    const f32 stepSize = 4.f;
    TrackGraph& trackGraph = scene->getTrackGraph();
    Vec3 finishLinePosition = translationOf(scene->getStart());
    for (auto& c : connections)
    {
        f32 totalLength = c->getLength();
        u32 totalSteps = (u32)(totalLength / stepSize);
        Vec3 prevP = points[c->pointIndexA].position;
        f32 dA = trackGraph.getNode(c->trackGraphNodeIndexA)->t;
        f32 dB = trackGraph.getNode(c->trackGraphNodeIndexB)->t;

        u32 finishLineStepIndex = 0;
        bool crossesFinishLine = false;
        for (u32 i=0; i<=totalSteps; ++i)
        {
            f32 t = (f32)i / (f32)totalSteps;
            Vec3 p = pointOnBezierCurve(
                    points[c->pointIndexA].position,
                    points[c->pointIndexA].position + c->handleOffsetA,
                    points[c->pointIndexB].position + c->handleOffsetB,
                    points[c->pointIndexB].position, t);
            if (distance2(p, finishLinePosition) < square(6.f))
            {
                crossesFinishLine = true;
                finishLineStepIndex = i;
                break;
            }
        }

        f32 ddA = dA;
        f32 ddB = dB;
        if (crossesFinishLine)
        {
            ddB = dA < dB ? 0.f : trackGraph.getStartNode()->t;
        }

        for (u32 i=0; i<=totalSteps; ++i)
        {
            f32 t = (f32)i / (f32)totalSteps;
            Vec3 p = pointOnBezierCurve(
                    points[c->pointIndexA].position,
                    points[c->pointIndexA].position + c->handleOffsetA,
                    points[c->pointIndexB].position + c->handleOffsetB,
                    points[c->pointIndexB].position, t);
            Vec3 xDir = normalize(i == 0 ? c->handleOffsetA :
                    (i == totalSteps ? -c->handleOffsetB : normalize(p - prevP)));
            Vec3 yDir = normalize(cross(xDir, Vec3(0, 0, 1)));
            Vec3 zDir = normalize(cross(yDir, xDir));
            f32 width = lerp(c->widthA, c->widthB, t);
            Vec3 p1 = p + yDir * width;
            Vec3 p2 = p - yDir * width;

            f32 distance = 0.f;
            f32 lerpT = t;
            if (crossesFinishLine)
            {
                if (i <= finishLineStepIndex)
                {
                    lerpT = (f32)i / (f32)(finishLineStepIndex);
                }
                else
                {
                    lerpT = (f32)(i - finishLineStepIndex) / (f32)(totalSteps - finishLineStepIndex);
                }
                distance = lerp(ddA, ddB, lerpT);
            }
            else
            {
                distance = trackGraph.findTrackProgressAtPoint(p, lerp(ddA, ddB, lerpT));
            }

            previewMesh.vertices[vertexIndex + 0]  = p1.x;
            previewMesh.vertices[vertexIndex + 1]  = p1.y;
            previewMesh.vertices[vertexIndex + 2]  = p1.z;
            previewMesh.vertices[vertexIndex + 3]  = zDir.x;
            previewMesh.vertices[vertexIndex + 4]  = zDir.y;
            previewMesh.vertices[vertexIndex + 5]  = zDir.z;
            previewMesh.vertices[vertexIndex + 6]  = distance / trackGraph.getStartNode()->t;
            previewMesh.vertices[vertexIndex + 7]  = p2.x;
            previewMesh.vertices[vertexIndex + 8]  = p2.y;
            previewMesh.vertices[vertexIndex + 9]  = p2.z;
            previewMesh.vertices[vertexIndex + 10] = zDir.x;
            previewMesh.vertices[vertexIndex + 11] = zDir.y;
            previewMesh.vertices[vertexIndex + 12] = zDir.z;
            previewMesh.vertices[vertexIndex + 13] = distance / trackGraph.getStartNode()->t;;
            vertexIndex += 2 * vertexElementCount;

            if (i > 0)
            {
                previewMesh.indices[indexIndex + 0] = indexOffset - 1;
                previewMesh.indices[indexIndex + 1] = indexOffset - 2;
                previewMesh.indices[indexIndex + 2] = indexOffset;
                previewMesh.indices[indexIndex + 3] = indexOffset;
                previewMesh.indices[indexIndex + 4] = indexOffset + 1;
                previewMesh.indices[indexIndex + 5] = indexOffset - 1;

                indexIndex += 6;
            }

            if (crossesFinishLine && i == finishLineStepIndex)
            {
                previewMesh.vertices[vertexIndex + 0]  = p1.x;
                previewMesh.vertices[vertexIndex + 1]  = p1.y;
                previewMesh.vertices[vertexIndex + 2]  = p1.z;
                previewMesh.vertices[vertexIndex + 3]  = zDir.x;
                previewMesh.vertices[vertexIndex + 4]  = zDir.y;
                previewMesh.vertices[vertexIndex + 5]  = zDir.z;
                previewMesh.vertices[vertexIndex + 6]  = ddA;
                previewMesh.vertices[vertexIndex + 7]  = p2.x;
                previewMesh.vertices[vertexIndex + 8]  = p2.y;
                previewMesh.vertices[vertexIndex + 9]  = p2.z;
                previewMesh.vertices[vertexIndex + 10] = zDir.x;
                previewMesh.vertices[vertexIndex + 11] = zDir.y;
                previewMesh.vertices[vertexIndex + 12] = zDir.z;
                previewMesh.vertices[vertexIndex + 13] = ddA;
                vertexIndex += 2 * vertexElementCount;
                indexOffset += 2;

                ddA = dA < dB ? trackGraph.getStartNode()->t : 0.f;
                ddB = dB;
            }

            indexOffset += 2;
            prevP = p;
        }
    }

#else
    u32 vertexIndex = 0;
    u32 indexOffset = 0;
    u32 indexIndex = 0;
    for (auto& c : connections)
    {
        for (u32 i=0; i<c->vertices.size(); ++i)
        {
            previewMesh.vertices[vertexIndex + 0] = c->vertices[i].position.x;
            previewMesh.vertices[vertexIndex + 1] = c->vertices[i].position.y;
            previewMesh.vertices[vertexIndex + 2] = c->vertices[i].position.z;
            // TODO: don't include normals if not used
            previewMesh.vertices[vertexIndex + 3] = c->vertices[i].normal.x;
            previewMesh.vertices[vertexIndex + 4] = c->vertices[i].normal.y;
            previewMesh.vertices[vertexIndex + 5] = c->vertices[i].normal.z;
            //previewMesh.vertices[vertexIndex + 6] = 0.f;

            vertexIndex += vertexElementCount;
        }
        for (u32 i=0; i<c->indices.size(); ++i)
        {
            previewMesh.indices[indexIndex + i] = c->indices[i] + indexOffset;
        }
        indexIndex += (u32)c->indices.size();
        indexOffset += (u32)c->vertices.size();
    }
#endif
    previewMesh.name = "Track Preview";
    previewMesh.numVertices = previewMesh.vertices.size() / vertexElementCount;
    previewMesh.numIndices = previewMesh.indices.size();
    previewMesh.numColors = 0;
    previewMesh.numTexCoords = 0;
    previewMesh.stride = vertexElementCount * sizeof(f32);
    previewMesh.aabb = boundingBox;
    previewMesh.vertexFormat = {
        { 0, VertexAttributeType::FLOAT3 }, // position
        { 1, VertexAttributeType::FLOAT3 }, // normal
        //{ 2, VertexAttributeType::FLOAT1 }, // distance
    };

    previewMesh.createVAO();
}

void Track::serializeState(Serializer& s)
{
    s.field(points);
    s.field(connections);

    if (s.deserialize)
    {
        for (auto& c : connections)
        {
            c->track = this;
        }
    }
}
