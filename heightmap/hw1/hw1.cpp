/*
  CSCI 420 Computer Graphics, Computer Science, USC
*/
#include "openGLHeader.h"
#include "glutHeader.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "pipelineProgram.h"
#include "vbo.h"
#include "vao.h"

#include <iostream>
#include <cstring>
#include <memory>
#include <vector>

#if defined(WIN32) || defined(_WIN32)
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#if defined(WIN32) || defined(_WIN32)
  char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
  char shaderBasePath[1024] = "../openGLHelper";
#endif

using namespace std;

int mousePos[2]; // x,y screen coordinates of the current mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
typedef enum { POINT_MODE, LINE_MODE, TRIANGLE_MODE, SMOOTH_MODE, PATTERN_MODE, WIRED_MODE } GEOMETRY_MODE; //darwing modes
CONTROL_STATE controlState = ROTATE;

// Transformations of the terrain.
float terrainRotate[3] = { 0.0f, 0.0f, 0.0f }; 
// terrainRotate[0] gives the rotation around x-axis (in degrees)
// terrainRotate[1] gives the rotation around y-axis (in degrees)
// terrainRotate[2] gives the rotation around z-axis (in degrees)
float terrainTranslate[3] = { 0.0f, 0.0f, 0.0f };
float terrainScale[3] = { 1.0f, 1.0f, 1.0f };

// Width and height of the OpenGL window, in pixels.
int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 Homework 1";


// num of vertices for each mode
int pointNumVertices;
int lineNumVertices;
int triNumVertices;
int smoothNumVertices;

// CSCI 420 helper classes.
OpenGLMatrix matrix;
PipelineProgram pipelineProgram;

// VBO's and VAO's
VBO pointVboVertices;
VBO pointVboColors;
VAO pointVao;

VBO lineVboVertices;
VBO lineVboColors;
VAO lineVao;

VBO triVboVertices;
VBO triVboColors;
VAO triVao;

VBO centerVbo;
VBO leftVbo;
VBO rightVbo;
VBO upVbo;
VBO downVbo;
VAO smoothVao;

// needed global vars
int geometry_mode;
int width;
int height;

// uniform vars
float scale = 1.0f;
float exponent = 1.0f;
int vertexShaderMode = 0;
int topPolygon = 0; // for wired mode: wireframe on top of triangle

// Write a screenshot to the specified filename.
void saveScreenshot(const char * filename)
{
  std::unique_ptr<unsigned char[]> screenshotData = std::make_unique<unsigned char[]>(windowWidth * windowHeight * 3);
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData.get());

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData.get());

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;
}

void idleFunc()
{
  // Do some stuff... 
  // For example, here, you can save the screenshots to disk (to make the animation).

  // Notify GLUT that it should call displayFunc.
  glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
  glViewport(0, 0, w, h);

  // When the window has been resized, we need to re-set our projection matrix.
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.LoadIdentity();
  // You need to be careful about setting the zNear and zFar. 
  // Anything closer than zNear, or further than zFar, will be culled.
  const float zNear = 1.0f;
  const float zFar = 1000.0f;
  const float humanFieldOfView = 35.0f; // change to zoom if needed
  matrix.Perspective(humanFieldOfView, 1.0f * w / h, zNear, zFar);
}

