
#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>

// Drawing function
void display() {
    glClearColor(0.10f, 0.12f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glutSwapBuffers();
}

// Handle window resizing
void reshape(int width, int height) {
    glViewport(0, 0, width, height);
}

// Keyboard input
void keyboard(unsigned char key, int x, int y) {
    if (key == 27) {
        glutLeaveMainLoop();
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);

    // Request OpenGL 3.3 Core Profile
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);

    glutInitDisplayMode(
        GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH
    );

    glutInitWindowSize(1280, 720);
    glutCreateWindow("3D Maze Escape");

    glewExperimental = GL_TRUE;

    GLenum status = glewInit();

    if (status != GLEW_OK) {
        std::cerr << "GLEW initialization failed\n";
        return 1;
    }

    std::cout << "OpenGL Version: "
        << glGetString(GL_VERSION) << std::endl;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);

    glutMainLoop();

    return 0;
}
