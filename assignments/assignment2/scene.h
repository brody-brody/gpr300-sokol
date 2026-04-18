#pragma once

// batteries
#include "batteries/scene.h"
#include "batteries/lights.h"

// ew
#include "ew/model.h"
#include "ew/shader.h"
#include "ew/texture.h"

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
    void initShadowMap();

    std::unique_ptr<ew::Model> suzanne;
    std::unique_ptr<ew::Shader> simpleShadow;
    std::unique_ptr<ew::Shader> simpleShadowDepth;
    std::unique_ptr<ew::Texture> texture;

    batteries::ambient_t ambient;
    batteries::light_t light;

    // to see the shadow better
    unsigned int planeVAO;
    unsigned int planeVBO;
    void initPlane();

    // shadow map stuff to write to
    unsigned int depthMapFBO;
    unsigned int depthMap;
    static const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
};