void mouseMotionDragFunc(int x, int y)
{
  // Mouse has moved, and one of the mouse buttons is pressed (dragging).

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

  switch (controlState)
  {
    // translate the terrain
    case TRANSLATE:
      if (leftMouseButton)
      {
        // control x,y translation via the left mouse button
        terrainTranslate[0] += mousePosDelta[0] * 0.01f;
        terrainTranslate[1] -= mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z translation via the middle mouse button
        terrainTranslate[2] += mousePosDelta[1] * 0.01f;
      }
      break;

    // rotate the terrain
    case ROTATE:
      if (leftMouseButton)
      {
        // control x,y rotation via the left mouse button
        terrainRotate[0] += mousePosDelta[1];
        terrainRotate[1] += mousePosDelta[0];
      }
      if (middleMouseButton)
      {
        // control z rotation via the middle mouse button
        terrainRotate[2] += mousePosDelta[1];
      }
      break;

    // scale the terrain
    case SCALE:
      if (leftMouseButton)
      {
        // control x,y scaling via the left mouse button
        terrainScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
        terrainScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z scaling via the middle mouse button
        terrainScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // Mouse has moved.
  // Store the new mouse position.
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // A mouse button has has been pressed or depressed.

  // Keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables.
  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      leftMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_MIDDLE_BUTTON:
      middleMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_RIGHT_BUTTON:
      rightMouseButton = (state == GLUT_DOWN);
    break;
  }

  // Keep track of whether CTRL and SHIFT keys are pressed.
  switch (glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      controlState = TRANSLATE;
    break;

    case GLUT_ACTIVE_SHIFT:
      controlState = SCALE;
    break;

    // If CTRL and SHIFT are not pressed, we are in rotate mode.
    default:
      controlState = ROTATE;
    break;
  }

  // Store the new mouse position.
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 27: // ESC key
        exit(0); // exit the program
        break;

    case ' ':
        cout << "You pressed the spacebar." << endl;
        break;

    case 'x':
        // Take a screenshot.
        saveScreenshot("pic.jpg");
        break;

    case '1':
        vertexShaderMode = 0;
        geometry_mode = POINT_MODE;
        break;

    case '2':
        vertexShaderMode = 0;
        geometry_mode = LINE_MODE;
        break;

    case '3':
        vertexShaderMode = 0;
        geometry_mode = TRIANGLE_MODE;
        break;

    case '4':
        vertexShaderMode = 1;
        geometry_mode = SMOOTH_MODE;
        break;
    case '5':
        vertexShaderMode = 2;
        geometry_mode = PATTERN_MODE;
        break;
    case '6':
        vertexShaderMode = 0;
        geometry_mode = WIRED_MODE;
        break;

    case '+':
        scale = scale * 2.0f;
        if (scale > 16.0f) { scale = 16.0f; }
        break;
    case '-':
        scale = scale / 2.0f;
        if (scale < 1.0f) { scale = 1.0f; } // lossy scaling
        break;
    case '9':
        exponent = exponent * 2.0f;
        break;
    case '0':
        exponent = exponent / 2.0f;
        break;
  }
}

void displayFunc()
{
  // This function performs the actual rendering.

  // First, clear the screen.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Set up the camera position, focus point, and the up vector.
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();
  float eyeX = 1.0f*width/2.0f; 
  float eyeY = 300.0f;
  float eyeZ = 1.0f * height/2.0f-30.0f;
  matrix.LookAt(eyeX, eyeY, eyeZ,
      1.0f*width/2.0f, 1.0f * height / 2.0f, 0.0,
                0.0, 1.0, 0.0);
  matrix.Translate(-37.0f, 0.0f, 0.0f); // to have camera point at center of heightfield
  matrix.Translate(terrainTranslate[0], terrainTranslate[1], terrainTranslate[2]);
  matrix.Scale(11.5f, 11.5f, 11.5f); 
  matrix.Scale(terrainScale[0], terrainScale[1], terrainScale[2]);
  matrix.Rotate(-40.0f, 1.0f, 0.0f, 0.0f);
  matrix.Rotate(terrainRotate[0], 1.0f, 0.0, 0.0);
  matrix.Rotate(terrainRotate[1], 0.0, 1.0f, 0.0);
  matrix.Rotate(terrainRotate[2], 0.0, 0.0, 1.0f);

  // In here, you can do additional modeling on the object, such as performing translations, rotations and scales.
  // ...

  // Read the current modelview and projection matrices from our helper class.
  // The matrices are only read here; nothing is actually communicated to OpenGL yet.
  float modelViewMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetMatrix(modelViewMatrix);

  float projectionMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(projectionMatrix);

  // Upload the modelview and projection matrices to the GPU. Note that these are "uniform" variables.
  // Important: these matrices must be uploaded to *all* pipeline programs used.
  // In hw1, there is only one pipeline program, but in hw2 there will be several of them.
  // In such a case, you must separately upload to *each* pipeline program.
  // Important: do not make a typo in the variable name below; otherwise, the program will malfunction.
  pipelineProgram.SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
  pipelineProgram.SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);

  pipelineProgram.SetUniformVariablei("mode", vertexShaderMode);
  pipelineProgram.SetUniformVariablei("topPolygon", topPolygon);
  pipelineProgram.SetUniformVariablef("exponent", exponent);
  pipelineProgram.SetUniformVariablef("scale", scale);

  // Execute the rendering.
  // Bind the VAO that we want to render. Remember, one object = one VAO. 
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  switch (geometry_mode) {
    case POINT_MODE:
        pointVao.Bind();
        glDrawArrays(GL_POINTS, 0, pointNumVertices);
        break;
    case LINE_MODE:
        lineVao.Bind();
        glDrawArrays(GL_LINE_STRIP, 0, lineNumVertices);
        break;
    case TRIANGLE_MODE:
        triVao.Bind();
        glDrawArrays(GL_TRIANGLE_STRIP, 0, triNumVertices);
        break;
    case SMOOTH_MODE:
        smoothVao.Bind();
        glDrawArrays(GL_TRIANGLE_STRIP, 0, smoothNumVertices);
        break;
    case PATTERN_MODE:
        triVao.Bind();
        glDrawArrays(GL_TRIANGLE_STRIP, 0, triNumVertices);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 5.0f);
        pointVao.Bind();
        glDrawArrays(GL_POINTS, 0, pointNumVertices);
        break;
    case WIRED_MODE:
        triVao.Bind();
        glDrawArrays(GL_TRIANGLE_STRIP, 0, triNumVertices);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 10.0f);
        topPolygon = 1;
        pipelineProgram.SetUniformVariablei("topPolygon", topPolygon);
        lineVao.Bind();
        glDrawArrays(GL_LINE_STRIP, 0, lineNumVertices);
        topPolygon = 0;
        break;

  }

  // Swap the double-buffers.
  glutSwapBuffers();
}

