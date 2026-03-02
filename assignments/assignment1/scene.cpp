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

#include <vector>

glm::mat4 lightMatrix = glm::mat4(1.0f);
glm::vec3 lightColor = glm::vec3(1.0f);

const glm::vec4 backgroundColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);

std::vector<std::unique_ptr<ew::Shader>> postEffects;

const char* postNames[] = {
    "None",
    "Grayscale",
    "Chromatic Aberration",
    "Scanlines",
    "Edge Detection",
    "Pixelate",
    "Vignette",
    "Invert",
    "Contrast"
};

struct{
    float alpha = 128.0f;
    float strength = 16.0f;
    float resolution = 1.0f;
    int postIndex = 0;
} debug;

struct FullScreenQuad
{
    // vertex attribute object
    GLuint vao;
    // vertex buffer object
    GLuint vbo;

    int stride = 4;
    int verticesNumber = 6;

    bool Initialize()
    {
        float vertices[] = {
            // pos (x, y), texcoord (u, v)
            // triangle 1
            -1,  1, 0, 1,
            -1, -1, 0, 0,
             1, -1, 1, 0,

            // triangle 2
            -1,  1, 0, 1,
             1, -1, 1, 0,
             1,  1, 1, 1,
        };

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        // bind VAO
        glBindVertexArray(vao);

        // VBO is bound to VAO state because VAO is bound first
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        // vec2 position (x, y)
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        // vec2 tex coords (u, v), offset by position size
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(sizeof(float) * 2));

        // unbind VAO
        glBindVertexArray(0);
        return true;
    }
} fullscreenQuad;

Scene::Scene()
{
    suzanne = std::make_unique<ew::Model>("assets/models/suzanne.obj");

    // blinn phong
    blinnphong = std::make_unique<ew::Shader>("assets/shaders/blinnphong.vs", "assets/shaders/blinnphong.fs");

    // post processing effects
    postEffects.push_back(std::make_unique<ew::Shader>("assets/shaders/fullscreen.vs", "assets/shaders/fullscreen.fs"));
    postEffects.push_back(std::make_unique<ew::Shader>("assets/shaders/fullscreen.vs", "assets/postprocess/grayscale.fs"));
    postEffects.push_back(std::make_unique<ew::Shader>("assets/shaders/fullscreen.vs", "assets/postprocess/chromaticaberration.fs"));
    postEffects.push_back(std::make_unique<ew::Shader>("assets/shaders/fullscreen.vs", "assets/postprocess/scanlines.fs"));
    postEffects.push_back(std::make_unique<ew::Shader>("assets/shaders/fullscreen.vs", "assets/postprocess/edgedetection.fs"));
    postEffects.push_back(std::make_unique<ew::Shader>("assets/shaders/fullscreen.vs", "assets/postprocess/pixelate.fs"));
    postEffects.push_back(std::make_unique<ew::Shader>("assets/shaders/fullscreen.vs", "assets/postprocess/vignette.fs"));
    postEffects.push_back(std::make_unique<ew::Shader>("assets/shaders/fullscreen.vs", "assets/postprocess/invert.fs"));
    postEffects.push_back(std::make_unique<ew::Shader>("assets/shaders/fullscreen.vs", "assets/postprocess/contrast.fs"));

    texture = std::make_unique<ew::Texture>("assets/textures/brick_color.jpg");

    // defintiiion specific, keep variables in order
    light = {
        .brightness = 1.0f,
        .color = {1.0f, 1.0f, 1.0f},
        .position = {0.0f, 2.0f, 0.0f},
    };

    lightColor = light.color;

    palette = {
        .color1 = {1.0f, 0.0f, 1.0f},
        .color2 = {0.0f, 0.0f, 1.0f}
    };

    fullscreenQuad.Initialize();

    // allocate frame buffer
    glCreateFramebuffers(1, &framebuffer);

    // create image while frame buffer is bound
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    {
        glGenTextures(1, &fboTexture);
        glBindTexture(GL_TEXTURE_2D, fboTexture);

        // create 800x600 render texture with 8 unsigned bytes
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 800, 600, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTexture, 0);

        // create depth texture
        glGenTextures(1, &fboDepth);
        glBindTexture(GL_TEXTURE_2D, fboDepth);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, 800, 600, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, fboDepth, 0);

        // cleanup textures
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Scene::~Scene()
{
    glDeleteFramebuffers(1, &framebuffer);
}

