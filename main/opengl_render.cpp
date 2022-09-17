#ifndef OPENGL_RENDER_CPP
#define OPENGL_RENDER_CPP

#include "render.h"
#include "math.cpp"
#include <gl\gl.h>

static void OpenGLRectangle(v2 minP, v2 maxP, v4 color)
{
    glBegin(GL_TRIANGLES);

    glColor4fv(color.e);

    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(minP.x, minP.y);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(maxP.x, minP.y);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(maxP.x, maxP.y);

    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(minP.x, minP.y);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(maxP.x, maxP.y);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(minP.x, maxP.y);
    glEnd();
}

mat4 OpenGLGetProjectionMatrix(camera* cam)
{
    float a = cam->size.x / cam->size.y;
    float s = 1 / Tan(0.5f * cam->rFov);
    float deltaZ = cam->zFar - cam->zNear;

    mat4 result = {};
    result.r0 = V4(s, 0, 0, 0);
    result.r1 = V4(0, s * a, 0, 0);
    result.r2 = V4(0, 0, cam->zFar / deltaZ, 1);
    result.r3 = V4(0, 0, -(cam->zFar * cam->zNear) / deltaZ, 0);

    return result;
}

mat4 OpenGLGetTranslateMatrix(v3 pos)
{
    mat4 result = {};
    result.r0 = V4(1, 0, 0, 0);
    result.r1 = V4(0, 1, 0, 0);
    result.r2 = V4(0, 0, 1, 0);
    result.r3 = V4(-pos.x, -pos.y, -pos.z, 1);

    return result;
}

mat4 OpenGLGetLookAtMatrix(v3 eye, v3 target, v3 up = V3(0, 1, 0))
{
    v3 camZ = Normalize(target - eye);
    v3 camX = Normalize(Cross(up, camZ));
    v3 camY = Normalize(Cross(camZ, camX));

    // char buf[256];
    // _snprintf_s(buf, sizeof(buf), "up=%f, %f, %f", up.x, up.y, up.z);
    // OutputDebugStringA(buf);

    mat4 result = {};
    result.r0 = V4(camX.x, camY.x, camZ.x, 0);
    result.r1 = V4(camX.y, camY.y, camZ.y, 0);
    result.r2 = V4(camX.z, camY.z, camZ.z, 0);
    result.r3 = V4(-Dot(camX, eye), -Dot(camY, eye), -Dot(camZ, eye), 1);

    return result;
}

static void OpenGLRenderGroupToOutput(render_group* renderGroup, loaded_bitmap* drawBuffer)
{
    glViewport(0, 0, drawBuffer->width, drawBuffer->height);
    camera* cam = renderGroup->cam;

    glEnable(GL_TEXTURE_2D);
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    float noProj[] =
    {
        2.0f / drawBuffer->width, 0, 0, 0,
        0, 2.0f / drawBuffer->height, 0, 0,
        0, 0, 1, 0,
        -1, -1, 0, 1,
    };
    mat4 proj = OpenGLGetProjectionMatrix(cam);
    mat4 trans = OpenGLGetTranslateMatrix(cam->pos);
    mat4 view = OpenGLGetLookAtMatrix(cam->pos, cam->pos + cam->dir, V3(0, 1, 0));
    mat4 modelView = trans * view;

    bool isProj = true;

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf((float*)modelView.e);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf((float*)proj.e);

    for (int index = 0; index < renderGroup->pushBuffer.used;)
    {
        render_entry_header* header = (render_entry_header*)((u8*)renderGroup->pushBuffer.base + index);
        index += sizeof(*header);

        switch (header->type)
        {
            case RENDER_TYPE_render_entry_rectangle:
            case RENDER_TYPE_render_entry_rectangle_outline:
            case RENDER_TYPE_render_entry_bitmap:
            {
                if (isProj == true)
                {
                    glMatrixMode(GL_MODELVIEW);
                    glLoadIdentity();

                    glMatrixMode(GL_PROJECTION);
                    glLoadMatrixf(noProj);
                    isProj = false;
                }
            } break;

            case RENDER_TYPE_render_entry_model:
            {
                if (!isProj)
                {
                    glMatrixMode(GL_MODELVIEW);
                    glLoadMatrixf((float*)modelView.e);

                    glMatrixMode(GL_PROJECTION);
                    glLoadMatrixf((float*)proj.e);
                    isProj = true;
                }
            } break;
            case RENDER_TYPE_render_entry_saturation:
            case RENDER_TYPE_render_entry_clear:
            {
                //NOTE: skip!
            }
            break;
            INVALID_DEFAULT_CASE;
        }

        switch (header->type)
        {
            case RENDER_TYPE_render_entry_saturation:
            {
                render_entry_saturation* entry = (render_entry_saturation*)((u8*)renderGroup->pushBuffer.base + index);
                index += sizeof(*entry);

                // ChangeSaturation(drawBuffer, entry->level);
            } break;

            case RENDER_TYPE_render_entry_rectangle:
            {
                render_entry_rectangle* entry = (render_entry_rectangle*)((u8*)renderGroup->pushBuffer.base + index);
                index += sizeof(*entry);
                glDisable(GL_TEXTURE_2D);

                OpenGLRectangle(entry->minP, entry->maxP, entry->color);
                glEnable(GL_TEXTURE_2D);
            } break;

            case RENDER_TYPE_render_entry_rectangle_outline:
            {
                render_entry_rectangle_outline* entry = (render_entry_rectangle_outline*)((u8*)renderGroup->pushBuffer.base + index);
                index += sizeof(*entry);

                // DrawRectangleOutline(drawBuffer, entry->minP, entry->maxP, entry->thickness, entry->color);
            } break;

            case RENDER_TYPE_render_entry_bitmap:
            {
                render_entry_bitmap* entry = (render_entry_bitmap*)((u8*)renderGroup->pushBuffer.base + index);
                index += sizeof(*entry);


                loaded_bitmap* texture = entry->bitmap;

                glBindTexture(GL_TEXTURE_2D, 2);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture->width, texture->height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, texture->pixel);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

                OpenGLRectangle(entry->minP, entry->maxP, entry->color);

            } break;

            case RENDER_TYPE_render_entry_clear:
            {
                render_entry_clear* entry = (render_entry_clear*)((u8*)renderGroup->pushBuffer.base + index);
                index += sizeof(*entry);
                glClearColor(entry->color.r, entry->color.g, entry->color.b, entry->color.a);
                glClear(GL_COLOR_BUFFER_BIT);
            } break;

            case RENDER_TYPE_render_entry_model:
            {
                render_entry_model* entry = (render_entry_model*)((u8*)renderGroup->pushBuffer.base + index);
                index += sizeof(*entry);

                // Draw(queue, arena, drawBuffer, entry->light, renderGroup->cam, entry->model, entry->col);
            } break;

            INVALID_DEFAULT_CASE;
        }
    }
}

#endif