void createHeightfields(const std::unique_ptr<std::vector<float>>& positions, const std::unique_ptr<std::vector<float>>& colors, const std::unique_ptr<ImageIO>& image, int mode)
{
    // helper function to add vertex and color to vbo
    auto addV = [&](int x, int y) {
        float hScale = 4.0f;
        unsigned char gray = image->getPixel(x, y, 0);
        // positions: x = x/(resolution-1), y=height*scale, z=-y/(resolution-1)
        positions->push_back(1.0f * x / 9.0f);
        positions->push_back(static_cast<float>(gray) / 255.0f * hScale);
        positions->push_back(-1.0f * y / 9.0f);

        float color = static_cast<float>(gray) / 255.0f;
        // rgba 
        colors->push_back(color);
        colors->push_back(color);
        colors->push_back(color);
        colors->push_back(1.0f);
    };
    int h = image->getHeight();
    int w = image->getWidth();
    //  |\--
    //  |__\  --> my lines to draw triangles
    if (mode == POINT_MODE) {
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                addV(x, y);
            }
        }
    }
    else {
        for (int y = 0; y < h-1 ; ++y) {
            if (y % 2 == 0) {
                for (int x = 0; x < w -1; ++x) {
                    int bottomLeft[] = { x, y + 1 };
                    int topRight[] = { x + 1, y };
                    int bottomRight[] = {x + 1, y + 1};

                    addV(x, y);

                    // first line
                    addV(bottomLeft[0], bottomLeft[1]);

                    // second line
                    addV(bottomRight[0], bottomRight[1]);

                    //thrid line
                    addV(x, y);

                    // fourth line
                    addV(topRight[0], topRight[1]);
                }
            }
            else {
                for (int x = w - 1; x > 0; --x) {
                    int bottomRight[] = {x, y + 1, 0};
                    int topLeft[] = { x - 1, y, 0 };
                    int bottomLeft[] = { x - 1, y + 1 };

                    addV(x, y);

                    // first line
                    addV(bottomRight[0], bottomRight[1]);

                    // second line
                    addV(bottomLeft[0], bottomLeft[1]);

                    //third line
                    addV(x, y);

                    // fourth line
                    addV(topLeft[0], topLeft[1]);
                }
            }
        }
    }
}

