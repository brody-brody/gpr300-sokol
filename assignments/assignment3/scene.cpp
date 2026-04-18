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

// just for the random point light positions
#include <ctime>

const glm::vec4 backgroundColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);

struct {
    float shininess = 32.0f;
    float brightness = 1.0f;
    float radius = 4.0f;
} debug;

void Scene::initGBuffer()
{
    gBuffer.width  = SCREEN_WIDTH;
    gBuffer.height = SCREEN_HEIGHT;

    glGenFramebuffers(1, &gBuffer.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.fbo);

    // 0 is world pos, 1 is world normal, 2 is albedo
    int formats[3] = {GL_RGB32F, GL_RGB16F, GL_RGB16F};

    for (size_t i = 0; i < 3; i++)
    {
        glGenTextures(1, &gBuffer.colorBuffers[i]);
        glBindTexture(GL_TEXTURE_2D, gBuffer.colorBuffers[i]);
        glTexStorage2D(GL_TEXTURE_2D, 1, formats[i], gBuffer.width, gBuffer.height);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // attach each texture to its own  color attachment slot
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, gBuffer.colorBuffers[i], 0);
    }

    // which attachments to draw to
    const GLenum drawBuffers[3] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
    };
    glDrawBuffers(3, drawBuffers);

    // depth buffer for geometry pass
    glGenRenderbuffers(1, &gBuffer.depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, gBuffer.depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, gBuffer.width, gBuffer.height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gBuffer.depthBuffer);

    // reset tha stufff
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Scene::Scene()
{
    suzanne = std::make_unique<ew::Model>("assets/models/suzanne.obj");

    geometryPassShader = std::make_unique<ew::Shader>("assets/shaders/geometryPass.vs", "assets/shaders/geometryPass.fs");
    deferredLitShader = std::make_unique<ew::Shader>("assets/shaders/deferredLit.vs", "assets/shaders/deferredLit.fs");
    lightOrbShader = std::make_unique<ew::Shader>("assets/shaders/lightOrb.vs", "assets/shaders/lightOrb.fs");

    texture = std::make_unique<ew::Texture>("assets/textures/brick_color.jpg");

    // grid of suzannes around origin
    for (int row = 0; row < GRID_SIZE; row++)
    {
        for (int col = 0; col < GRID_SIZE; col++)
        {
            float x = (col - GRID_SIZE / 2) * GRID_SPACING;
            float z = (row - GRID_SIZE / 2) * GRID_SPACING;

            suzanneMatrices[row * GRID_SIZE + col] = glm::translate(glm::mat4(1.0f), glm::vec3(x, 0.0f, z));
        }
    }

    // randomize 64 point lights with random colors too
    // yeah i had to look up this terribly stupid randomization stuff
    srand((unsigned int)::time(nullptr));

    for (int i = 0; i < MAX_POINT_LIGHTS; i++)
    {
        float x = ((rand() % 200) - 100) / 10.0f;
        float z = ((rand() % 200) - 100) / 10.0f;
        pointLights[i].position = glm::vec3(x, 1.0f, z);
        pointLights[i].radius = 2.0f;
        pointLights[i].color = glm::vec4(
            (rand() % 100) / 100.0f,
            (rand() % 100) / 100.0f,
            (rand() % 100) / 100.0f,
            1.0f
        );
    }

    // orb sphere
    sphereMesh = ew::Mesh(ew::createSphere(1.0f, 8));

    // fs triangle vao
    glGenVertexArrays(1, &dummyVAO);

    initGBuffer();
    initPlane();
}

Scene::~Scene()
{
    glDeleteFramebuffers(1, &gBuffer.fbo);
    glDeleteTextures(3, gBuffer.colorBuffers);
    glDeleteRenderbuffers(1, &gBuffer.depthBuffer);

    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &planeVBO);

    glDeleteVertexArrays(1, &dummyVAO);
}

void Scene::Update(float dt)
{
    batteries::Scene::Update(dt);
}

