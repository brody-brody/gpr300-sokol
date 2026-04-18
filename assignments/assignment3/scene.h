#pragma once

// batteries
#include "batteries/scene.h"
#include "batteries/lights.h"

// ew
#include "ew/model.h"
#include "ew/mesh.h"
#include "ew/shader.h"
#include "ew/texture.h"
#include "ew/procGen.h"

// glm
#include "glm/glm.hpp"

struct PointLight
{
    glm::vec3 position;
    float radius;
    glm::vec4 color;
};

struct GBuffer
{
    unsigned int fbo;
    // 0 is pos, 1 is normal, 2 is albeod
    unsigned int colorBuffers[3];
    unsigned int depthBuffer;
    unsigned int width;
    unsigned int height;
};

class Scene final : public batteries::Scene
{
  public:
    Scene();
    virtual ~Scene();

    void Update(float dt);
    void Render(void);
    void Debug(void);

  private:
    void renderScene(ew::Shader &shader);
    void initGBuffer();
    void initPlane();

    std::unique_ptr<ew::Model> suzanne;

    // geometry pass writes surface attribs to gbuffer
    std::unique_ptr<ew::Shader> geometryPassShader;

    // lighting pass make fs triangle sample g buffer
    std::unique_ptr<ew::Shader> deferredLitShader;

    // forward pass for the light orbs drawn on top
    std::unique_ptr<ew::Shader> lightOrbShader;

    std::unique_ptr<ew::Texture> texture;

    batteries::ambient_t ambient;

    // grid of suzannes
    static const int GRID_SIZE = 8;
    static const int GRID_SPACING = 3;
    glm::mat4 suzanneMatrices[GRID_SIZE * GRID_SIZE];

    // point lights
    static const int MAX_POINT_LIGHTS = 64;
    PointLight pointLights[MAX_POINT_LIGHTS];

    // sphere mesh for light orbs
    ew::Mesh sphereMesh;

    // plane geometry
    unsigned int planeVAO;
    unsigned int planeVBO;

    // vao for the fullscreen triangle draw call
    unsigned int dummyVAO;

    GBuffer gBuffer;
    static const unsigned int SCREEN_WIDTH = 800, SCREEN_HEIGHT = 600;
};