void smoothData(const std::unique_ptr<std::vector<float>>& center, const std::unique_ptr<std::vector<float>>& left, const std::unique_ptr<std::vector<float>>& right, const std::unique_ptr<std::vector<float>>& up, const std::unique_ptr<std::vector<float>>& down, const std::unique_ptr<ImageIO>& image) 
{
    // helper function to add vertex and surrounding vertices
    auto processSurroundingV = [&](int i, int j)
    {
        unsigned char leftGray = image->getPixel(i - 1, j, 0);
        unsigned char rightGray = image->getPixel(i + 1, j, 0);
        unsigned char upGray = image->getPixel(i, j + 1, 0);
        unsigned char downGray = image->getPixel(i, j - 1, 0);
        unsigned char gray = image->getPixel(i, j, 0);


        center->push_back(1.0f * i);
        center->push_back(static_cast<float>(gray) / 255.0f);
        center->push_back(-1.0f * j);

        left->push_back(1.0f * i - 1.0f);
        left->push_back(static_cast<float>(leftGray) / 255.0f);
        left->push_back(-1.0f * j);

        right->push_back(1.0f * i + 1.0f);
        right->push_back(static_cast<float>(rightGray) / 255.0f);
        right->push_back(-1.0f * j);

        up->push_back(1.0f * i);
        up->push_back(static_cast<float>(upGray) / 255.0f);
        up->push_back(-1.0f * j + 1.0f);

        down->push_back(1.0f * i);
        down->push_back(static_cast<float>(downGray) / 255.0f);
        down->push_back(-1.0f * j - 1.0f);
    };

    int h = image->getHeight();
    int w = image->getWidth();
    //  |\--
    //  |__\  --> my lines to draw triangles
    for (int y = 0; y < h-1 ; ++y) {
        if (y % 2 == 0) {
            for (int x = 0; x < w-1 ; ++x) {
                int bottomLeft[] = { x, y + 1 };
                int topRight[] = { x + 1, y };
                int bottomRight[] = { x + 1, y + 1 };

                processSurroundingV(x, y);

                // first line
                processSurroundingV(bottomLeft[0], bottomLeft[1]);

                // second line
                processSurroundingV(bottomRight[0], bottomRight[1]);

                //thrid line
                processSurroundingV(x, y);

                // fourth line
                processSurroundingV(topRight[0], topRight[1]);
            }
        }
        else {
            for (int x = w - 1; x > 0; --x) {
                int bottomRight[] = { x, y + 1 };
                int topLeft[] = { x - 1, y };
                int bottomLeft[] = { x - 1, y + 1 };

                processSurroundingV(x, y);

                // first line
                processSurroundingV( bottomRight[0], bottomRight[1]);

                // second line
                processSurroundingV( bottomLeft[0], bottomLeft[1]);

                //thrid line
                processSurroundingV( x, y);

                // fourth line
                processSurroundingV(topLeft[0], topLeft[1]);
            }
        }
    }
    
}