void Scene::renderScene(ew::Shader &shader)
{
    // suzannes
    for (int i = 0; i < GRID_SIZE * GRID_SIZE; i++)
    {
        shader.setMat4("model", suzanneMatrices[i]);
        suzanne->draw();
    }

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

    const auto view_proj = camera.Projection() * camera.View();

    // geometry pass is rendering opaque geometry into gbuffer textures
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.fbo);
    glViewport(0, 0, gBuffer.width, gBuffer.height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    geometryPassShader->use();
    geometryPassShader->setMat4("view_proj", view_proj);
    geometryPassShader->setInt("texture0", 0);
    glBindTextureUnit(0, texture->getID());

    // geo pass render call
    renderScene(*geometryPassShader);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // lighting pass is fs triangle sampling gbuffer and getting lighting for all lights
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glClearColor(backgroundColor.x, backgroundColor.y, backgroundColor.z, backgroundColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    deferredLitShader->use();
    deferredLitShader->setVec3("camera_position", camera.position);
    deferredLitShader->setFloat("material.shininess", debug.shininess);
    deferredLitShader->setVec3("material.diffuse", glm::vec3(1.0f));
    deferredLitShader->setVec3("material.specular", glm::vec3(0.5f));
    deferredLitShader->setVec3("material.ambient", glm::vec3(backgroundColor) * 0.1f);

    // gbuffers
    deferredLitShader->setInt("gPositions", 0);
    deferredLitShader->setInt("gNormals", 1);
    deferredLitShader->setInt("gAlbedo", 2);
    glBindTextureUnit(0, gBuffer.colorBuffers[0]);
    glBindTextureUnit(1, gBuffer.colorBuffers[1]);
    glBindTextureUnit(2, gBuffer.colorBuffers[2]);

    // all point light unifroms
    for (int i = 0; i < MAX_POINT_LIGHTS; i++)
    {
        std::string prefix = "_PointLights[" + std::to_string(i) + "].";
        deferredLitShader->setVec3(prefix + "position", pointLights[i].position);
        deferredLitShader->setFloat(prefix + "radius", debug.radius);
        deferredLitShader->setVec4(prefix + "color", pointLights[i].color);
        deferredLitShader->setVec4(prefix + "color", pointLights[i].color * debug.brightness);
    }

    glBindVertexArray(dummyVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // light orb pass is blitting gbuffer depth into default framebuffer for dept htesting
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer.fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(
        0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
        0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
        GL_DEPTH_BUFFER_BIT, GL_NEAREST
    );

    // model for lights with the right color
    lightOrbShader->use();
    lightOrbShader->setMat4("view_proj", view_proj);

    for (int i = 0; i < MAX_POINT_LIGHTS; i++)
    {
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, pointLights[i].position);
        m = glm::scale(m, glm::vec3(0.1f));
        lightOrbShader->setMat4("model", m);
        lightOrbShader->setVec3("orb_color", glm::vec3(pointLights[i].color));
        sphereMesh.draw();
    }
}

void Scene::initPlane()
{
    // big enough to hold all the suzannes baby
    float s = (GRID_SIZE / 2.0f) * GRID_SPACING + 1.0f;

    float planeVertices[] =
    {
    // positions    // normals         // texcoords
     s, -1.0f,  s,  0.0f, 1.0f, 0.0f,  s, 0.0f,
    -s, -1.0f, -s,  0.0f, 1.0f, 0.0f,  0.0f, s,
    -s, -1.0f,  s,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,

     s, -1.0f,  s,  0.0f, 1.0f, 0.0f,  s, 0.0f,
     s, -1.0f, -s,  0.0f, 1.0f, 0.0f,  s, s,
    -s, -1.0f, -s,  0.0f, 1.0f, 0.0f,  0.0f, s,
    };

    // one million gl calls god what a great way to do things
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
    cameracontroller.Debug();

    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::SliderFloat("Shininess", &debug.shininess, 1.0f, 128.0f);
    ImGui::SliderFloat("Light Brightness", &debug.brightness, 0.0f, 5.0f);
    ImGui::SliderFloat("Light Radius", &debug.radius, 0.1f, 50.0f);

    ImGui::End();

    // all gbuffer textures
    ImGui::Begin("GBuffers");
    {
        ImVec2 texSize = ImVec2(gBuffer.width / 4, gBuffer.height / 4);
        for (size_t i = 0; i < 3; i++)
        {
            // yeah cause im supposed to know imgui wants uv flipped this makes sense this is awesome
            ImGui::Image((ImTextureID)(intptr_t)gBuffer.colorBuffers[i], texSize, ImVec2(0, 1), ImVec2(1, 0));
        }
    }
    ImGui::End();
}