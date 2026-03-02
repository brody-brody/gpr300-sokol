#include "scene.h"

// imgui
#include "imgui/imgui.h"
#include "imguizmo/imguizmo.h"

// glm
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

// batteries
#include "batteries/materials.h"
#include "batteries/math.h"
#include "batteries/opengl.h"

// ew

#include "ew/procGen.h"

glm::mat4 lightMatrix = glm::mat4(1.0f);


const glm::vec4 backgroundColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);

struct{
    glm::vec3 waterColor = {0, 0, 1};
} debug;

Scene::Scene()
{
    suzanne = std::make_unique<ew::Model>("assets/models/suzanne.obj");
    water = std::make_unique<ew::Shader>("assets/shaders/water.vs", "assets/shaders/water.fs");
    texture = std::make_unique<ew::Texture>("assets/brick_color.jpg");

    // defintiiion specific, keep variables in order
    light = {
        .brightness = 1.0f,
        .color = {1.0f, 1.0f, 1.0f},
        .position = {0.0f, 2.0f, 0.0f},
    };

    water128 = std::make_unique<ew::Texture>("assets/windwaker/water128.png");
    water64 = std::make_unique<ew::Texture>("assets/windwaker/water64.png");
    water32 = std::make_unique<ew::Texture>("assets/windwaker/water32.png");
    water16 = std::make_unique<ew::Texture>("assets/windwaker/water16.png");
    water8 = std::make_unique<ew::Texture>("assets/windwaker/water8.png");

    plane.load(ew::createPlane(100.0f, 100.0f, 10));
}

Scene::~Scene()
{
}

void Scene::Update(float dt)
{
    batteries::Scene::Update(dt);

    /* body */
}

auto matrix = glm::mat4(1.0f);

void Scene::Render(void)
{
    const auto view_proj = camera.Projection() * camera.View();

    glClearColor(backgroundColor.x, backgroundColor.y, backgroundColor.z, backgroundColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);
    // glDisable(GL_DEPTH_TEST);

    // bind texture to channel
    glBindTextureUnit(0, water128->getID());

    water->use();

    // sampler for texture
    water->setInt("texture0", 0);

    // passing in time for scrolling the texture in water.fs
    water->setFloat("time", time.absolute);
    water->setVec3("water_color", debug.waterColor);

    // scene matricesefd43tfrhbvdgbn yjn67gt
    water->setMat4("model", matrix);
    water->setMat4("view_proj", view_proj);
    water->setVec3("camera_position", camera.position);

    // draw plane
    plane.draw();
}

void Scene::Debug(void)
{
    ImGuizmo::BeginFrame();
    ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
    ImGuizmo::SetRect(0, 0, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);

    glm::mat4 m{1.0f};
    auto *view = glm::value_ptr(camera.View());
    auto *proj = glm::value_ptr(camera.Projection());
    
    //ImGuizmo::DrawGrid(view, proj, glm::value_ptr(m), 100.0f);

    if (ImGuizmo::IsUsing()){
        light.position = glm::vec3(lightMatrix[3]);
    }

    ImGuizmo::Manipulate(
        view,
        proj,
        ImGuizmo::TRANSLATE,
        ImGuizmo::WORLD,
        glm::value_ptr(lightMatrix)
    );

    cameracontroller.Debug();

    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    // pause time checkbox
    ImGui::Checkbox("Paused", &time.paused);
    ImGui::SliderFloat("Time Factor", &time.factor, 0.0f, 10.0f);

    // color settings
    ImGui::ColorEdit3("Water Color", &debug.waterColor.x);

    ImGui::End();
}