void initScene(int argc, char *argv[])
{
  // Load the image from a jpeg disk file into main memory.
  std::unique_ptr<ImageIO> heightmapImage = std::make_unique<ImageIO>();
  if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
  {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }
  width = heightmapImage->getWidth();
  height = heightmapImage->getHeight();

  // Set the background color.
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black color.

  // Enable z-buffering (i.e., hidden surface removal using the z-buffer algorithm).
  glEnable(GL_DEPTH_TEST);

  // Create a pipeline program. This operation must be performed BEFORE we initialize any VAOs.
  // A pipeline program contains our shaders. Different pipeline programs may contain different shaders.
  // In this homework, we only have one set of shaders, and therefore, there is only one pipeline program.
  // In hw2, we will need to shade different objects with different shaders, and therefore, we will have
  // several pipeline programs (e.g., one for the rails, one for the ground/sky, etc.).
  // Load and set up the pipeline program, including its shaders.
  if (pipelineProgram.BuildShadersFromFiles(shaderBasePath, "vertexShader.glsl", "fragmentShader.glsl") != 0)
  {
    cout << "Failed to build the pipeline program." << endl;
    throw 1;
  } 
  cout << "Successfully built the pipeline program." << endl;
    
  // Bind the pipeline program that we just created. 
  // The purpose of binding a pipeline program is to activate the shaders that it contains, i.e.,
  // any object rendered from that point on, will use those shaders.
  // When the application starts, no pipeline program is bound, which means that rendering is not set up.
  // So, at some point (such as below), we need to bind a pipeline program.
  // From that point on, exactly one pipeline program is bound at any moment of time.
  pipelineProgram.Bind();
  std::unique_ptr<std::vector<float>> positions;
  std::unique_ptr<std::vector<float>> colors;

  // point
  positions = std::make_unique<std::vector<float>>();
  colors = std::make_unique<std::vector<float>>();
  createHeightfields(positions, colors, heightmapImage, GL_POINTS);
  pointNumVertices = positions->size()/3;
  pointVboVertices.Gen(pointNumVertices, 3, positions.get()->data(), GL_STATIC_DRAW); // 3 values per position
  pointVboColors.Gen(pointNumVertices, 4, colors.get()->data(), GL_STATIC_DRAW); // 4 values per color
  pointVao.Gen();
  pointVao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &pointVboVertices, "position");
  pointVao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &pointVboColors, "color");


  // line
  positions = std::make_unique<std::vector<float>>();
  colors = std::make_unique<std::vector<float>>();
  createHeightfields(positions, colors, heightmapImage, GL_LINE_STRIP);
  lineNumVertices = positions->size() / 3;
  lineVboVertices.Gen(lineNumVertices, 3, positions.get()->data(), GL_STATIC_DRAW); // 3 values per position
  lineVboColors.Gen(lineNumVertices, 4, colors.get()->data(), GL_STATIC_DRAW); // 4 values per color
  lineVao.Gen();
  lineVao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &lineVboVertices, "position");
  lineVao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &lineVboColors, "color");

  // triangle
  positions = std::make_unique<std::vector<float>>();
  colors = std::make_unique<std::vector<float>>();
  createHeightfields(positions, colors, heightmapImage, GL_TRIANGLE_STRIP);
  triNumVertices = positions->size() / 3;
  triVboVertices.Gen(triNumVertices, 3, positions.get()->data(), GL_STATIC_DRAW); // 3 values per position
  triVboColors.Gen(triNumVertices, 4, colors.get()->data(), GL_STATIC_DRAW); // 4 values per color
  triVao.Gen();
  triVao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &triVboVertices, "position");
  triVao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &triVboColors, "color");

  // smooth
  std::unique_ptr<std::vector<float>> center = std::make_unique<std::vector<float>>();
  std::unique_ptr<std::vector<float>> left = std::make_unique<std::vector<float>>();
  std::unique_ptr<std::vector<float>> right = std::make_unique<std::vector<float>>();
  std::unique_ptr<std::vector<float>> up = std::make_unique<std::vector<float>>();
  std::unique_ptr<std::vector<float>> down = std::make_unique<std::vector<float>>();
  smoothData(center, left, right, up, down, heightmapImage);
  smoothNumVertices = center->size() / 3;
  centerVbo.Gen(smoothNumVertices, 3, center.get()->data(), GL_STATIC_DRAW);
  leftVbo.Gen(smoothNumVertices, 3, left.get()->data(), GL_STATIC_DRAW);
  rightVbo.Gen(smoothNumVertices, 3, right.get()->data(), GL_STATIC_DRAW);
  upVbo.Gen(smoothNumVertices, 3, up.get()->data(), GL_STATIC_DRAW);
  downVbo.Gen(smoothNumVertices, 3, down.get()->data(), GL_STATIC_DRAW);
  smoothVao.Gen();
  smoothVao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &centerVbo, "center");
  smoothVao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &leftVbo, "left");
  smoothVao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &rightVbo, "right");
  smoothVao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &upVbo, "up");
  smoothVao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &downVbo, "down");


  // Check for any OpenGL errors.
  std::cout << "GL error status is: " << glGetError() << std::endl;
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    cout << "The arguments are incorrect." << endl;
    cout << "usage: ./hw1 <heightmap file>" << endl;
    exit(EXIT_FAILURE);
  }

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

  cout << "Initializing OpenGL..." << endl;

  #ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);  
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  #ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(windowWidth - 1, windowHeight - 1);
  #endif

  // Tells GLUT to use a particular display function to redraw.
  glutDisplayFunc(displayFunc);
  // Perform animation inside idleFunc.
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);

  // init glew
  #ifdef __APPLE__
    // nothing is needed on Apple
  #else
    // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
      cout << "error: " << glewGetErrorString(result) << endl;
      exit(EXIT_FAILURE);
    }
  #endif

  // Perform the initialization.
  initScene(argc, argv);

  // Sink forever into the GLUT loop.
  glutMainLoop();
}

