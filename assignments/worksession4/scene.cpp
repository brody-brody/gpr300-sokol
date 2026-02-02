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

glm::mat4 lightMatrix = glm::mat4(1.0f);


const glm::vec4 backgroundColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);

struct{
    float alpha = 128.0f;
} debug;

Scene::Scene()
{
    skull = std::make_unique<ew::Model>("assets/models/skull.obj");
    toon = std::make_unique<ew::Shader>("assets/shaders/toon.vs", "assets/shaders/toon.fs");
    texture = std::make_unique<ew::Texture>("assets/brick_color.jpg");

    // defintiiion specific, keep variables in order
    light = {
        .brightness = 1.0f,
        .color = {1.0f, 1.0f, 1.0f},
        .position = {0.0f, 2.0f, 0.0f},
    };
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
    glBindTextureUnit(0, texture->getID());

    toon->use();

    // sampler for texture
    toon->setInt("texture0", 0);

    // scene matrices
    toon->setMat4("model", matrix);
    toon->setMat4("view_proj", view_proj);
    toon->setVec3("camera_position", camera.position);

    toon->setVec3("light.position", light.position);
    toon->setVec3("light.color", light.color);
    toon->setFloat("material.shininess", debug.alpha);


    toon->setVec3("material.diffuse", glm::vec3(1));
    toon->setVec3("material.specular", glm::vec3(1));
    toon->setVec3("material.ambient", backgroundColor * 0.1f);

    // draw suzanne
    suzanne->draw();
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
    ImGui::ColorEdit3("Light Color", &light.color.x);

    // alpha settings
    ImGui::SliderFloat("Alpha", &debug.alpha, 0, 128);

    ImGui::End();
}