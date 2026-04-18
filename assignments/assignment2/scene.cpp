#include "scene.h"

// imgui
#include "imgui/imgui.h"
#include "imguizmo/imguizmo.h"

// glm
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

// batteries
#include "batteries/materials.h"
#include "batteries/math.h"
#include "batteries/opengl.h"

glm::mat4 lightMatrix = glm::mat4(1.0f);

const glm::vec4 backgroundColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);

struct {
    float alpha = 128.0f;
} debug;

auto matrix = glm::mat4(1.0f);

void Scene::initShadowMap()
{
    glGenFramebuffers(1, &depthMapFBO);

    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // attach depth texture to fbo, doenst need color buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // reset
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Scene::Scene()
{
    suzanne = std::make_unique<ew::Model>("assets/models/suzanne.obj");

    simpleShadow = std::make_unique<ew::Shader>("assets/shaders/shadow.vs", "assets/shaders/shadow.fs");
    simpleShadowDepth = std::make_unique<ew::Shader>("assets/shaders/shadowDepth.vs", "assets/shaders/shadowDepth.fs");

    texture = std::make_unique<ew::Texture>("assets/textures/brick_color.jpg");

    light = {
        .brightness = 1.0f,
        .color = {1.0f, 1.0f, 1.0f},
        .position = {0.0f, 2.0f, 0.0f},
    };

    initShadowMap();
    initPlane();
}

Scene::~Scene()
{
    glDeleteFramebuffers(1, &depthMapFBO);
    glDeleteTextures(1, &depthMap);

    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &planeVBO);
}

void Scene::Update(float dt)
{
    batteries::Scene::Update(dt);
}

void Scene::renderScene(ew::Shader &shader)
{
    // suzanne
    shader.setMat4("model", matrix);
    suzanne->draw();

    // plane
    glm::mat4 planeModel = glm::mat4(1.0f);
    shader.setMat4("model", planeModel);

    glBindVertexArray(planeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Scene::Render(void)
{
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);

    // build light space matrix with orthographic proj
    float near_plane = 0.1f, far_plane = 20.0f;
    glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
    glm::mat4 lightView = glm::lookAt(light.position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    // first pass just renders depth map from light pov
    simpleShadowDepth->use();
    simpleShadowDepth->setMat4("lightSpaceMatrix", lightSpaceMatrix);

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    // shadow render call
    renderScene(*simpleShadowDepth);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // second pass renders scene with shadow
    int display_w, display_h;

    // restoring the viewport, idk how to get screen size
    glViewport(0, 0, 800, 600);

    glClearColor(backgroundColor.x, backgroundColor.y, backgroundColor.z, backgroundColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const auto view_proj = camera.Projection() * camera.View();

    simpleShadow->use();
    simpleShadow->setMat4("view_proj", view_proj);
    simpleShadow->setMat4("lightSpaceMatrix", lightSpaceMatrix);
    simpleShadow->setVec3("camera_position", camera.position);
    simpleShadow->setVec3("light.position", light.position);
    simpleShadow->setVec3("light.color", light.color);
    simpleShadow->setFloat("material.shininess", debug.alpha);
    simpleShadow->setVec3("material.diffuse", glm::vec3(1.0f));
    simpleShadow->setVec3("material.specular", glm::vec3(1.0f));
    simpleShadow->setVec3("material.ambient", glm::vec3(backgroundColor) * 0.1f);

    // texture on unit 0, shadow map on unit 1
    simpleShadow->setInt("texture0", 0);
    simpleShadow->setInt("shadowMap", 1);
    glBindTextureUnit(0, texture->getID());
    glBindTextureUnit(1, depthMap);

    // scene render call
    renderScene(*simpleShadow);
}

void Scene::initPlane()
{
    float planeVertices[] = 
    {
    // positions          // normals         // texcoords
     5.0f, -1.0f,  5.0f,  0.0f, 1.0f, 0.0f,  5.0f, 0.0f,
    -5.0f, -1.0f, -5.0f,  0.0f, 1.0f, 0.0f,  0.0f, 5.0f,
    -5.0f, -1.0f,  5.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,

     5.0f, -1.0f,  5.0f,  0.0f, 1.0f, 0.0f,  5.0f, 0.0f,
     5.0f, -1.0f, -5.0f,  0.0f, 1.0f, 0.0f,  5.0f, 5.0f,
    -5.0f, -1.0f, -5.0f,  0.0f, 1.0f, 0.0f,  0.0f, 5.0f,
    };

    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);
}

void Scene::Debug(void)
{
    ImGuizmo::BeginFrame();
    ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
    ImGuizmo::SetRect(0, 0, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);

    glm::mat4 m{1.0f};
    auto *view = glm::value_ptr(camera.View());
    auto *proj = glm::value_ptr(camera.Projection());

    if (ImGuizmo::IsUsing()) {
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

    ImGui::Checkbox("Paused", &time.paused);
    ImGui::SliderFloat("Time Factor", &time.factor, 0.0f, 10.0f);
    ImGui::ColorEdit3("Light Color", &light.color.x);
    ImGui::SliderFloat("Alpha", &debug.alpha, 0, 128);

    ImGui::End();
}