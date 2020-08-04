#include <glad/glad.h>
#include <iostream>

#include <GLFW/glfw3.h>

#include <camera.hpp>
#include <earcut.hpp>
#include <shader.hpp>
#include <string>

#include "point.hpp"

namespace mapbox {
namespace util {
template <> struct nth<0, Point> {
  static int64_t get(const Point &t) { return t.x; };
};

template <> struct nth<1, Point> {
  static int64_t get(const Point &t) { return t.y; };
};

} // namespace util
} // namespace mapbox

typedef std::vector<Point> Polygon;

static const char *vertex_shader_text = R"SHADER(
#version 410

layout (location = 0) in vec2 position;

uniform mat4 matrix;
uniform vec4 u_color;

out vec4 vs_color;

void main()
{
    gl_Position = matrix * vec4(position, 0.0, 1.0);
    vs_color = u_color;
}

)SHADER";

static const char *frag_shader_text = R"SHADER(
#version 410 core

layout (location = 0) out vec4 color;

in vec4 vs_color;

void main(void)
{
    color = vs_color;
}

)SHADER";

float width = 800, height = 600;
float aspect = width / height;

Polygon currPoly;

int mouse_x, mouse_y;
bool polyComplete = false;

vector<Polygon> polys;
vector<Point> steinerPoints, reflexVertices;

bool isReflex(const Polygon &p, const int &i);
void makeCCW(Polygon &poly);
void initGraphics();
void decomposePoly(Polygon poly);

std::vector<glm::vec4> colors = {
    glm::vec4(1.0f, 0.0, 0.0, 1.0), glm::vec4(0.0f, 1.0, 0.0, 1.0),
    glm::vec4(0.0f, 0.0, 1.0, 1.0), glm::vec4(1.0f, 1.0, 0.0, 1.0),
    glm::vec4(1.0f, 0.0, 1.0, 1.0), glm::vec4(0.0f, 1.0, 1.0, 1.0),
    glm::vec4(1.0f, 0.53, 0.0, 1.0)};

int main() {

  // init
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

  GLFWwindow *window =
      glfwCreateWindow(width, height, "polydecomp", nullptr, nullptr);
  glfwSetWindowSizeCallback(window, [](GLFWwindow *, int width_, int height_) {
    width = width_;
    height = height_;
    aspect = float(width) / float(height);
  });

  glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode,
                                int action, int mods) {
    if (action != GLFW_PRESS)
      return;
    switch (key) {
    case 'C':
      currPoly.clear();
      polys.clear();
      polyComplete = false;
      steinerPoints.clear();
      reflexVertices.clear();
      printf("---\n");
      break;
    }
  });
  glfwSetMouseButtonCallback(
      window, [](GLFWwindow *window, int button, int action, int mods) {
        if (action != GLFW_PRESS)
          return;
        switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
          if (currPoly.size() < 30 ||
              sqdist(Point(mouse_x, mouse_y), currPoly[0]) > 10) {
            if (!polyComplete)
              currPoly.push_back(Point(mouse_x, mouse_y));
            break;
          }
        case GLFW_MOUSE_BUTTON_RIGHT:
          polyComplete = true;
          makeCCW(currPoly);
          decomposePoly(currPoly);
          break;
        }
      });
  glfwSetCursorPosCallback(window, [](GLFWwindow *window, double x, double y) {
    mouse_x = x;
    mouse_y = y;
  });

  glfwMakeContextCurrent(window);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

  // camera
  Camera camera;

  // shader
  // Shader shader("../vs.glsl", "../fs.glsl");
  Shader shader(std::string{vertex_shader_text}, std::string{frag_shader_text});

  // vao vbo
  GLuint vao, vbo;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(0);

  GLuint ibo;
  glGenBuffers(1, &ibo);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_LINE_SMOOTH);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);
    shader.use();
    shader.setMat4("matrix", glm::ortho(0.0f, width, height, 0.0f));

    if (!polyComplete) {
      if (currPoly.size() > 0) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Point) * currPoly.size(),
                     currPoly.data(), GL_DYNAMIC_DRAW);

        glBindVertexArray(vao);
        glPointSize(12);
        glLineWidth(3);
        shader.setVec4("u_color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0));
        glDrawArrays(GL_LINE_STRIP, 0, currPoly.size());
        glDrawArrays(GL_POINTS, 0, currPoly.size());

        std::vector<Point> lastLine;
        lastLine.push_back(currPoly.back());
        lastLine.push_back(Point(mouse_x, mouse_y));
        glBufferData(GL_ARRAY_BUFFER, sizeof(Point) * lastLine.size(),
                     lastLine.data(), GL_DYNAMIC_DRAW);

        glLineWidth(1.5);
        shader.setVec4("u_color", glm::vec4(1.0f, 1.0f, 1.0f, .5f));
        glDrawArrays(GL_LINE_STRIP, 0, lastLine.size());
      }
    } else {
      for (int i = 0; i < polys.size(); ++i) {
        // convex polygon
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Point) * polys[i].size(),
                     polys[i].data(), GL_DYNAMIC_DRAW);

        auto indices = mapbox::earcut<uint16_t>(std::vector<Polygon>{polys[i]});

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * indices.size(),
                     indices.data(), GL_DYNAMIC_DRAW);
        glBindVertexArray(vao);
        shader.setVec4("u_color", colors[i % colors.size()]);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT,
                       nullptr);
        // outline
        glLineWidth(3);
        shader.setVec4("u_color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0));
        glDrawArrays(GL_LINE_STRIP, 0, polys[i].size());
      }
    }

    glfwPollEvents();
    glfwSwapBuffers(window);
  }

  glfwTerminate();
}

