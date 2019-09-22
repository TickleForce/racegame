#pragma once

#include "math.h"
#include "renderable.h"
#include "resources.h"

class BillboardRenderable : public Renderable
{
    GLuint tex = 0;
    glm::vec3 position;
    glm::vec4 color;
    f32 scale;
    f32 angle;

public:
    BillboardRenderable(Texture* tex, glm::vec3 const& position, glm::vec4 const& color,
            f32 scale=1.f, f32 angle=0.f)
        : tex(tex->handle), position(position), color(color), scale(scale), angle(angle) {}

    i32 getPriority() const override { return 10001; }

    void onLitPass(Renderer* renderer) override
    {
        glBindTextureUnit(0, tex);
        glUniform4fv(0, 1, (GLfloat*)&color);
        glm::mat4 translation = glm::translate(glm::mat4(1.f), position);
        glm::mat4 rotation = glm::rotate(glm::mat4(1.f), angle, { 0, 0, 1 });
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(translation));
        glUniform3f(2, scale, scale, scale);
        glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(rotation));
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
};