void Scene::Update(float dt)
{
    batteries::Scene::Update(dt);

    /* body */
}

auto matrix = glm::mat4(1.0f);

void Scene::PostProcess(ew::Shader* shader)
{
    shader->use();
    shader->setInt("screen", 0);

    switch (debug.postIndex){
        case grayscale:
        break;

        case chromaticAberration:
            shader->setFloat("strength", debug.strength / 1000);
        break;

        case scanlines:
            shader->setFloat("strength", debug.strength / 1000);
            shader->setFloat("resolution", debug.resolution);
        break;

        case edgeDetection:
            shader->setFloat("strength", debug.strength / 100);
        break;

        case pixelate:
            shader->setFloat("strength", debug.strength / 10000);
        break;

        case vignette:
            shader->setFloat("strength", debug.strength / 100);
            shader->setFloat("resolution", debug.resolution / 1000);
        break;

        case invert:
        break;

        case contrast:
            shader->setFloat("strength", debug.strength / 100);
        break;
    }

    // disable depth test for fullscreen quad
    glDisable(GL_DEPTH_TEST);

    // clear default framebuffer
    glClearColor(backgroundColor.x, backgroundColor.y, backgroundColor.z, backgroundColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw fullscreen quad
    glBindVertexArray(fullscreenQuad.vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fboTexture);

    glDrawArrays(GL_TRIANGLES, 0, fullscreenQuad.verticesNumber);
}

void Scene::Render(void)
{
    const auto view_proj = camera.Projection() * camera.View();

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // re-enable depth test
    glEnable(GL_DEPTH_TEST);
    // glDisable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    {
        glClearColor(backgroundColor.x, backgroundColor.y, backgroundColor.z, backgroundColor.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // bind texture to channel
        glBindTextureUnit(0, texture->getID());

        blinnphong->use();

        // sampler for texture
        blinnphong->setInt("texture0", 0);

        // scene matrices
        blinnphong->setMat4("model", matrix);
        blinnphong->setMat4("view_proj", view_proj);
        blinnphong->setVec3("camera_position", camera.position);

        blinnphong->setVec3("light.position", light.position);
        blinnphong->setVec3("light.color", light.color);
        blinnphong->setFloat("material.shininess", debug.alpha);

        blinnphong->setVec3("material.diffuse", glm::vec3(1));
        blinnphong->setVec3("material.specular", glm::vec3(1));
        blinnphong->setVec3("material.ambient", backgroundColor * 0.1f);

        // draw suzanne
        suzanne->draw();
    }

    // clear framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // render fullscreen quad
    PostProcess(postEffects[debug.postIndex].get());
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

    light.color = lightColor;

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

    // post processing selector
    ImGui::Combo("Post Processing Effect", &debug.postIndex, postNames, IM_ARRAYSIZE(postNames));

    // alpha settings
    ImGui::SliderFloat("Alpha", &debug.alpha, 0, 128);

    // effect controls
    ImGui::SliderFloat("Effect Strength", &debug.strength, 0, 1000);
    ImGui::SliderFloat("Effect Resolution", &debug.resolution, 0, 1000);

    // color settings
    ImGui::ColorEdit3("Light Color", &lightColor[0]);

    ImGui::SeparatorText("Color Palette");
    ImGui::ColorEdit3("Color 1", &palette.color1[0]);
    ImGui::ColorEdit3("Color 2", &palette.color2[0]);

    ImGui::Image((void*)(intptr_t)fboTexture, ImVec2(400, 300), ImVec2(0, 1), ImVec2(1, 0));
    ImGui::Image((void*)(intptr_t)fboDepth, ImVec2(400, 300), ImVec2(0, 1), ImVec2(1, 0));

    // pause time checkbox
    ImGui::Checkbox("Paused", &time.paused);
    ImGui::SliderFloat("Time Factor", &time.factor, 0.0f, 10.0f);

    /* build debug ui here */

    ImGui::End();
}