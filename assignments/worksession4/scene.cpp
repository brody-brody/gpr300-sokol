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

struct {
    float alpha = 128.0f;
} debug;

struct FullScreenQuad
{
    unsigned int vao;
    unsigned int vbo;

    void Initialize()
    {
        float vertices[] = {
            // pos (x, y), texcoord (u, v)
            // triangle one
            -1.0f, 1.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f,
            1.0f, -1.0f, 1.0f, 0.0f,

            // triangle two
            -1.0f, 1.0f, 0.0f, 1.0f,
            1.0f, -1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
        };

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(sizeof(float) * 2));

        // always last
        glBindVertexArray(0);
    }
} fullscreen_quad; // cool syntax to just do FullScreenQuad fullscreen_quad; automatically

Scene::Scene()
{
    suzanne = std::make_unique<ew::Model>("assets/models/suzanne.obj");
    toon = std::make_unique<ew::Shader>("assets/shaders/toon.vs", "assets/shaders/toon.fs");
    mainTexture = std::make_unique<ew::Texture>("assets/textures/mario.png");
    gradientTexture = std::make_unique<ew::Texture>("assets/textures/ZAtoon.png");

    postprocess = std::make_unique<ew::Shader>("assets/shaders/fullscreen.vs", "assets/shaders/fullscreen.fs");

    // defintiiion specific, keep variables in order
    light = {
        .brightness = 1.0f,
        .color = {1.0f, 1.0f, 1.0f},
        .position = {0.0f, 2.0f, 0.0f},
    };

    lightMatrix[3] = glm::vec4(light.position, 1.0f);

    palette = {
    .color1 = {1.0f, 0.0f, 1.0f},
    .color2 = {0.0f, 0.0f, 1.0f},
    };

    fullscreen_quad.Initialize();

    glCreateFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    {
        // create texture
        glGenTextures(1, &fboTexture);
        glBindTexture(GL_TEXTURE_2D, fboTexture);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 800, 600, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTexture, 0);

        // create texture
        glGenTextures(1, &fboDepth);
        glBindTexture(GL_TEXTURE_2D, fboDepth);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, 800, 600, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, fboDepth, 0);

        // clean up
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        printf("naw dog that aint it");
    }

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

void Scene::Render(void)
{

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);
    // glDisable(GL_DEPTH_TEST);

    // bind texture to channel
    glBindTextureUnit(0, mainTexture->getID());
    toon->setInt("mainTex", 0);
    glBindTextureUnit(1, gradientTexture->getID());
    toon->setInt("gradientTex", 1);

    // suzanne pipeline
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    {
        const auto view_proj = camera.Projection() * camera.View();

        glClearColor(backgroundColor.x, backgroundColor.y, backgroundColor.z, backgroundColor.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        toon->use();

        // scene matrices
        toon->setMat4("model", matrix);
        toon->setMat4("view_proj", view_proj);
        toon->setVec3("camera_position", camera.position);

        toon->setVec3("light.position", light.position);
        toon->setVec3("light.color", light.color);
        toon->setFloat("material.shininess", debug.alpha);

        toon->setVec3("pal.color1", palette.color1);
        toon->setVec3("pal.color2", palette.color2);


        toon->setVec3("material.diffuse", glm::vec3(1));
        toon->setVec3("material.specular", glm::vec3(1));
        toon->setVec3("material.ambient", backgroundColor * 0.1f);

        // draw da suzanne
        suzanne->draw();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    { // post process pipeline
        postprocess->use();

        postprocess->setInt("screen", 0); //<------ gl_texture0

        // fullscreen pipeline
        glDisable(GL_DEPTH_TEST);

        // default framebuffer
        glClearColor(backgroundColor.x, backgroundColor.y, backgroundColor.z, backgroundColor.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw quad
        glBindVertexArray(fullscreen_quad.vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fboTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }


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

    // palette settings
    ImGui::ColorEdit3("Color1", &palette.color1[0]);
    ImGui::ColorEdit3("Color2", &palette.color2[0]);

    ImGui::Image(
        (void*)(intptr_t)fboTexture,
        ImVec2(400, 300),
        ImVec2(0, 1), ImVec2(1, 0)
    );

    ImGui::Image(
        (void*)(intptr_t)fboDepth,
        ImVec2(400, 300),
        ImVec2(0, 1), ImVec2(1, 0)
    );

    ImGui::End();
}