void makeCCW(Polygon &poly) {
  int br = 0;

  // find bottom right point
  for (int i = 1; i < poly.size(); ++i) {
    if (poly[i].y < poly[br].y ||
        (poly[i].y == poly[br].y && poly[i].x > poly[br].x)) {
      br = i;
    }
  }

  // reverse poly if clockwise
  if (!left(at(poly, br - 1), at(poly, br), at(poly, br + 1))) {
    reverse(poly.begin(), poly.end());
  }
}

bool isReflex(const Polygon &poly, const int &i) {
  return right(at(poly, i - 1), at(poly, i), at(poly, i + 1));
}

Point intersection(const Point &p1, const Point &p2, const Point &q1,
                   const Point &q2) {
  Point i;
  Scalar a1, b1, c1, a2, b2, c2, det;
  a1 = p2.y - p1.y;
  b1 = p1.x - p2.x;
  c1 = a1 * p1.x + b1 * p1.y;
  a2 = q2.y - q1.y;
  b2 = q1.x - q2.x;
  c2 = a2 * q1.x + b2 * q1.y;
  det = a1 * b2 - a2 * b1;
  if (!eq(det, 0)) { // lines are not parallel
    i.x = (b2 * c1 - b1 * c2) / det;
    i.y = (a1 * c2 - a2 * c1) / det;
  }
  return i;
}

void swap(int &a, int &b) {
  int c;
  c = a;
  a = b;
  b = c;
}

void decomposePoly(Polygon poly) {
  Point upperInt, lowerInt, p, closestVert;
  Scalar upperDist, lowerDist, d, closestDist;
  int upperIndex, lowerIndex, closestIndex;
  Polygon lowerPoly, upperPoly;

  for (int i = 0; i < poly.size(); ++i) {
    if (isReflex(poly, i)) {
      reflexVertices.push_back(poly[i]);
      upperDist = lowerDist = numeric_limits<Scalar>::max();
      for (int j = 0; j < poly.size(); ++j) {
        if (left(at(poly, i - 1), at(poly, i), at(poly, j)) &&
            rightOn(at(poly, i - 1), at(poly, i),
                    at(poly, j - 1))) { // if line intersects with an edge
          p = intersection(at(poly, i - 1), at(poly, i), at(poly, j),
                           at(poly, j - 1)); // find the point of intersection
          if (right(at(poly, i + 1), at(poly, i),
                    p)) { // make sure it's inside the poly
            d = sqdist(poly[i], p);
            if (d < lowerDist) { // keep only the closest intersection
              lowerDist = d;
              lowerInt = p;
              lowerIndex = j;
            }
          }
        }
        if (left(at(poly, i + 1), at(poly, i), at(poly, j + 1)) &&
            rightOn(at(poly, i + 1), at(poly, i), at(poly, j))) {
          p = intersection(at(poly, i + 1), at(poly, i), at(poly, j),
                           at(poly, j + 1));
          if (left(at(poly, i - 1), at(poly, i), p)) {
            d = sqdist(poly[i], p);
            if (d < upperDist) {
              upperDist = d;
              upperInt = p;
              upperIndex = j;
            }
          }
        }
      }

      // if there are no vertices to connect to, choose a point in the middle
      if (lowerIndex == (upperIndex + 1) % poly.size()) {
        printf("Case 1: Vertex(%d), lowerIndex(%d), upperIndex(%d), "
               "poly.size(%d)\n",
               i, lowerIndex, upperIndex, (int)poly.size());
        p.x = (lowerInt.x + upperInt.x) / 2;
        p.y = (lowerInt.y + upperInt.y) / 2;
        steinerPoints.push_back(p);

        if (i < upperIndex) {
          lowerPoly.insert(lowerPoly.end(), poly.begin() + i,
                           poly.begin() + upperIndex + 1);
          lowerPoly.push_back(p);
          upperPoly.push_back(p);
          if (lowerIndex != 0)
            upperPoly.insert(upperPoly.end(), poly.begin() + lowerIndex,
                             poly.end());
          upperPoly.insert(upperPoly.end(), poly.begin(), poly.begin() + i + 1);
        } else {
          if (i != 0)
            lowerPoly.insert(lowerPoly.end(), poly.begin() + i, poly.end());
          lowerPoly.insert(lowerPoly.end(), poly.begin(),
                           poly.begin() + upperIndex + 1);
          lowerPoly.push_back(p);
          upperPoly.push_back(p);
          upperPoly.insert(upperPoly.end(), poly.begin() + lowerIndex,
                           poly.begin() + i + 1);
        }
      } else {
        // connect to the closest point within the triangle
        printf("Case 2: Vertex(%d), closestIndex(%d), poly.size(%d)\n", i,
               closestIndex, (int)poly.size());

        if (lowerIndex > upperIndex) {
          upperIndex += poly.size();
        }
        closestDist = numeric_limits<Scalar>::max();
        for (int j = lowerIndex; j <= upperIndex; ++j) {
          if (leftOn(at(poly, i - 1), at(poly, i), at(poly, j)) &&
              rightOn(at(poly, i + 1), at(poly, i), at(poly, j))) {
            d = sqdist(at(poly, i), at(poly, j));
            if (d < closestDist) {
              closestDist = d;
              closestVert = at(poly, j);
              closestIndex = j % poly.size();
            }
          }
        }

        if (i < closestIndex) {
          lowerPoly.insert(lowerPoly.end(), poly.begin() + i,
                           poly.begin() + closestIndex + 1);
          if (closestIndex != 0)
            upperPoly.insert(upperPoly.end(), poly.begin() + closestIndex,
                             poly.end());
          upperPoly.insert(upperPoly.end(), poly.begin(), poly.begin() + i + 1);
        } else {
          if (i != 0)
            lowerPoly.insert(lowerPoly.end(), poly.begin() + i, poly.end());
          lowerPoly.insert(lowerPoly.end(), poly.begin(),
                           poly.begin() + closestIndex + 1);
          upperPoly.insert(upperPoly.end(), poly.begin() + closestIndex,
                           poly.begin() + i + 1);
        }
      }

      // solve smallest poly first
      if (lowerPoly.size() < upperPoly.size()) {
        decomposePoly(lowerPoly);
        decomposePoly(upperPoly);
      } else {
        decomposePoly(upperPoly);
        decomposePoly(lowerPoly);
      }
      return;
    }
  }
  polys.push_back(poly);
}
